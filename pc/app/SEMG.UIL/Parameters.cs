using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data;
using System.IO;
using System.Xml.Linq;
using System.Windows.Forms;
using System.Reflection;

namespace SEMG.UIL
{
    enum dev_stat : int
    {
        dev_NONE = 0,
        dev_START,
        dev_CONNECT,
        dev_UNCONNECT,
        dev_ERROR
    };

    //!< 工频和基线漂移的处理，两者的选项相或
    enum filter_selector : int
    {
        FILTER_NONE = 1,		//<! no 50hz filter
        FILTER_BUTTERWORTH,
        FILTER_FOURIER,
        FILTER_RLS,
        FILTER_BASELINE_NONE = 0x0100,		//<! don't deal with baseline bias
        FILTER_BASELINE_YES = 0x0200				//<! filter the baseline bias
    };

    public  static class  Parameters
    {
        private static string settingFilename = "Setting.xml";

        /// <summary>
        /// the linux board
        /// </summary>
        public static string ip = "10.13.89.25";

        /// <summary>
        /// the ms length of time to be show in graph, should be 100ms的整数倍
        /// </summary>
        public static int duration = 100;

        /// <summary>
        /// the count of channel to be show 
        /// </summary>
        public static int showChannelCount = 3;

        /// <summary>
        /// using channel count, eg. 128
        /// </summary>
        public static int usingChannelCount = 128;
        
        /// <summary>
        /// total channel count, always 128
        /// </summary>
        public readonly static  int totalChannelCount = 128;
        
        /// <summary>
        /// 数据缩放，为了可调放大所用
        /// </summary>
        public static double scaling = 1;

        /// <summary>
        /// 数据范围, -DataScale ~ DataScale
        /// </summary>
        public readonly static double DataScale = 1;

        /// <summary>
        /// 坐标轴范围, -AxisScale ~ AxisScale
        /// </summary>
        public static double AxisScale = 1;

        public static DataTable ActionGroup;

        public static int filterOption = (int)(filter_selector.FILTER_NONE | filter_selector.FILTER_BASELINE_NONE);

        /// <summary>
        /// get parameters from xml if exists, else create xml using default value
        /// </summary>
        public static void GetSetting()
        {
            try
            {
                if (!File.Exists(settingFilename))
                {
                    createXML(); 
                }
                else
                {
                    XDocument xdoc = XDocument.Load(settingFilename);
                    XElement setting = xdoc.Element("SEMG").Element("Setting");
                    FieldInfo[] fields = typeof(Parameters).GetFields();
                    foreach (var field in fields)
                    {
                        if (field.IsInitOnly)
                            continue;
                        if (field.FieldType == typeof(string))
                            field.SetValue(null, setting.Element(field.Name).Value);
                        else if (field.FieldType == typeof(int))
                            field.SetValue(null, int.Parse(setting.Element(field.Name).Value));
                        else if (field.FieldType == typeof(double))
                            field.SetValue(null, double.Parse(setting.Element(field.Name).Value));
                     }
                }


            }
            catch (IOException exp)
            {
                MessageBox.Show(exp.Message);
            }

        }
        /// <summary>
        /// create setting xml
        /// </summary>
        public static void createXML()
        {
            XElement setting = new XElement("Setting");
            FieldInfo[] fields = typeof(Parameters).GetFields();
            foreach(var field in fields)
            {
                if (field.IsInitOnly)
                    continue;
                if (field.Name == "ActionGroup")
                    continue;
                setting.Add(new XElement(field.Name, field.GetValue(null).ToString()));
            }
            XDocument xdoc = new XDocument(
            new XDeclaration("1.0", "utf-8", "yes"),
            new XComment("Created: " + DateTime.Now.ToString()),
            new XElement("SEMG",
                setting,
                new XElement("Data"),
                new XElement("ActionGroup")
                )

            );
            xdoc.Save(settingFilename);
        }
        /// <summary>
        /// 写入xml配置文件
        /// </summary>
        public static void writeSetting()
        {
            XDocument xdoc = XDocument.Load(settingFilename);
            XElement setting = xdoc.Element("SEMG").Element("Setting");
           // setting.add
            Type t = typeof(Parameters);
            FieldInfo[] fields = typeof(Parameters).GetFields();
            foreach (var field in fields)
            {
                if (field.IsInitOnly)
                    continue;
                if (field.Name == "ActionGroup")
                    continue;
                if (setting.Element(field.Name) == null)
                    throw new ApplicationException("setting xml format error");
                setting.Element(field.Name).Value = field.GetValue(null).ToString();
            }
            xdoc.Save(settingFilename);
        }

        /// <summary>
        /// get actions from xml if exists, else create xml using default value
        /// </summary>
        public static void GetActions()
        {
            try
            {
                if (!File.Exists(settingFilename))
                {
                    createXML();
                }
                else
                {
                    XDocument xdoc = XDocument.Load(settingFilename);
                    XElement actions = xdoc.Element("SEMG").Element("ActionGroup");

                    ActionGroup = new DataTable();
                    DataColumn dc = new DataColumn();
                    dc.ColumnName = "ActionName";
                    dc.DataType = typeof(string);
                    dc.Unique = true;
                    ActionGroup.Columns.Add(dc);

                    dc = new DataColumn();
                    dc.ColumnName = "ActionValue";
                    dc.DataType = typeof(string);
                    dc.Unique = true;
                    ActionGroup.Columns.Add(dc);

                    dc = new DataColumn();
                    dc.ColumnName = "order";
                    dc.DataType = typeof(int);
                   // dc.Unique = true;
                  //  dc.AutoIncrement = true;
                    ActionGroup.Columns.Add(dc);
                    foreach (var action in actions.Descendants())
                    {
                        DataRow dr = ActionGroup.NewRow();
                        dr["ActionName"] = action.Attribute("ActionName").Value;
                        dr["ActionValue"] = action.Attribute("ActionValue").Value;
                        dr["order"] = int.Parse(action.Attribute("order").Value);
                        ActionGroup.Rows.Add(dr);
                    }
                }


            }
            catch (IOException exp)
            {
                MessageBox.Show(exp.Message);
            }

        }
        /// <summary>
        /// 写入actions
        /// </summary>
        public static void writeActions()
        {
            XDocument xdoc = XDocument.Load(settingFilename);
            XElement actions = xdoc.Element("SEMG").Element("ActionGroup");
            actions.RemoveNodes();   
            foreach (DataRow dr in ActionGroup.Select("","order asc"))
            {
                XElement action = new XElement("Action");
                action.SetAttributeValue("ActionName", dr["ActionName"].ToString());
                action.SetAttributeValue("ActionValue", dr["ActionValue"].ToString());
                action.SetAttributeValue("order", dr["order"].ToString());
                actions.Add(action);
            }
            xdoc.Save(settingFilename);
        }

    }
}
