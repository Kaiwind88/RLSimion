﻿using System.Collections.Generic;
using Herd;
using Caliburn.Micro;


namespace Badger.ViewModels
{
    public class ShepherdViewModel : PropertyChangedBase
    {
        const int m_agentTimeoutSeconds = 10;
        const int m_updateTimeSeconds = 5;
        System.Timers.Timer m_timer;

        private Shepherd m_shepherd;
        public Shepherd shepherd { get { return m_shepherd; } set{}}

        private object m_listsLock = new object();
        private List<HerdAgentInfo> m_innerHerdAgentList =
            new List<HerdAgentInfo>();
        private BindableCollection <HerdAgentViewModel> m_herdAgentList
            = new BindableCollection<HerdAgentViewModel>();
        public BindableCollection<HerdAgentViewModel> herdAgentList
        {
            get
            {
                //return m_herdAgentList;
                lock (m_listsLock)
                {
                    m_shepherd.getHerdAgentList(ref m_innerHerdAgentList, m_agentTimeoutSeconds);

                    m_herdAgentList.Clear();
                    foreach (HerdAgentInfo agent in m_innerHerdAgentList)
                        m_herdAgentList.Add(new HerdAgentViewModel(agent));
                }
                return m_herdAgentList;
            }
            set {}
        }
        public int getAvailableHerdAgents(ref List<HerdAgentViewModel> outList)
        {
            //we assume outList needs no synchronization
            int numAvailableCores = 0;
            lock (m_listsLock)
            {
                outList.Clear();
                foreach (HerdAgentInfo agent in m_innerHerdAgentList)
                {
                    if (agent.isAvailable)
                    {
                        outList.Add(new HerdAgentViewModel(agent));
                        numAvailableCores += agent.numProcessors;
                    }
                }
            }
            return numAvailableCores;
        }

        private void notifyHerdAgentChanged()
        {
            NotifyOfPropertyChange(() => herdAgentList);
        }
        private void resendBroadcast(object sender, System.Timers.ElapsedEventArgs e)
        {
            m_shepherd.sendBroadcastHerdAgentQuery();
        }

        public ShepherdViewModel()
        {
            m_shepherd = new Shepherd();
            m_shepherd.setNotifyAgentListChangedFunc(notifyHerdAgentChanged);

            m_timer = new System.Timers.Timer(m_updateTimeSeconds*1000);
            m_shepherd.sendBroadcastHerdAgentQuery();
            m_shepherd.beginListeningHerdAgentQueryResponses();

            m_timer.AutoReset = true;
            m_timer.Elapsed += new System.Timers.ElapsedEventHandler(resendBroadcast);
            m_timer.Start();
        }
    }
}
