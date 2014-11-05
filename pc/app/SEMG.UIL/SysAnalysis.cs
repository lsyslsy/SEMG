using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MathNet.Numerics;

namespace SEMG.UIL
{
    public partial class SysAnalysis : Form
    {
        List<double>[] channelData = new List<double>[Parameters.usingChannelCount];
        double[] baselines = new double[128];//基线漂移
        double[] noises = new double[128];//50Hz 噪声幅度
        double[] SNR = new double[128];//信噪比
       // double[] noise

        public SysAnalysis()
        {
            InitializeComponent();
        }

        
        void AnalysisData()
        {
            //load

            //analysis

            //write result
        }

        //split data

        /**
         * 使用(S+N)/N 标注的信号段功率/空闲时信号功率
         * 
         */
        void NoSignal()
        {

        }

        private void  Noise()
        {

        }


    }
}
