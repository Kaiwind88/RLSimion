﻿using Badger.Simion;
using System.Xml;

namespace Badger.ViewModels
{
    class IntegerValueConfigViewModel: ConfigNodeViewModel
    {
        public IntegerValueConfigViewModel(ExperimentViewModel parentExperiment, ConfigNodeViewModel parent, XmlNode definitionNode, string parentXPath, XmlNode configNode = null)
        {
            commonInit(parentExperiment, parent, definitionNode, parentXPath);

            if (configNode == null || configNode[name] == null)
            {
                //default init
                content = definitionNode.Attributes[XMLConfig.defaultAttribute].Value;
                textColor = XMLConfig.colorDefaultValue;
            }
            else
            {
                //init from config file
                content = configNode[name].InnerText;
            }
        }

        public override ConfigNodeViewModel clone()
        {
            IntegerValueConfigViewModel newInstance =
                new IntegerValueConfigViewModel(m_parentExperiment, m_parent, nodeDefinition, m_parent.xPath);

            newInstance.content = content;
            newInstance.textColor = textColor;
            return newInstance;
        }

        public override bool validate()
        {
            int parsedValue;
            if (int.TryParse(content, out parsedValue))
                return true;
            return false;
        }

    }
}
