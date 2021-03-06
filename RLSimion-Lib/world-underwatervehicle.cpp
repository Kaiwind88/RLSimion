#include "stdafx.h"
#include "world-underwatervehicle.h"
#include "named-var-set.h"
#include "setpoint.h"
#include "config.h"
#include "world.h"
#include "app.h"
#include "reward.h"


CUnderwaterVehicle::CUnderwaterVehicle(CConfigNode* pConfigNode)
{
	METADATA("World", "Underwater-vehicle");
	m_sVSetpoint = addStateVariable("v-setpoint","m/s",-5.0,5.0);
	m_sV = addStateVariable("v","m/s",-5.0,5.0);
	m_sVDeviation = addStateVariable("v-deviation","m/s",-10.0,10.0);

	m_aUThrust = addActionVariable("u-thrust","N",-30.0,30.0);

	FILE_PATH_PARAM setpointFile= FILE_PATH_PARAM(pConfigNode, "Set-Point-File"
		,"The setpoint file", "../config/world/underwater-vehicle/setpoint.txt");

	m_pSetpoint= new CFileSetPoint(setpointFile.get());

	m_pRewardFunction->addRewardComponent(new CToleranceRegionReward("v-deviation", 0.1, 1.0));
	m_pRewardFunction->initialize();
}

CUnderwaterVehicle::~CUnderwaterVehicle()
{
	delete m_pSetpoint;
}



void CUnderwaterVehicle::reset(CState *s)
{
	s->setValue(m_sVSetpoint,m_pSetpoint->getPointSet(0.0));
	s->setValue(m_sVDeviation,m_pSetpoint->getPointSet(0.0));
	s->setValue(m_sV,0.0);
}

void CUnderwaterVehicle::executeAction(CState *s,const CAction *a,double dt)
{
	double newSetpoint = m_pSetpoint->getPointSet(CSimionApp::get()->pWorld->getT());
	double v= s->getValue(m_sV);
	double u= a->getValue(m_aUThrust); //thrust
	double dot_v= (u*(-0.5*tanh((fabs((1.2+0.2*sin(fabs(v)))*v*fabs(v) - u) -30.0)*0.1) + 0.5) 
		- (1.2+0.2*sin(fabs(v)))*v*fabs(v))	/(3.0+1.5*sin(fabs(v)));
	double newV= v + dot_v*dt;

	s->setValue(m_sV,newV);
	s->setValue(m_sVSetpoint,newSetpoint);
	s->setValue(m_sVDeviation,newSetpoint-newV);
}
