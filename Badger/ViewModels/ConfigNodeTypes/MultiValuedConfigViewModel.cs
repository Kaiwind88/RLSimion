﻿using System.IO;
using System.Xml;
using Simion;

namespace Badger.ViewModels
{
    public class MultiValuedConfigViewModel: NestedConfigNode
    {
        private string m_className = "";
        private bool m_bOptional;

        public MultiValuedConfigViewModel(AppViewModel appDefinition, ConfigNodeViewModel parent
            , XmlNode definitionNode, string parentXPath, XmlNode configNode= null, bool initChildren= true)
        {
            commonInit(appDefinition,parent, definitionNode,parentXPath);

            m_className = definitionNode.Attributes[XMLConfig.classAttribute].Value;
            if (definitionNode.Attributes.GetNamedItem(XMLConfig.optionalAttribute) != null)
                m_bOptional = definitionNode.Attributes[XMLConfig.optionalAttribute].Value == "true";
            else m_bOptional = false;
            
            if (configNode!=null)
            {
                foreach(XmlNode configChild in configNode.ChildNodes)
                {
                    if (configChild.Name==name)
                        children.Add(new MultiValuedItemConfigViewModel(appDefinition, this,definitionNode, m_xPath, configChild));
                }
            }
            else //default initialization
            {
                if (initChildren && !m_bOptional)
                    children.Add(new MultiValuedItemConfigViewModel(appDefinition, this,definitionNode, m_xPath));
            }
        }

        public override ConfigNodeViewModel clone()
        {
            MultiValuedConfigViewModel newNestedCopy = getInstance(m_appViewModel, m_parent
                , nodeDefinition, m_parent.xPath) as MultiValuedConfigViewModel;
            foreach (ConfigNodeViewModel child in children)
            {
                newNestedCopy.children.Add(child.clone());
            }
            return newNestedCopy;
        }

        public void addChild()
        {
            children.Add(new MultiValuedItemConfigViewModel(m_appViewModel,this, nodeDefinition, m_xPath));
            m_appViewModel.doDeferredLoadSteps();
        }
        public void removeChild(MultiValuedItemConfigViewModel child)
        {
            children.Remove(child);
        }

        public override bool validate()
        {
            if (!m_bOptional && children.Count == 0) return false;
            return base.validate();
        }

        //the base method is overriden because we don't want the MultiValuedConfigViewModel to add
        //any header or footer... those will be added by the MultiValuedItemConfigViewModel children,
        //only once per child
        public override void outputXML(StreamWriter writer,string leftSpace)
        {
            outputChildrenXML(writer,leftSpace);
        }
    }
}
