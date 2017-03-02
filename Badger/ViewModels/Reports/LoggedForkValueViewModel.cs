﻿using Caliburn.Micro;
using System.Xml;
using System;

namespace Badger.ViewModels
{

    public class LoggedForkValueViewModel: SelectableTreeItem
    {
        private string m_value = "";
        public string value { get { return m_value; } set { m_value = value; } }

        public LoggedForkValueViewModel(XmlNode configNode, ReportsWindowViewModel parent)
        {
            m_parentWindow = parent;
            value = configNode.InnerText;
        }

        public override void TraverseAction(bool doActionLocally,System.Action<SelectableTreeItem> action)
        {
            if (doActionLocally) LocalTraverseAction(action);
        }
    }
}