#pragma once

#include "critic.h"
#include "parameters.h"

class CLinearStateVFA;
class CETraces;
class CFeatureList;
class CConfigNode;
class CNumericValue;



//CCritic implementations

class CTDLambdaCritic : public CCritic
{
	CHILD_OBJECT<CETraces> m_z; //traces
	CFeatureList* m_aux;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pAlpha;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pGamma;
public:
	CTDLambdaCritic(CConfigNode *pParameters);
	virtual ~CTDLambdaCritic();

	double updateValue(const CState *s, const CAction *a, const CState *s_p, double r);
};

class CTrueOnlineTDLambdaCritic : public CCritic
{
	//True Online TD(lambda)
	//Harm van Seijen, Richard Sutton
	//Proceedings of the 31st International Conference on Machine learning

	CHILD_OBJECT<CETraces> m_e; //traces
	CFeatureList* m_aux;
	double m_v_s;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pAlpha;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pGamma;
public:
	CTrueOnlineTDLambdaCritic(CConfigNode *pParameters);
	virtual ~CTrueOnlineTDLambdaCritic();

	double updateValue(const CState *s, const CAction *a, const CState *s_p, double r);

};

class CTDCLambdaCritic : public CCritic
{
	CHILD_OBJECT<CETraces> m_z; //traces
	CFeatureList* m_s_features;
	CFeatureList* m_s_p_features;
	CFeatureList* m_omega;
	CFeatureList* m_a;
	CFeatureList* m_b;

	CHILD_OBJECT_FACTORY<CNumericValue> m_pAlpha;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pGamma;
	CHILD_OBJECT_FACTORY<CNumericValue> m_pBeta;
	

public:
	CTDCLambdaCritic(CConfigNode *pParameters);
	virtual ~CTDCLambdaCritic();

	double updateValue(const CState *s, const CAction *a, const CState *s_p, double r);
};


