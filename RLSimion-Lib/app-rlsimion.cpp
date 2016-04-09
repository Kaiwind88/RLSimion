#include "stdafx.h"
#include "app-rlsimion.h"
#include "world.h"
#include "experiment.h"
#include "SimGod.h"
#include "parameters.h"
#include "logger.h"
#include "named-var-set.h"
#include "globals.h"
#include "app.h"

CLASS_CONSTRUCTOR(RLSimionApp, int argc, char* argv[]) : CApp(argc,argv)
{
	try
	{

		//In the beginning, a logger was created so that we could know about creation itself
		CHILD_CLASS_INIT(Logger, "Log", "The logger class", false, CLogger);
		Logger.setLogDirectory(argv[1]); //we provide the path to the xml configuration file so that the logger saves its log files in the directory

		//Then the world was created by chance
		CHILD_CLASS_INIT(World, "World", "The simulation environment and its parameters", false, CWorld);

		//Then, the experiment.
		CHILD_CLASS_INIT(Experiment, "Experiment", "The parameters of the experiment", false, CExperiment);	//Dependency: it needs DT from the world to calculate the number of steps-per-episode

		//Last, the SimGod was created to create and control all the simions
		CHILD_CLASS_INIT(SimGod, "SimGod", "The god class that controls all aspects of the simulation process", false, CSimGod);
	}
	catch (std::exception& e)
	{
		CLogger::logMessage(MessageType::Error, e.what());
	}

	END_CLASS();
}

RLSimionApp::~RLSimionApp()
{
}

void RLSimionApp::run()
{
	try
	{
		//create state and action vectors
		CState *s = World.getDynamicModel()->getStateDescriptor()->getInstance();
		CState *s_p = World.getDynamicModel()->getStateDescriptor()->getInstance();
		CAction *a = World.getDynamicModel()->getActionDescriptor()->getInstance();
		//register the state and action vectors in the logger
		CApp::Logger.addVarSetToStats("State", s);
		CApp::Logger.addVarSetToStats("Action", a);
		CApp::Logger.addVarToStats("Reward", "sum", CApp::World.getScalarReward());
		CApp::Logger.addVarSetToStats("Reward", CApp::World.getReward());

		double r = 0.0;

		//episodes
		for (CApp::Experiment.nextEpisode(); CApp::Experiment.isValidEpisode(); CApp::Experiment.nextEpisode())
		{
			CApp::World.reset(s);

			//steps per episode
			for (CApp::Experiment.nextStep(); CApp::Experiment.isValidStep(); CApp::Experiment.nextStep())
			{
				//a= pi(s)
				CApp::SimGod.selectAction(s, a);

				//s_p= f(s,a); r= R(s');
				r = CApp::World.executeAction(s, a, s_p);

				//update god's policy and value estimation
				CApp::SimGod.update(s, a, s_p, r);

				//log tuple <s,a,s',r>
				CApp::Experiment.timestep(s, a, s_p, CApp::World.getReward()); //we need the complete reward vector for logging

				//s= s'
				s->copy(s_p);
			}
		}

		delete s;
		delete s_p;
		delete a;
	}
	catch (std::exception& e)
	{
		CLogger::logMessage(MessageType::Error, e.what());
	}
}