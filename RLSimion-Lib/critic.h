#pragma once
#include "parameters.h"

class CNamedVarSet;
typedef CNamedVarSet CState;
typedef CNamedVarSet CAction;

class CConfigNode;
class CLinearStateVFA;

class CCritic
{

protected:
	CHILD_OBJECT<CLinearStateVFA> m_pVFunction; //value function approximator

public:
	CCritic(CConfigNode* pParameters);
	virtual ~CCritic();

	virtual double updateValue(const CState *s, const CAction *a, const CState *s_p, double r) = 0;

	static std::shared_ptr<CCritic> getInstance(CConfigNode* pParameters);
};

