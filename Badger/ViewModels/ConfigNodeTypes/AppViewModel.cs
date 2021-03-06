﻿using System.Xml;
using System.Collections.Generic;
using Caliburn.Micro;
using System.IO;
using Simion;
using Badger.Data;
using System;
using System.Linq;

namespace Badger.ViewModels
{
    //two modes:
    //-CombineForks: for each combination of fork values, a different experiment will be saved
    //-SaveForks: forkedNodes and forks will be saved as a unique experiment
    public enum SaveMode { CombineForks,SaveForks};
    public enum WorldVarType { StateVar, ActionVar,Constant };

    public class AppViewModel: PropertyChangedBase
    {
        public SaveMode saveMode= SaveMode.SaveForks;

        //deferred load step
        public delegate void deferredLoadStep();

        private List<deferredLoadStep> m_deferredLoadSteps= new List<deferredLoadStep>();
        public void registerDeferredLoadStep(deferredLoadStep func) { m_deferredLoadSteps.Add(func); }
        public void doDeferredLoadSteps()
        {
            foreach (deferredLoadStep deferredStep in m_deferredLoadSteps)
                deferredStep();
            m_deferredLoadSteps.Clear();
        }

        //app properties: prerrequisites, exe files, definitions...
        private List<string> m_preFiles= new List<string>();
        public List<string> getPrerrequisites() { return m_preFiles; }
        private string m_exeFile;
        public string getExeFilename() { return m_exeFile; }
        private Dictionary<string,XmlNode> m_classDefinitions = new Dictionary<string, XmlNode>();
        private Dictionary<string,List<string>> m_enumDefinitions 
            = new Dictionary<string, List<string>>();
        

        public XmlNode getClassDefinition(string className, bool bCanBeNull= false)
        {
            if (!m_classDefinitions.ContainsKey(className))
            {
                if (!bCanBeNull)
                    CaliburnUtility.showWarningDialog("Undefined class" + className, "ERROR");
                return null;
            }
            return m_classDefinitions[className];
        }

        public void addEnumeratedType(XmlNode definition)
        {
            List<string> enumeratedValues = new List<string>();
            foreach (XmlNode child in definition)
            {
                enumeratedValues.Add(child.InnerText);
            }

            m_enumDefinitions.Add(definition.Attributes[XMLConfig.nameAttribute].Value
                ,enumeratedValues);
        }
        public List<string> getEnumeratedType(string enumName)
        {
            if (!m_enumDefinitions.ContainsKey(enumName))
            {
                CaliburnUtility.showWarningDialog("Undefined enumeratedType: " + enumName, "ERROR");
                return null;
            }
            return m_enumDefinitions[enumName];
        }

        //the app node's name: RLSimion, ...
        private string m_appName;
        public string appName { get { return m_appName; }set { m_appName = value; NotifyOfPropertyChange(() => appName); } }
        //experiment's name
        private string m_name;
        public string name
        {
            get { return m_name; }
            set { m_name = WindowViewModel.getValidAppName(value); NotifyOfPropertyChange(() => name); }
        }
        //file name (not null if it has been saved)
        private string m_fileName= null;
        public string fileName
        {
            get { return m_fileName; }
            set { m_fileName = value; NotifyOfPropertyChange(() => fileName); }
        }

        private string m_version;

        private BindableCollection<ConfigNodeViewModel> m_children= new BindableCollection<ConfigNodeViewModel>();
        public BindableCollection<ConfigNodeViewModel> children { get { return m_children; }
            set { m_children = value; NotifyOfPropertyChange(() => children); } }

        //Auxiliary XML definition files: Worlds (states and actions)
        //private Dictionary<WorldVarType, List<string>> m_auxDefinitions = new Dictionary<WorldVarType, List<string>>();

        //public void loadAuxDefinitions(string fileName)
        //{
        //    XmlDocument doc = new XmlDocument();

        //    doc.Load(fileName);
        //    XmlNode rootNode= doc.LastChild; //we take here the last node to skip the <?xml ...> initial tag
        //    //HARD-CODED: the list is filled with the contents of the nodes from: ../<hangingFrom>/Variable/Name
        //    foreach (XmlNode child in rootNode.ChildNodes)
        //    {
        //        List<string> definedValues = new List<string>();
        //        string hangingFrom = child.Name;
        //        foreach (XmlNode child2 in child.ChildNodes)
        //        {
        //            if (child2.Name == "Variable")
        //            {
        //                foreach (XmlNode child3 in child2.ChildNodes)
        //                {
        //                    if (child3.Name == "Name")
        //                        definedValues.Add(child3.InnerText);
        //                }
        //            }
        //        }
        //        m_auxDefinitions[hangingFrom] = definedValues;
        //    }
        //    updateXMLDefRefs();
        //}

        private Dictionary<string, WorldDefinition> m_worldDefinitions = new Dictionary<string, WorldDefinition>();
        private string m_selectedWorld="";

        public void selectWorld(string worldName)
        {
            if (m_worldDefinitions.ContainsKey(worldName))
            {
                m_selectedWorld = worldName;
                updateWorldDefinition();
            }
        }

        private void parseClassDefinitionMetadata(XmlNode definition)
        {
            string worldName;
            WorldDefinition worldDefinition;
            if (definition.Attributes.GetNamedItem(XMLConfig.worldAttribute)!=null)
            {
                worldName = definition.Attributes[XMLConfig.worldAttribute].Value;
                worldDefinition = new WorldDefinition();
                foreach(XmlNode child in definition.ChildNodes)
                {
                    if (child.Name == XMLConfig.stateVarTag)
                        worldDefinition.addStateVar(new StateVar(child));
                    else if (child.Name == XMLConfig.actionVarTag)
                        worldDefinition.addActionVar(new ActionVar(child));
                    else if (child.Name == XMLConfig.constantTag)
                        worldDefinition.addConstant(child.Attributes[XMLConfig.nameAttribute].Value);
                }
                m_worldDefinitions.Add(worldName, worldDefinition);
            }
        }

        public void getWorldVarNameList(WorldVarType varType,ref List<string> varNameList)
        {
            if (varNameList!=null && m_selectedWorld!="")
            {
                switch(varType)
                {
                    case WorldVarType.StateVar:
                        m_worldDefinitions[m_selectedWorld].getStateVarNameList(ref varNameList);
                        break;
                    case WorldVarType.ActionVar:
                        m_worldDefinitions[m_selectedWorld].getActionVarNameList(ref varNameList);
                        break;
                    case WorldVarType.Constant:
                        m_worldDefinitions[m_selectedWorld].getConstantNameList(ref varNameList);
                        break;
                }
            }
        }

        //WorldVarRefs
        private List<deferredLoadStep> m_WorldVarRefListeners = new List<deferredLoadStep>();
        public void registerWorldVarRef(deferredLoadStep func)
        { m_WorldVarRefListeners.Add(func); }
        private void updateWorldDefinition()
        {
            foreach (deferredLoadStep func in m_WorldVarRefListeners)
                func();
        }
        public void init(string appDefinitionFileName, XmlNode configRootNode,string experimentName)
        {
            XmlDocument appDefinition = new XmlDocument();
            appDefinition.Load(appDefinitionFileName);

            foreach (XmlNode rootChild in appDefinition.ChildNodes)
            {
                if (rootChild.Name == XMLConfig.appNodeTag)
                {
                    //APP node
                    m_preFiles.Clear();
                    m_appName = rootChild.Attributes[XMLConfig.nameAttribute].Value;

                    name = experimentName;

                    if (rootChild.Attributes.GetNamedItem(XMLConfig.versionAttribute) != null)
                        m_version = rootChild.Attributes[XMLConfig.versionAttribute].Value;
                    else
                    {
                        CaliburnUtility.showWarningDialog("Error reading version attribute: " + XMLConfig.experimentConfigVersion
                            ,"ERROR");
                        m_version = "0.0.0.0";
                    }

                    foreach (XmlNode child in rootChild.ChildNodes)
                    {
                        //Only EXE, PRE, INCLUDE and BRANCH children nodes
                        if (child.Name == XMLConfig.exeNodeTag) m_exeFile = child.InnerText;
                        else if (child.Name == XMLConfig.preNodeTag) m_preFiles.Add(child.InnerText);
                        else if (child.Name == XMLConfig.includeNodeTag)
                            loadIncludedDefinitionFile(child.InnerText);
                        else
                        {
                            children.Add(ConfigNodeViewModel.getInstance(this, null, child, m_appName, configRootNode));
                            //here we assume definitions are before the children
                        }
                    }
                }
            }
            //deferred load step: enumerated types
            doDeferredLoadSteps();
        }
        private WindowViewModel m_parent;
        public WindowViewModel parent { get { return m_parent; }set { m_parent = value; } }

        //This constructor builds the whole tree of ConfigNodes either
        // -with default values ("New")
        // -with a configuration file ("Load")
        public AppViewModel(WindowViewModel parentWindow, string appDefinitionFileName, string configFilename)
        {
            m_parent = parentWindow;
            //Load the configFile if a configFilename is provided
            XmlDocument configDoc= null;
            XmlNode configRootNode = null;
            if (configFilename != null)
            {
                configDoc = new XmlDocument();
                configDoc.Load(configFilename);
                configRootNode = configDoc.LastChild;
            }

            init(appDefinitionFileName, configRootNode, Utility.getFileName(configFilename, true));
        }
        //This constructor is called when a badger file is loaded. Because all the experiments are embedded within a single
        //XML file, the calling method will be passing XML nodes belonging to the single XML file instead of filenames
        public AppViewModel(WindowViewModel parentWindow, string appDefinitionFileName, XmlNode configRootNode,string experimentName)
        {
            m_parent = parentWindow;
            init(appDefinitionFileName, configRootNode,experimentName);
        }

        private void loadIncludedDefinitionFile(string appDefinitionFile)
        {
            XmlDocument definitionFile = new XmlDocument();
            definitionFile.Load(appDefinitionFile);
            foreach (XmlNode child in definitionFile.ChildNodes)
            {
                if (child.Name == XMLConfig.definitionNodeTag)
                {
                    foreach (XmlNode definition in child.ChildNodes)
                    {
                        string name = definition.Attributes[XMLConfig.nameAttribute].Value;
                        if (definition.Name == XMLConfig.classDefinitionNodeTag)
                        {
                            parseClassDefinitionMetadata(definition);
                            m_classDefinitions.Add(name, definition);
                        }
                        else if (definition.Name == XMLConfig.enumDefinitionNodeTag)
                            addEnumeratedType(definition);
                    }
                }
            }
        }

        public bool validate()
        {
            foreach (ConfigNodeViewModel node in m_children)
                if (!node.validate()) return false;
            return true;
        }

        //this method saves an experiment. Depending on the mode:
        //  -SaveMode.CombineForks -> the caller should iterate on i= [0..getNumForkCombinations) and call
        //setCombination(i). This method will then save the i-th combination
        //  -SaveMode.SaveForks -> this method should be called only once per experiment. All the forks will be saved embedded
        //in the config file
        public void save(string filename,SaveMode mode, string leftSpace="")
        {
            using (FileStream stream = File.Create(filename))
            {
                using (StreamWriter writer = new StreamWriter(stream))
                {
                    saveToStream(writer, mode, leftSpace);
                }
            }
        }
        public void saveToStream(StreamWriter writer, SaveMode mode, string leftSpace)
        {
            saveMode = mode;

            writer.WriteLine(leftSpace + "<" + appName + " " + XMLConfig.versionAttribute 
                + "=\"" + XMLConfig.experimentConfigVersion + "\">");

            foreach (ConfigNodeViewModel node in m_children)
            {
                node.outputXML(writer, leftSpace + "  ");
            }
            writer.WriteLine(leftSpace + "</" + appName + ">");
        }


        public string getLogDescriptorsFilePath()
        {
            if (m_fileName != "")
            {
                //the hard way because the elegant way didn't seem to work
                int lastPos1, lastPos2, lastPos;
                lastPos1 = m_fileName.LastIndexOf("/");
                lastPos2 = m_fileName.LastIndexOf("\\");

                lastPos = Math.Max(lastPos1, lastPos2);
                if (lastPos > 0)
                {
                    string directory = m_fileName.Substring(0, lastPos + 1);

                    return directory + "experiment-log.xml";
                }
            }
            return "";
        }
        public string getLogFilePath()
        {
            if (m_fileName != "")
            {
                //the hard way because the elegant way didn't seem to work
                int lastPos1, lastPos2, lastPos;
                lastPos1 = m_fileName.LastIndexOf("/");
                lastPos2 = m_fileName.LastIndexOf("\\");

                lastPos = Math.Max(lastPos1, lastPos2);
                if (lastPos > 0)
                {
                    string directory = m_fileName.Substring(0, lastPos + 1);

                    return directory + "experiment-log.bin";
                }
            }
            return "";
        }

        public string numForkCombinations
        {
            get { return "(" + getNumForkCombinations() + " experiments)"; }
        }

        public void updateNumForkCombinations()
        {
            NotifyOfPropertyChange(() => numForkCombinations);
        }

        public int getNumForkCombinations()
        {
            int numForkCombinations= 1;
            foreach (ConfigNodeViewModel child in children)
                numForkCombinations *= child.getNumForkCombinations();
                    
            return numForkCombinations;
        }

        public string setForkCombination(int combination)
        {
            string combinationName = name;
            int combinationId= combination;
            foreach(ConfigNodeViewModel child in children)
            {
                child.setForkCombination(ref combinationId,ref combinationName);
            }
            return combinationName;
        }

        public void close()
        {
            if (m_parent != null)
                m_parent.close(this);
        }
    }
}
