#pragma once

#include "world.h"
class CSetPoint;
#include "reward.h"

//Original implementation in C by Sutton et al can be found in:
//https://webdocs.cs.ualberta.ca/~sutton/book/code/pole.c

//BALANCING POLE
class CBalancingPole: public CDynamicModel
{
	double GRAVITY;
	double MASSCART;
	double MASSPOLE;
	double TOTAL_MASS;
	double LENGTH;
	double POLEMASS_LENGTH;

	int m_sX, m_sX_dot;
	int m_sTheta, m_sTheta_dot;
	int m_aPitch;

public:
	CBalancingPole(CConfigNode* pParameters);
	virtual ~CBalancingPole() = default;

	void reset(CState *s);

	void executeAction(CState *s, const CAction *a, double dt);
};

class CBalancingPoleReward : public IRewardComponent
{
public:
	double getReward(const CState *s, const CAction *a, const CState *s_p);
	const char* getName(){ return "reward"; }
	double getMin();
	double getMax();
};