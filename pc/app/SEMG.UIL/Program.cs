 using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace SEMG.UIL
{
    static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Parameters.GetActions();
            Parameters.GetSetting();
            Application.Run(new sEMGFrm());
        }
    }
}
