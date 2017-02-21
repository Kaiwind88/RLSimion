﻿using System;
using System.Collections.Generic;
using System.Xml;
using Caliburn.Micro;
using Badger.Simion;

namespace Badger.ViewModels
{
    public class LoggedExperimentViewModel: PropertyChangedBase
    {
        //private const string m_descriptorRootNodeName = "ExperimentLogDescriptor";

        private bool m_bIsSelected = false;
        public bool bIsSelected {
            get { return m_bIsSelected; }
            set 
            {
                m_bIsSelected= value;
                m_parent.updateAvailableVariableList();
                m_parent.updateVariableListHeader();
                m_parent.updateLogListHeader();
                NotifyOfPropertyChange(()=>bIsSelected); }}


        private string m_name = "";
        public string name
        {
            get { return m_name; }
            set { m_name = value; }
        }
        private ReportsWindowViewModel m_parent = null;

        private List<LoggedForkViewModel> m_forks = new List<LoggedForkViewModel>();
        public List<LoggedForkViewModel> forks
        {
            get { return m_forks; }
            set { m_forks = value; }
        }

        private List<LoggedExperimentalUnitViewModel> m_expUnits = new List<LoggedExperimentalUnitViewModel>();
        public List<LoggedExperimentalUnitViewModel> expUnits
        {
            get { return m_expUnits; }
            set { m_expUnits = value; }
        }

        public LoggedExperimentViewModel(XmlNode configNode,ReportsWindowViewModel parent)
        {
            if (configNode.Attributes.GetNamedItem(XMLConfig.nameAttribute)!=null)
                name= configNode.Attributes[XMLConfig.nameAttribute].Value;
            m_parent = parent;
            foreach(XmlNode child in configNode.ChildNodes)
            {
                if (child.Name==XMLConfig.forkTag)
                {
                    LoggedForkViewModel newFork = new LoggedForkViewModel(child);
                    forks.Add(newFork);
                }
                else if (child.Name==XMLConfig.experimentalUnitNodeTag)
                {
                    LoggedExperimentalUnitViewModel newExpUnit
                        = new LoggedExperimentalUnitViewModel(child);
                    expUnits.Add(newExpUnit);
                }
            }
        }

     }
        //private void processDescriptor()
        //{
        //    XmlNode node = m_logDescriptor.FirstChild;
        //    if (node.Name==m_descriptorRootNodeName)
        //    {
        //        foreach (XmlNode child in node.ChildNodes)
        //        {
        //            if (child.Name == "State-variable" || child.Name == "Action-variable" || child.Name == "Reward-variable"
        //                || child.Name == "Stat-variable")
        //            {
        //                string varName = child.InnerText;                
        //                m_variablesInLog.Add(varName);
        //            }
        //        }
        //    }
        //}
        //public void addVariablesToList(ref ObservableCollection<LoggedVariableViewModel> variableList)
        //{
        //    if (variableList.Count==0)
        //    {
        //        //if the list is empty, we add all the variables available in this log
        //        foreach(string var in m_variablesInLog)
        //        {
        //            variableList.Add(new LoggedVariableViewModel(var, m_parent));
        //        }
        //    }
        //    else
        //    {
        //        //else, we intersect both sets: remove all variables not present in this log
        //        foreach(LoggedVariableViewModel variable in variableList)
        //        {
        //            if (!m_variablesInLog.Contains(variable.name))
        //                variableList.Remove(variable);
        //        }
        //    }
        //}


  


        //public class VarPlotInfo
        //{
        //    public PlotViewModel plot;
        //    public int seriesId;
        //    public int varIndexInLogFile;
        //    public double avg;
        //    public void addValue(double value) { avg += value; }
        //}
        ////for now, it doesn't make much sense to calculate stats but from the last evaluation episode, so that's what we will do
        ////regardless of sourceOption
        //public List<Stat> getVariableStats(List<LoggedVariableViewModel> variables)
        //{
        //    ExperimentData experimentData = SimionLogFile.load(m_logFilePath);
        //    List<Stat> stats = new List<Stat>();

        //    foreach (LoggedVariableViewModel var in variables)
        //    {
        //        int varIndex = m_variablesInLog.FindIndex((name) => name == var.name);
        //        Stat newStat = new Stat(m_name, var.name);
        //        newStat.avg = experimentData.doForEpisodeVar(experimentData.numEpisodes, varIndex,
        //             (episode, vIndex) => { return EpisodeData.calculateVarAvg(episode,vIndex); });
        //        newStat.stdDev = experimentData.doForEpisodeVar(experimentData.numEpisodes, varIndex,
        //             (episode, vIndex) => { return EpisodeData.calculateStdDev(episode, vIndex); });
        //        newStat.min = experimentData.doForEpisodeVar(experimentData.numEpisodes, varIndex,
        //             (episode, vIndex) => { return EpisodeData.calculateMin(episode, vIndex); });
        //        newStat.max = experimentData.doForEpisodeVar(experimentData.numEpisodes, varIndex,
        //             (episode, vIndex) => { return EpisodeData.calculateMax(episode, vIndex); });

        //        stats.Add(newStat);
        //    }


        //    return stats;
        //}
        ////we can use directly the name of the plots as the name of the variables
        ////it should be enough for now
        //public void plotData(List<PlotViewModel> plots, string sourceOption)
        //{
        //    List<VarPlotInfo> varInfoList = new List<VarPlotInfo>();

        //    try
        //    {
        //        //init the series and get the index of each logged variable
        //        plots.ForEach((plot) =>
        //        {
        //            VarPlotInfo varInfo = new VarPlotInfo();
        //            varInfo.plot = plot;
        //            varInfo.seriesId = plot.addLineSeries(m_name);
        //            varInfo.varIndexInLogFile = m_variablesInLog.FindIndex((name) => name == plot.name);
        //            varInfo.avg = 0.0;
        //            varInfoList.Add(varInfo);
        //        });

        //        ExperimentData experimentData = SimionLogFile.load(m_logFilePath);

        //        foreach (VarPlotInfo var in varInfoList)
        //        {
        //            int varIndex = var.varIndexInLogFile;

        //            if (sourceOption == ReportsWindowViewModel.m_optionAllEvalEpisodes)
        //                experimentData.doForEachEvalEpisode(episode =>
        //                        {
        //                            double avg = EpisodeData.calculateVarAvg(episode, varIndex);
        //                            var.plot.addLineSeriesValue(var.seriesId, episode.index, avg);
        //                        });
        //            else if (sourceOption == ReportsWindowViewModel.m_optionLastEvalEpisode)
        //                experimentData.doForEpisodeSteps(experimentData.numEpisodes,
        //                    step =>
        //                    {
        //                        var.plot.addLineSeriesValue(var.seriesId, step.stepIndex
        //                                    , step.data[var.varIndexInLogFile]);
        //                    });
        //        }
        //     }
        //    catch (Exception ex)
        //    {
        //        Console.WriteLine("Error generating plots");
        //        Console.WriteLine(ex.ToString());
        //    }
    //    }
    //}
}