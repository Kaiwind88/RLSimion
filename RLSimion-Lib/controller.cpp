#include "stdafx.h"
#include "controller.h"
#include "named-var-set.h"
#include "world.h"
#include "config.h"
#include "parameters-numeric.h"
#include "controller-adaptive.h"
#include "app.h"



std::shared_ptr<CController> CController::getInstance(CConfigNode* pConfigNode)
{
	return CHOICE<CController>(pConfigNode, "Controller", "The specific controller to be used",
	{
		{"PID",CHOICE_ELEMENT_NEW<CPIDController>},
		{"LQR",CHOICE_ELEMENT_NEW<CLQRController>},
		{"Jonkman",CHOICE_ELEMENT_NEW<CWindTurbineJonkmanController>},
		{"Vidal",CHOICE_ELEMENT_NEW<CWindTurbineVidalController>},
		{"Boukhezzar",CHOICE_ELEMENT_NEW<CWindTurbineBoukhezzarController>},
		{"Extended-Vidal",CHOICE_ELEMENT_NEW<CExtendedWindTurbineVidalController>}
	});
}



//LQR//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
	/*int* m_pVariableIndices;
	double *m_pGains;
	int numVars;*/

CLQRGain::CLQRGain(CConfigNode* pConfigNode)
{
	m_variableIndex = STATE_VARIABLE(pConfigNode, "Variable", "The input state variable");
	m_gain = DOUBLE_PARAM(pConfigNode, "Gain", "The gain applied to the input state variable", 0.1);
}
CLQRController::CLQRController(CConfigNode* pConfigNode)
{
	m_outputActionIndex = ACTION_VARIABLE(pConfigNode, "Output-Actionn", "The output action");
	m_gains = MULTI_VALUE<CLQRGain>(pConfigNode, "LQR-Gain", "An LQR gain on an input state variable");

}

CLQRController::~CLQRController()
{
	//for (unsigned int i = 0; i < m_gains.size(); i++) delete m_gains[i];
}

void CLQRController::selectAction(const CState *s, CAction *a)
{
	double output= 0.0; // only 1-dimension so far

	for (unsigned int i= 0; i<m_gains.size(); i++)
	{
		output+= s->getValue(m_gains[i]->m_variableIndex.get())*m_gains[i]->m_gain.get();
	}
	// delta= -K*x
	a->setValue(m_outputActionIndex.get(), -output);
}

int CLQRController::getNumOutputs()
{
	return 1;
}
int CLQRController::getOutputActionIndex(int output)
{
	if (output == 0)
		return m_outputActionIndex.get();
	throw std::exception("CLQRController. Invalid action output given.");
	return -1;
}

//PID//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CPIDController::CPIDController(CConfigNode* pConfigNode)
{
	m_outputActionIndex= ACTION_VARIABLE(pConfigNode, "Output-Action", "The output action");
	m_pKP = CHILD_OBJECT_FACTORY<CNumericValue>(pConfigNode, "KP", "Proportional gain");
	m_pKI = CHILD_OBJECT_FACTORY<CNumericValue>(pConfigNode, "KI", "Integral gain");
	m_pKP = CHILD_OBJECT_FACTORY<CNumericValue>(pConfigNode, "KD", "Derivative gain");

	m_errorVariableIndex = STATE_VARIABLE(pConfigNode, "Input-Variable", "The input state variable");

	m_intError= 0.0;
}

CPIDController::~CPIDController()
{}

int CPIDController::getNumOutputs()
{
	return 1;
}
int CPIDController::getOutputActionIndex(int output)
{
	if (output == 0)
		return m_outputActionIndex.get();
	throw std::exception("CLQRController. Invalid action output given.");
	return -1;
}

void CPIDController::selectAction(const CState *s, CAction *a)
{
	if (CSimionApp::get()->pWorld->getT()== 0.0)
		m_intError= 0.0;

	double error= s->getValue(m_errorVariableIndex.get());
	double dError = error*CSimionApp::get()->pWorld->getDT();
	m_intError += error*CSimionApp::get()->pWorld->getDT();

	a->setValue(m_outputActionIndex.get()
		,error*m_pKP->getValue() + m_intError*m_pKI->getValue() + dError*m_pKD->getValue());

}



//VIDAL////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CWindTurbineVidalController::~CWindTurbineVidalController()
{
}

CWindTurbineVidalController::CWindTurbineVidalController(CConfigNode* pConfigNode)
{
	m_pA= DOUBLE_PARAM(pConfigNode, "A", "A parameter of the torque controller",1.0);
	m_pK_alpha = DOUBLE_PARAM(pConfigNode,  "K_alpha", "K_alpha parameter of the torque controller",5000000);
	m_pKP = DOUBLE_PARAM(pConfigNode, "KP", "Proportional gain of the pitch controller",1.0);
	m_pKI = DOUBLE_PARAM(pConfigNode, "KI", "Integral gain of the pitch controller",0.0);

	CDescriptor& pStateDescriptor = CWorld::getDynamicModel()->getStateDescriptor();
	m_omega_r_index = pStateDescriptor.getVarIndex("omega_r");
	m_d_omega_r_index = pStateDescriptor.getVarIndex("d_omega_r");
	m_E_p_index = pStateDescriptor.getVarIndex("E_p");
	m_T_g_index = pStateDescriptor.getVarIndex("T_g");
	m_beta_index = pStateDescriptor.getVarIndex("beta");
	m_E_int_omega_r_index = pStateDescriptor.getVarIndex("E_int_omega_r");

	CDescriptor& pActionDescriptor = CWorld::getDynamicModel()->getActionDescriptor();

	m_d_beta_index = pActionDescriptor.getVarIndex("d_beta");
	m_d_T_g_index = pActionDescriptor.getVarIndex("d_T_g");
}

int CWindTurbineVidalController::getNumOutputs()
{
	return 2;
}
int CWindTurbineVidalController::getOutputActionIndex(int output)
{
	switch (output)
	{
	case 0: return m_d_beta_index;
	case 1: return m_d_T_g_index;
	default: throw std::exception("CLQRController. Invalid action output given.");
	}
	
	return -1;
}

//aux function used in WindTurbineVidal controller
double CWindTurbineVidalController::sgn(double value)
{
	if (value<0.0) return -1.0;
	else if (value>0.0) return 1.0;

	return 0.0;
}

void CWindTurbineVidalController::selectAction(const CState *s,CAction *a)
{
	//f(omega_r,T_g,d_omega_r,E_p, E_int_omega_r)

	//d(Tg)/dt= (-1/omega_r)*(T_g*(a*omega_r-d_omega_r)-a*P_setpoint + K_alpha*sgn(P_a-P_setpoint))
	//d(beta)/dt= K_p*(omega_ref - omega_r) + K_i*(error_integral)

	double omega_r= s->getValue(m_omega_r_index);
	double d_omega_r= s->getValue(m_d_omega_r_index);
	double error_P= s->getValue(m_E_p_index);

	double T_g= s->getValue(m_T_g_index);
	double beta= s->getValue(m_beta_index);
	
	double d_T_g;
	
	if (omega_r!=0.0) d_T_g= (-1/omega_r)*(T_g*( m_pA.get() *omega_r+d_omega_r) 
		- m_pA.get()*CWorld::getDynamicModel()->getConstant("RatedPower") + m_pK_alpha.get()*sgn(error_P));
	else d_T_g= 0.0;

	double e_omega_r = omega_r - CWorld::getDynamicModel()->getConstant("RatedRotorSpeed"); //NOMINAL WIND SPEED
	double d_beta = m_pKP.get()*e_omega_r 
		+m_pKI.get()*s->getValue(m_E_int_omega_r_index);
				 /*0.5*K_p*error_omega*(1.0+sgn(error_omega))
				+ K_i*s->getValue("integrative_omega_r_error);*/

	a->setValue(m_d_beta_index,d_beta);
	a->setValue(m_d_T_g_index,d_T_g);

}

//BOUKHEZZAR CONTROLLER////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

CWindTurbineBoukhezzarController::~CWindTurbineBoukhezzarController()
{

}

CWindTurbineBoukhezzarController::CWindTurbineBoukhezzarController(CConfigNode* pConfigNode)
{
	m_pC_0	= DOUBLE_PARAM(pConfigNode,"C_0", "C_0 parameter",1.0);
	m_pKP = DOUBLE_PARAM(pConfigNode,"KP", "Proportional gain of the pitch controller",1.0);
	m_pKI = DOUBLE_PARAM(pConfigNode,"KI", "Integral gain of the pitch controller",0.0);

	m_J_t = CWorld::getDynamicModel()->getConstant("HubInertia");
	m_K_t = CWorld::getDynamicModel()->getConstant("DriveTrainTorsionalDamping");
	CDescriptor& pStateDescriptor = CWorld::getDynamicModel()->getStateDescriptor();
	m_omega_r_index = pStateDescriptor.getVarIndex("omega_r");
	m_d_omega_r_index = pStateDescriptor.getVarIndex("d_omega_r");
	m_E_p_index = pStateDescriptor.getVarIndex("E_p");
	m_T_g_index = pStateDescriptor.getVarIndex("T_g");
	m_T_a_index = pStateDescriptor.getVarIndex("T_a");
	m_beta_index = pStateDescriptor.getVarIndex("beta");

	CDescriptor& pActionDescriptor = CWorld::getDynamicModel()->getActionDescriptor();

	m_d_beta_index = pActionDescriptor.getVarIndex("d_beta");
	m_d_T_g_index = pActionDescriptor.getVarIndex("d_T_g");
}

int CWindTurbineBoukhezzarController::getNumOutputs()
{
	return 2;
}
int CWindTurbineBoukhezzarController::getOutputActionIndex(int output)
{
	switch (output)
	{
	case 0: return m_d_beta_index;
	case 1: return m_d_T_g_index;
	default: throw std::exception("CLQRController. Invalid action output given.");
	}

	return -1;
}


void CWindTurbineBoukhezzarController::selectAction(const CState *s,CAction *a)
{
	//d(Tg)/dt= (1/omega_r)*(C_0*error_P - (1/J_t)*(T_a*T_g - K_t*omega_r*T_g - T_g*T_g))
	//d(beta)/dt= K_p*(omega_ref - omega_r)

	double omega_r= s->getValue(m_omega_r_index);
	double C_0= m_pC_0.get();		
	double error_P= -s->getValue(m_E_p_index);	
	double T_a= s->getValue(m_T_a_index);		

	double T_g= s->getValue(m_T_g_index);	
	double beta= s->getValue(m_beta_index);	
	
	double d_T_g= (1.0/omega_r)*(C_0*error_P - (1.0/m_J_t)
		*(T_a*T_g - m_K_t*omega_r*T_g - T_g*T_g));

	double e_omega_r = omega_r - 4.39823; //NOMINAL WIND SPEED
	double d_beta = m_pKP.get()*e_omega_r;// +m_pKI.get()*s->getValue(m_E_int_omega_r_index);

	a->setValue(m_d_beta_index,d_beta); //action->setActionValue(DIM_A_beta,d_beta);
	a->setValue(m_d_T_g_index,d_T_g); //action->setActionValue(DIM_A_torque,d_T_g);

}

//JONKMAN//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CWindTurbineJonkmanController::~CWindTurbineJonkmanController()
{
}

CWindTurbineJonkmanController::CWindTurbineJonkmanController(CConfigNode* pConfigNode)
{
	//GENERATOR SPEED FILTER PARAMETERS
	m_CornerFreq = DOUBLE_PARAM(pConfigNode, "CornerFreq", "Corner Freq. parameter", 1.570796);

	//TORQUE CONTROLLER'S PARAMETERS
	m_VS_RtGnSp = DOUBLE_PARAM(pConfigNode, "VSRtGnSp", "Rated Generator Speed", 121.6805);
	m_VS_SlPc = DOUBLE_PARAM(pConfigNode, "VS_SlPc", "SIPc parameter", 10.0);
	m_VS_Rgn2K = DOUBLE_PARAM(pConfigNode, "VS_Rgn2K", "Rgn2K parameter", 2.332287);
	m_VS_Rgn2Sp = DOUBLE_PARAM(pConfigNode, "VS_Rgn2Sp", "Rgn2Sp parameter", 91.21091);
	m_VS_CtInSp = DOUBLE_PARAM(pConfigNode, "VS_CtInSp", "CtlnSp parameter", 70.16224);
	m_VS_RtPwr = DOUBLE_PARAM(pConfigNode, "VS_RtPwr", "Rated power", 5296610.0);
	m_VS_Rgn3MP = DOUBLE_PARAM(pConfigNode, "VS_Rgn3MP", "Rgn3MP parameter", 0.01745329);
	
	m_VS_SySp    = m_VS_RtGnSp.get()/( 1.0 +  0.01*m_VS_SlPc.get() );
	m_VS_Slope15 = ( m_VS_Rgn2K.get()*m_VS_Rgn2Sp.get()*m_VS_Rgn2Sp.get() )/( m_VS_Rgn2Sp.get() - m_VS_CtInSp.get());
	m_VS_Slope25 = ( m_VS_RtPwr.get()/m_VS_RtGnSp.get())/( m_VS_RtGnSp.get() - m_VS_SySp);

	if ( m_VS_Rgn2K.get()== 0.0 )  //.TRUE. if the Region 2 torque is flat, and thus, the denominator in the ELSE condition is zero
		m_VS_TrGnSp = m_VS_SySp;
	else                          //.TRUE. if the Region 2 torque is quadratic with speed
		m_VS_TrGnSp = ( m_VS_Slope25 - sqrt( m_VS_Slope25*( m_VS_Slope25 - 4.0*m_VS_Rgn2K.get()*m_VS_SySp ) ) )/( 2.0*m_VS_Rgn2K.get() );

	//PITCH CONTROLLER'S PARAMETERS
	m_PC_KK = DOUBLE_PARAM(pConfigNode,"PC_KK","Pitch angle were the the derivative of the...", 0.1099965);
	m_PC_KP= DOUBLE_PARAM(pConfigNode, "PC_KP","Proportional gain of the pitch controller",0.01882681);
	m_PC_KI= DOUBLE_PARAM(pConfigNode, "PC_KI","Integral gain of the pitch controller",0.008068634);
	m_PC_RefSpd= DOUBLE_PARAM(pConfigNode,"PC_RefSpd","Pitch control reference speed", 122.9096);

	m_IntSpdErr= 0.0;

	CDescriptor& pStateDescriptor = CWorld::getDynamicModel()->getStateDescriptor();
	m_omega_g_index = pStateDescriptor.getVarIndex("omega_g");

	m_E_p_index = pStateDescriptor.getVarIndex("E_p");
	m_T_g_index = pStateDescriptor.getVarIndex("T_g");
	m_beta_index = pStateDescriptor.getVarIndex("beta");

	CDescriptor& pActionDescriptor = CWorld::getDynamicModel()->getActionDescriptor();

	m_d_beta_index = pActionDescriptor.getVarIndex("d_beta");
	m_d_T_g_index = pActionDescriptor.getVarIndex("d_T_g");
}

int CWindTurbineJonkmanController::getNumOutputs()
{
	return 2;
}
int CWindTurbineJonkmanController::getOutputActionIndex(int output)
{
	switch (output)
	{
	case 0: return m_d_beta_index;
	case 1: return m_d_T_g_index;
	default: throw std::exception("CLQRController. Invalid action output given.");
	}

	return -1;
}

void CWindTurbineJonkmanController::selectAction(const CState *s,CAction *a)
{
	//Filter the generator speed
	double Alpha;

	if (CSimionApp::get()->pWorld->getT() == 0.0)
	{
		Alpha= 1.0;
		m_GenSpeedF= s->getValue(m_omega_g_index);
	}
	else
		Alpha = exp(-CSimionApp::get()->pWorld->getDT()*m_CornerFreq.get());

	m_GenSpeedF = (1.0 - Alpha)*s->getValue(m_omega_g_index) + Alpha*m_GenSpeedF;

	//TORQUE CONTROLLER
	double DesiredGenTrq;
	if ( (   m_GenSpeedF >= m_VS_RtGnSp.get() ) 
		|| (  s->getValue(m_beta_index) >= m_VS_Rgn3MP.get() ) )   //We are in region 3 - power is constant
		DesiredGenTrq = m_VS_RtPwr.get()/m_GenSpeedF;
	else if ( m_GenSpeedF <= m_VS_CtInSp.get() )							//We are in region 1 - torque is zero
		DesiredGenTrq = 0.0;
	else if ( m_GenSpeedF <  m_VS_Rgn2Sp.get() )                          //We are in region 1 1/2 - linear ramp in torque from zero to optimal
		DesiredGenTrq = m_VS_Slope15*( m_GenSpeedF - m_VS_CtInSp.get() );
	else if ( m_GenSpeedF <  m_VS_TrGnSp )                                      //We are in region 2 - optimal torque is proportional to the square of the generator speed
		DesiredGenTrq = m_VS_Rgn2K.get()*m_GenSpeedF*m_GenSpeedF;
	else                                                                       //We are in region 2 1/2 - simple induction generator transition region
		DesiredGenTrq = m_VS_Slope25*( m_GenSpeedF - m_VS_SySp   );

	DesiredGenTrq  = std::min( DesiredGenTrq, s->getProperties("T_g").getMax()  );   //Saturate the command using the maximum torque limit

	double TrqRate;
	TrqRate = (DesiredGenTrq - s->getValue(m_T_g_index)) / CSimionApp::get()->pWorld->getDT(); //Torque rate (unsaturated)
	a->setValue(m_d_T_g_index,TrqRate);

	//PITCH CONTROLLER
	double GK = 1.0/( 1.0 + s->getValue(m_beta_index)/m_PC_KK.get() );

	//Compute the current speed error and its integral w.r.t. time; saturate the
	//  integral term using the pitch angle limits:
	double SpdErr    = m_GenSpeedF - m_PC_RefSpd.get();                                 //Current speed error
	m_IntSpdErr = m_IntSpdErr + SpdErr*CSimionApp::get()->pWorld->getDT();                           //Current integral of speed error w.r.t. time
	//Saturate the integral term using the pitch angle limits, converted to integral speed error limits
	m_IntSpdErr = std::min( std::max( m_IntSpdErr, s->getProperties(m_beta_index).getMax()/( GK*m_PC_KI.get() ) )
		, s->getProperties("beta").getMin()/( GK*m_PC_KI.get() ));
  
	//Compute the pitch commands associated with the proportional and integral
	//  gains:
	double PitComP   = GK* m_PC_KP.get() *   SpdErr; //Proportional term
	double PitComI   = GK* m_PC_KI.get() * m_IntSpdErr; //Integral term (saturated)

	//Superimpose the individual commands to get the total pitch command;
	//  saturate the overall command using the pitch angle limits:
	double PitComT   = PitComP + PitComI;                                     //Overall command (unsaturated)
	PitComT   = std::min( std::max( PitComT, s->getProperties(m_beta_index).getMin() )
		, s->getProperties(m_beta_index).getMax() );           //Saturate the overall command using the pitch angle limits

	//Saturate the overall commanded pitch using the pitch rate limit:
	//NOTE: Since the current pitch angle may be different for each blade
	//      (depending on the type of actuator implemented in the structural
	//      dynamics model), this pitch rate limit calculation and the
	//      resulting overall pitch angle command may be different for each
	//      blade.

	double d_beta = (PitComT - s->getValue(m_beta_index)) / CSimionApp::get()->pWorld->getDT();
	
	a->setValue(m_d_beta_index,d_beta);
}