﻿using Simion;
using System.Xml;
using System.IO;
using System.Windows.Forms;

namespace Badger.ViewModels
{
    class DirPathValueConfigViewModel: ConfigNodeViewModel
    {
        public DirPathValueConfigViewModel(AppViewModel appDefinition, ConfigNodeViewModel parent, XmlNode definitionNode, string parentXPath, XmlNode configNode = null)
        {
            commonInit(appDefinition, parent, definitionNode, parentXPath);

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
            DirPathValueConfigViewModel newInstance =
                new DirPathValueConfigViewModel(m_appViewModel, m_parent, nodeDefinition, m_parent.xPath);
           
            newInstance.content = content;
            newInstance.textColor = textColor;
            return newInstance;
        }


        public override bool validate()
        {
            return Directory.Exists(content);
        }


        public void selectFolder()
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            if (m_default != "" && Directory.Exists(m_default))
            {
                if (Path.IsPathRooted(m_default))
                    fbd.SelectedPath = m_default;
                else
                    fbd.SelectedPath = System.IO.Path.GetFullPath(Path.Combine(Directory.GetCurrentDirectory(),content));
            }

            if (fbd.ShowDialog() == DialogResult.OK)
            {
                content = Data.Utility.GetRelativePathTo(Directory.GetCurrentDirectory(), fbd.SelectedPath);
            }
            content = content;
        }
    }
}
