#include "stdafx.h"
#include "parameters-numeric.h"
#include "config.h"
#include "experiment.h"
#include "app.h"

//std::list<INumericValue*> INumericValue::m_handlers;

class CConstantValue : public CNumericValue
{
	DOUBLE_PARAM m_value;
public:
	CConstantValue(CConfigNode* pConfigNode);
	double getValue(){ return m_value.get(); }
};

CConstantValue::CConstantValue(CConfigNode* pConfigNode)
{
	//<ALPHA>0.1</ALPHA>
	m_value = DOUBLE_PARAM(pConfigNode, "Value", "Constant value", 0.0);
	// CONST_DOUBLE_VALUE(m_value, "Value", 0.0, "Constant value");
}



class CInterpolatedValue : public CNumericValue
{
	DOUBLE_PARAM m_startOffset; //normalized offset to start
	DOUBLE_PARAM m_preOffsetValue; //value before the start of the schedule
	DOUBLE_PARAM m_startValue;
	DOUBLE_PARAM m_endValue;
	DOUBLE_PARAM m_evaluationValue;
	ENUM_PARAM<Interpolation> m_interpolation;
	ENUM_PARAM<TimeReference> m_timeReference;
public:
	CInterpolatedValue(CConfigNode* pParameters);
	double getValue();
};

CInterpolatedValue::CInterpolatedValue(CConfigNode* pConfigNode)
{
	m_startOffset = DOUBLE_PARAM(pConfigNode, "Start-Offset", "Normalized time from which the schedule will begin [0...1]", 0.0);
	//CONST_DOUBLE_VALUE(m_startOffset, "Start-Offset", 0.0,"Normalized time from which the schedule will begin [0...1]");
	m_preOffsetValue = DOUBLE_PARAM(pConfigNode, "Pre-Offset-Value", "Output value before the schedule begins", 0.1);
	//CONST_DOUBLE_VALUE(m_preOffsetValue, "Pre-Offset-Value", 0.0,"Output value before the schedule begins");
	m_interpolation = ENUM_PARAM<Interpolation>(pConfigNode, "Interpolation", "Interpolation type", Interpolation::linear);
	//ENUM_VALUE(m_interpolation, Interpolation, "Interpolation", "linear","Interpolation type");
	m_timeReference = ENUM_PARAM<TimeReference>(pConfigNode, "Time-reference", "The time-reference type", TimeReference::experiment);
	//ENUM_VALUE(m_timeReference, TimeReference, "Time-reference", "experiment","Time reference");
	m_startValue = DOUBLE_PARAM(pConfigNode,"Initial-Value", "Output value at the beginning of the schedule", 0.1);
	//CONST_DOUBLE_VALUE(m_startValue, "Initial-Value", 0.0,"Output value at the beginning of the schedule");
	m_endValue = DOUBLE_PARAM(pConfigNode, "End-Value", "Output value at the end of the schedule", 0.0);
	//CONST_DOUBLE_VALUE(m_endValue, "End-Value", 1.0,"Output value at the end of the schedule");
	m_evaluationValue = DOUBLE_PARAM(pConfigNode, "Evaluation-Value", "Output value during evaluation episodes", 0.0);
	//CONST_DOUBLE_VALUE(m_evaluationValue, "Evaluation-Value", 0.0,"Output value during evaluation episodes");
}


double CInterpolatedValue::getValue()
{
	double progress;

	//evalution episode?
	if (CSimionApp::get()->pExperiment->isEvaluationEpisode())
		return m_evaluationValue.get();
	//time reference
	if (m_timeReference.get()==TimeReference::experiment)
		progress = CSimionApp::get()->pExperiment->getExperimentProgress();
	else if (m_timeReference.get()==TimeReference::episode)
		progress = CSimionApp::get()->pExperiment->getEpisodeProgress();

	if (m_startOffset.get() != 0.0)
	{
		if (progress < m_startOffset.get()) return m_preOffsetValue.get();

		progress = (progress - m_startOffset.get()) / (1 - m_startOffset.get());
	}
	//interpolation
	if (m_interpolation.get()==Interpolation::linear)
		return m_startValue.get() 
		+ (m_endValue.get() - m_startValue.get())* progress;
	else if (m_interpolation.get()==Interpolation::quadratic)
		return m_startValue.get()
		+ (1. - pow(1 - progress, 2.0))*(m_endValue.get() - m_startValue.get())* progress;
	else if (m_interpolation.get()==Interpolation::cubic)
		return m_startValue.get()
		+ (1. - pow(1 - progress, 3.0))*(m_endValue.get() - m_startValue.get())* progress;
	else assert(0);

	return 0.0;
}

class CBhatnagarSchedule : public CNumericValue
{
	DOUBLE_PARAM m_alpha_0; //alpha_0
	DOUBLE_PARAM m_alpha_c; //alpha_c
	DOUBLE_PARAM m_t_exp; // exp
	DOUBLE_PARAM m_evaluationValue; //value returned during evaluation episodes
	ENUM_PARAM<TimeReference> m_timeReference;
public:
	//alpha_t= alpha_0*alpha_c / (alpha_c+t^{exp})
	CBhatnagarSchedule(CConfigNode* pParameters);
	double getValue();
};


CBhatnagarSchedule::CBhatnagarSchedule(CConfigNode* pConfigNode)
{
	m_timeReference = ENUM_PARAM<TimeReference>(pConfigNode, "Time-reference", "The time reference", TimeReference::episode);
	//ENUM_VALUE(m_timeReference, TimeReference,  "Time-reference", "linear","Time reference"); //<- "linear" was a bug
	m_alpha_0 = DOUBLE_PARAM(pConfigNode, "Alpha-0", "Alpha-0 parameter in Bhatnagar's schedule", 1.0);
	//CONST_DOUBLE_VALUE(m_alpha_0,  "Alpha-0", 1.0,"Alpha-0 parameter in Bhatnagar's schedule");
	m_alpha_c = DOUBLE_PARAM(pConfigNode, "Alpha-c", "Alpha-c parameter in Bhatnagar's schedule", 1.0);
	//CONST_DOUBLE_VALUE(m_alpha_c, "Alpha-c", 1.0, "Alpha-c parameter in Bhatnagar's schedule");
	m_t_exp = DOUBLE_PARAM(pConfigNode, "Time-Exponent", "Time exponent in Bhatnagar's schedule", 1.0);
	//CONST_DOUBLE_VALUE(m_t_exp, "Time-Exponent", 1.0, "Time exponent in Bhatnagar's schedule");
	m_evaluationValue= DOUBLE_PARAM(pConfigNode, "Evaluation-Value", "Output value during evaluation episodes", 0.0);
	//CONST_DOUBLE_VALUE(m_evaluationValue, "Evaluation-Value", 0.0,"Output value during evaluation episodes");
}


double CBhatnagarSchedule::getValue()
{
	double t;

	//evalution episode?
	if (CSimionApp::get()->pExperiment->isEvaluationEpisode())
		return m_evaluationValue.get();
	//time reference
	if (m_timeReference.get()==TimeReference::experiment)
		t = CSimionApp::get()->pExperiment->getStep()
		+ (CSimionApp::get()->pExperiment->getEpisodeIndex() - 1) * CSimionApp::get()->pExperiment->getNumSteps();
	else if (m_timeReference.get()==TimeReference::episode)
		t = CSimionApp::get()->pExperiment->getStep();
	else assert(0);

	return m_alpha_0.get()*m_alpha_c.get() / (m_alpha_c.get() + pow(t, m_t_exp.get()));
}


std::shared_ptr<CNumericValue> CNumericValue::getInstance(CConfigNode* pConfigNode)
{
	return CHOICE<CNumericValue>(pConfigNode, "Schedule", "Schedule-type",
	{
		{"Constant",CHOICE_ELEMENT_NEW<CConstantValue>},
		{"Linear-Schedule",CHOICE_ELEMENT_NEW<CInterpolatedValue>},
		{"Bhatnagar-Schedule",CHOICE_ELEMENT_NEW<CBhatnagarSchedule>}
	});

}
