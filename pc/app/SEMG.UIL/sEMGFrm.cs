using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ZedGraph;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Collections.Specialized;

using Emgu.CV;
using Emgu.Util;
using Emgu.CV.Structure;

namespace SEMG.UIL
{      
    


    public partial class sEMGFrm : Form
    {
        System.Timers.Timer dataTimer;
        System.Timers.Timer connectTimer;
        System.Timers.Timer countTimer;
        Byte channelNum = 0;
        Byte sampleRate = 0;//的化简开始看粉红色的
        long tickCount = 0;
        bool closing = false;
        bool bStatus = false;
        long sampleTime = 0;
        private string selectedAction = "0";
        StringBuilder dllInfo = new StringBuilder(100);
        // List<sEMGdata> ldata = new List<sEMGdata>();
        //PointPairList pl = new PointPairList();
        List<double>[] channelData = new List<double>[Parameters.usingChannelCount+1];
        PointPairList[] pl = new PointPairList[10];
        int[] channelIndex = new int[10];
        filters.Butterworth_4order[] bw_filters =new filters.Butterworth_4order[10];
        private const int POINT_NUM = 100;
        FileHelper fileHelper;
  
        [StructLayout(LayoutKind.Sequential)]
        struct sEMGdata
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = POINT_NUM)]
            public double[] point;// = new double[POINT_NUM];
        };
        #region 外部引用
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int sEMG_open(int a, string ip, int filter_options);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int sEMG_close();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int sEMG_reset(int wait, string ip, int filter_options);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void get_dll_info(StringBuilder pinfo);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void clearbuffer();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int get_dev_stat();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void get_spi_stat(IntPtr p);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern Byte get_channel_num();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern Byte get_AD_rate();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern uint get_timestamp();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern uint get_losenum();
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int get_sEMG_data(int channel_id, uint size, IntPtr pd);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void set_data_notify(scallBack pfunc);
        [DllImport("sEMG_DLL.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void reset_data_notify();
        #endregion

        public delegate void scallBack();
        delegate void dosomething();
        EventHandler ds;
        scallBack myCallBack;
        public sEMGFrm()
        {
            InitializeComponent();

            channelView.MultiSelect = false;
          //  btnGetData.BackColor = Color.Green;
            ds = new EventHandler(hallo);
            for (int i = 0; i < pl.Length; i++)
            {
                channelIndex[i] = -1;
                pl[i] = new PointPairList();
               
            }
            for (int i = 0; i < channelData.Length; i++)
            {
                channelData[i] = new List<double>();
            }
                
            Graph_Show();
            // channelView.View = View.List;
            //showVideo();
                     
        }
       
        /// <summary>
        /// 跨平台调用
        /// </summary>
        void GetData()
        {
            tickCount++;
            if (tickCount == 10)
            {
                countTimer = new System.Timers.Timer(10000);//10s
                countTimer.Elapsed += new System.Timers.ElapsedEventHandler(countTimer_Elapsed);
                countTimer.AutoReset = false;
                countTimer.Start();
            }

            if (ar != null && !ar.IsCompleted)
            {
                Trace.WriteLine("处理超时");
                pd.EndInvoke(ar);
            }
            byte[] spi_tat = new byte[2];
            IntPtr spi_p =  Marshal.AllocHGlobal(2);
            get_spi_stat(spi_p);
            Marshal.Copy(spi_p, spi_tat, 0, 2);
            if (spi_tat[0] != 0)
            {
                MessageBox.Show("fatal wrong");
            }
            if (spi_tat[1] != 0)
            {
                try
                {
                    if (!(closing || this.IsDisposed || this.Disposing))
                        this.BeginInvoke((EventHandler)delegate
                        {
   /////////////////////////     
                            if(listBox1.Items.Count>100)
                                listBox1.Items.RemoveAt(0);
                            listBox1.Items.Add(Convert.ToString(spi_tat[1], 2).PadLeft(8, '0')+"\n");
                            listBox1.SelectedIndex = listBox1.Items.Count - 1;
                        });
                }

                catch (ObjectDisposedException e)
                { }
                catch (Exception e)
                {
                    throw e;
                }
            }
            //初始内存分配，放循环里将极大增加内存使用量
            sEMGdata[] mydata = new sEMGdata[10];
            int size = Marshal.SizeOf(typeof(sEMGdata)) * 10;
            IntPtr p = Marshal.AllocHGlobal(size);//= &mydata;
            byte[] bytes = new byte[size];
            //从所有通道中读取数据
            for (int _channelIndex = 0; _channelIndex < Parameters.usingChannelCount; _channelIndex++)
            { 
                //在回调中调用，将不会有数据同步问题
                int count = get_sEMG_data(_channelIndex, 10, p);
                for (int j = 0; j < count; j++)
                {
                    IntPtr pPonitor = new IntPtr(p.ToInt64() + Marshal.SizeOf(typeof(sEMGdata)) * j);
                    mydata[j] = (sEMGdata)Marshal.PtrToStructure(pPonitor, typeof(sEMGdata));

                }        
                channelData[_channelIndex].Clear();                //read all buffered data 
                for (int j = 0; j < count; j++)
                {
                    double data;
                    for (int k = 0; k< POINT_NUM; k++)
                    {
                        data = mydata[j].point[k] * Parameters.scaling;
                        if (data > 1) data = 1;
                        if (data < -1) data = -1;
                        channelData[_channelIndex].Add(data);
                    }
                }
            }


            pd = processdata;
            ar = pd.BeginInvoke(null, null);

        }
        IAsyncResult ar;
        public delegate void processdataDelegate();
       
        processdataDelegate pd;
        void processdata()
        {
            StringCollection stringCollection = new StringCollection();
            if (checkRecord.Checked && btnGetData.Enabled == false)//need to write data
            {
                for (int i = 0; i < channelData[0].Count; i++)
                {
                    StringBuilder sb = new StringBuilder(1500);//for optimization
                    sb.Append(string.Format("{0}.{1:D3}\t", sampleTime / 1000, sampleTime % 1000));
                    sampleTime++;
                    for (int _channelIndex = 0; _channelIndex < Parameters.usingChannelCount; _channelIndex++)
                    {
                        sb.Append(channelData[_channelIndex][i].ToString() + "\t");
                    }
                    sb.Append(selectedAction);
                    stringCollection.Add(sb.ToString());
                }

                try
                {
                    fileHelper.WriteSC(stringCollection);
                }
                catch (Exception exp)
                {
                    MessageBox.Show("写入文件错误:\n" + exp.ToString(), "失败");
                }
            }

            byte[] img = new byte[128];
            if (checkVideo.Checked)
            {

                for (int ii = 0; ii < Parameters.totalChannelCount; ii++)
                {
                    img[ii] = (byte)(Math.Abs(channelData[ii][0])*255);
                }

            }
            else //read data and add point
                lock (pl)   //lock pl
                {
                    for (int ii = 0; ii < Parameters.showChannelCount; ii++)
                    {
                        if (channelIndex[ii] == -1)
                        {
                            pl[ii].Clear();
                            continue;
                        }


                        List<double> dout = new List<double>();
                        //dout.AddRange(channelData[ii].ToList<double>());
                        //filters.filter(ref bw_filters[ii], channelData[channelIndex[ii]], ref dout, (uint)channelData[channelIndex[ii]].Count);
                        for (int i = 0; i < channelData[channelIndex[ii]].Count; i++)
                        {
                            pl[ii].Add(0, (double)channelData[channelIndex[ii]][i]);
                            // pl[ii].Add(0, dout[i]);
                        }
                        //remain only configure ms
                        if (pl[ii].Count >= Parameters.duration)
                        {
                            pl[ii].RemoveRange(0, pl[ii].Count - Parameters.duration);
                        }
                        //set x axis
                        for (int i = 0; i < pl[ii].Count; i++)
                        {
                            pl[ii][i].X = i;
                        }
                    }
                }
            try
            {
                if (!(closing || this.IsDisposed || this.Disposing))
                    this.BeginInvoke((EventHandler)delegate
                    {
                        if (checkVideo.Checked)
                        {
                            showVideo(img);
                        }
                        else
                        {
                            zedGraphControl1.AxisChange();
                            zedGraphControl1.Refresh();
                        }
                    });
            }

            catch (ObjectDisposedException e)
            { }
            catch (Exception e)
            {
                throw e;
            }

        }

        void countTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            toolStripStatusLabel5.Text = "周期数: " + ((tickCount - 10)).ToString();
            tickCount = 0;
        }

        void hallo(object sender, EventArgs e)
        {
            toolStripStatusLabel1.Text = "Yao";
            //textBox1.Text += System.Threading.Thread.CurrentThread.ManagedThreadId.ToString();
        }
        void Graph_Show()
        {

            MasterPane master = zedGraphControl1.MasterPane;

            master.PaneList.Clear();
            master.Fill = new Fill(Color.White, Color.FromArgb(220, 220, 255), 45.0f);
            master.PaneList.Clear();

            //master.Title.IsVisible = true;
            //master.Title.Text = "SEMG wave plot";

            master.Margin.All = 10;
            master.InnerPaneGap = 0;

            ColorSymbolRotator rotator = new ColorSymbolRotator();

            for (int j = 0; j < Parameters.showChannelCount; j++)
            {
                // Create a new graph with topLeft at (40,40) and size 600x400
                GraphPane myPaneT = new GraphPane(new Rectangle(40, 40, 600, 400),
                    "Case #" + (j + 1).ToString(),
                    "时间(ms)",
                    "signal(mV)");

                myPaneT.Fill.IsVisible = false;

                myPaneT.Chart.Fill = new Fill(Color.White, Color.LightYellow, 45.0F);
                myPaneT.BaseDimension = 3.0F;
                myPaneT.XAxis.Title.IsVisible = false;
                myPaneT.XAxis.Scale.IsVisible = false;
                myPaneT.Legend.IsVisible = false;
                myPaneT.Border.IsVisible = false;
                myPaneT.Title.IsVisible = false;
                myPaneT.XAxis.MajorTic.IsOutside = false;
                myPaneT.XAxis.MinorTic.IsOutside = false;
                myPaneT.XAxis.MajorGrid.IsVisible = true;
                myPaneT.XAxis.MinorGrid.IsVisible = true;
                myPaneT.XAxis.Scale.Max = Parameters.duration;
                myPaneT.XAxis.Scale.Min = 0;
                myPaneT.YAxis.Scale.Max = Parameters.AxisScale;
                myPaneT.YAxis.Scale.Min = -Parameters.AxisScale;
                myPaneT.Margin.All = 0;
                //if (j == 0)
                //    myPaneT.Margin.Top = 20;
                if (j == Parameters.showChannelCount - 1)
                {
                    myPaneT.XAxis.Title.IsVisible = true;
                    myPaneT.XAxis.Scale.IsVisible = true;
                    // myPaneT.Margin.Bottom = 10;
                }

                if (j > 0)
                    myPaneT.YAxis.Scale.IsSkipLastLabel = true;

                // This sets the minimum amount of space for the left and right side, respectively
                // The reason for this is so that the ChartRect's all end up being the same size.
                myPaneT.YAxis.MinSpace = 60;
                myPaneT.Y2Axis.MinSpace = 20;

                LineItem myCurve = myPaneT.AddCurve("Type " + j.ToString(),
                    pl[j], Color.Blue/*rotator.NextColor*/, SymbolType.None);
                myCurve.Symbol.Fill = new Fill(Color.White);

                master.Add(myPaneT);
            }

            using (Graphics g = zedGraphControl1.CreateGraphics())
            {
                ZedGraphControl z1 = zedGraphControl1;

                master.SetLayout(g, PaneLayout.SingleColumn);
                z1.AxisChange();

                z1.IsAutoScrollRange = true;
                z1.IsShowHScrollBar = true;
                z1.IsShowVScrollBar = true;
                z1.IsSynchronizeXAxes = true;

            }

            //base.ZedGraphControl.AxisChange();
            GraphPane myPane = zedGraphControl1.GraphPane;

            //// Set the title and axis labels
            //myPane.Title.Text = "Signal wave plot";
            //myPane.XAxis.Title.Text = "时间(ms)";
            //myPane.YAxis.Title.Text = "信号大小(V)";
            //myPane.XAxis.Scale.Max = Parameters.duration;
            //myPane.XAxis.Scale.Min = 0;
            //myPane.YAxis.Scale.Max = 3;
            //myPane.YAxis.Scale.Min = -3;
            //// Make up some data arrays based on the Sine function
            //PointPairList list1 = new PointPairList();
            //PointPairList list2 = new PointPairList();
            //for (int i = 0; i < 36; i++)
            //{
            //    double x = (double)i + 5;
            //    double y1 = 1.5 + Math.Sin((double)i * 0.2);
            //    double y2 = 3.0 * (1.5 + Math.Sin((double)i * 0.2));
            //    list1.Add(x, y1);
            //    list2.Add(x, y2);
            //}

            //// Generate a red curve with diamond
            //// symbols, and "Porsche" in the legend
            //LineItem myCurve = myPane.AddCurve("Signal",
            //    pl, Color.Blue, SymbolType.None);

            // Generate a blue curve with circle
            // symbols, and "Piper" in the legend
            /* LineItem myCurve2 = myPane.AddCurve("Piper",
                 list2, Color.Blue, SymbolType.Circle);*/

            zedGraphControl1.AxisChange();
        }

        #region  界面UI处理
         #region 菜单
        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            int ret = sEMG_open(1, Parameters.ip, Parameters.filterOption);
            openToolStripMenuItem.Enabled = false;
            //bool ret =true;
            if (ret != 0 )
            {
                
                channelNum = get_channel_num();
                if (channelNum != Parameters.usingChannelCount)
                {
                    MessageBox.Show("通道数配置错误");
                    sEMG_close();
                    openToolStripMenuItem.Enabled = true;
                    return;

                }
                sampleRate = get_AD_rate();
                get_dll_info(dllInfo);
                timer1.Start();
                toolStripStatusLabel1.Text = "状态: 打开";
                toolStripStatusLabel2.Text = string.Format("采样率: {0}K", sampleRate);
                toolStripStatusLabel3.Text = string.Format("总通道数: {0}", channelNum);
                CreateChannelList();
                configurationToolStripMenuItem.Enabled = false;
                closeToolStripMenuItem.Enabled = true;
                bStatus = true;

                Graph_Show();
                CreateButtons();

            }
            else
            {
                openToolStripMenuItem.Enabled = true;
                sEMG_close();
                MessageBox.Show("无法打开设备");
                bStatus = false;
            }
        }

        private void CreateButtons()
        {
            int btnNum = (int)(Math.Ceiling((double)channelNum / (double)Parameters.showChannelCount));
           
            for (int i = 0; i < btnNum; i++)
            {
                Button btn = new Button();
                int fromChn = i * Parameters.showChannelCount;
                int toChn = i * Parameters.showChannelCount + Parameters.showChannelCount - 1;
                toChn = (toChn < channelNum - 1) ? toChn : channelNum - 1;
                btn.Name = "btnChn_" + fromChn + "_" + toChn;
                btn.Text = fromChn + "-" + toChn;
                btn.Size = new Size(50, 23);
                btn.UseVisualStyleBackColor = true;
                btn.Click += new System.EventHandler(this.btnChnGrp_Click);
                flowLayoutPanel1.Controls.Add(btn);
            }
            Button btnClearList = new Button();
            btnClearList.Name = "btnClearList";
            btnClearList.Text = "Clear";
            btnClearList.Size = new Size(56, 23);
            btnClearList.UseVisualStyleBackColor = true;
            btnClearList.Click += new System.EventHandler(this.btnClearList_Click);
            flowLayoutPanel1.Controls.Add(btnClearList);
        }
        private void closeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            CloseDevice();
            toolStripStatusLabel1.Text = "状态:打开";
            timer1.Stop();
        }
        private void configurationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Setting settingfrm = new Setting();
            if (settingfrm.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                if (bStatus == true)
                {
                //    CloseDevice();
                }
                //Parameters.GetSettingXML();
            }
            Console.WriteLine("$");
        }

        private void actionMappingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ActionFrm actionfrm = new ActionFrm();
            if (actionfrm.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
               
                //Parameters.GetSettingXML();
            }
           
        }
        #endregion


        private void showVideo(byte[] src)
        {
            byte[] show = new byte[128];
            for (int j = 0; j < 128; ++j)
            {

                show[j] = src[(byte)(j % 16 * 8 + (7 - j / 16))];
            }
            Image<Gray, byte> img = new Image<Gray, byte>(16, 8, new Gray(100));
            img.Bytes = show;
            img = img.Resize(800, 400, Emgu.CV.CvEnum.INTER.CV_INTER_CUBIC);
            //Marshal.Copy(show, 0, intPtr, );

            pictureBox1.Image = img.ToBitmap();
        }

        private bool InitWritingFile()
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "txt files (*.txt)|*.txt";
            saveFileDialog.AddExtension = true;
            saveFileDialog.DefaultExt ="txt";
            saveFileDialog.CheckPathExists = true;
            if (saveFileDialog.ShowDialog() != DialogResult.OK)
            {
                MessageBox.Show("Pleae choose file or uncheck the recording","failed");
                return false;
            }
            
            try
            {          
                fileHelper = new FileHelper(saveFileDialog.FileName);
                fileHelper.WriteLine("Encoding:\tUTF-8");
                fileHelper.WriteLine("SampleRate:\t1000");
                fileHelper.WriteLine("Date:\t"+DateTime.Now.ToShortDateString());
                fileHelper.WriteLine("Time:\t" + DateTime.Now.ToShortTimeString());
                fileHelper.WriteLine("Voltage:\tmV");
                StringBuilder sb = new StringBuilder();
                sb.Append("Time(s)\t");
                for (int i=0; i < Parameters.usingChannelCount; i++)
                {
                    sb.Append("channel"+i.ToString()+"\t");
                }
                sb.Append("Action");
                fileHelper.WriteLine(sb.ToString());
                    return true;
            }
            catch (Exception exp)
            {
                MessageBox.Show("创建文件错误:\n" + exp.Message, "Failed");
                return false;
            }
        }
        private void CreateChannelList()
        {
            for (Byte i = 0; i < channelNum; i++)
            {
                channelView.Items.Add("Channel " + i.ToString());
            }
            channelView.Items[0].Selected = true;
        }

        private void btnGetData_Click(object sender, EventArgs e)
        {
            if (openToolStripMenuItem.Enabled == true)//还没连上就不要打开了
                return;
            sampleTime = 0;
            if (checkVideo.Checked)
            {
                zedGraphControl1.Visible = false;
                pictureBox1.Visible = true;
            }
            else
            {
                pictureBox1.Visible = false;
                zedGraphControl1.Visible = true;
               
            }
            if(checkRecord.Checked == true)
                if(!InitWritingFile()) return;
            btnGetData.Enabled = false;
            checkRecord.Enabled = false;
            btnStop.Enabled = true;
            myCallBack = new scallBack(this.GetData);
            set_data_notify(myCallBack);//设置回调函数
        }
        private void btnStop_Click(object sender, EventArgs e)
        {
            if (openToolStripMenuItem.Enabled == true)
            {
                MessageBox.Show("Please open the device first");
                return;
            }
            btnGetData.Enabled = true;
            checkRecord.Enabled = true;
            btnStop.Enabled = false;
            reset_data_notify();
            if (fileHelper != null)
            {
                fileHelper.Flush();//flush data to disk
                fileHelper.Close();
            }
        }
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            closing = true;//线程不安全
        }
        #endregion


        private void channelView_ItemChecked(object sender, ItemCheckedEventArgs e)
        {
            lock (pl)
            {
                if (Parameters.showChannelCount < channelView.CheckedIndices.Count)
                {
                    //select first 3
                    for (int i = 0; i < Parameters.showChannelCount; i++)
                    {
                        channelIndex[i] = channelView.CheckedIndices[i];
                        //TODO process
                        //var f = (from num in channelIndex
                        //         where num == i
                        //         select num).Count();
                        //clear exists channel data
                        //if(f == 0)
                        {
                            filters.filter_init(ref bw_filters[i]);//init filters
                            pl[i].Clear();
                            for (int j = 0; j < Parameters.duration; j++)
                            {
                                pl[i].Add(j, 0);
                            }

                        }
                        zedGraphControl1.MasterPane.PaneList[i].YAxis.Title.Text = channelView.CheckedItems[i].Text;
                    }
                    for (int i = Parameters.showChannelCount; i < channelView.CheckedIndices.Count; i++)
                    {
                        channelView.CheckedItems[i].Checked = false;
                    }
                }
                else
                {

                    for (int i = 0; i < channelView.CheckedIndices.Count; i++)
                    {
                        channelIndex[i] = channelView.CheckedIndices[i];
                        pl[i].Clear();
                        filters.filter_init(ref bw_filters[i]);//init filters
                        for (int j = 0; j < Parameters.duration; j++)
                        {
                            pl[i].Add(j, 0);
                        }
                        zedGraphControl1.MasterPane.PaneList[i].YAxis.Title.Text = channelView.CheckedItems[i].Text;
                    }
                    for (int i = channelView.CheckedIndices.Count; i < Parameters.showChannelCount; i++)
                    {
                        channelIndex[i] = -1;
                    }
                }
            }
            
        }

        private void sEMGFrm_Load(object sender, EventArgs e)
        {
            CreateActionButtons();
        }
        #region 其他的
        private void btnChnGrp_Click(object sender, EventArgs e)
        {
            Button currentBtn = (Button)sender;
            string[] btnText = currentBtn.Text.Split('-');
            int btnNum1 = int.Parse(btnText[0]);
            int btnNum2 = int.Parse(btnText[1]);
            clearListChecked();
            for (int i = btnNum1; i <= btnNum2; i++)
            {
                channelView.Items[i].Checked = true;
            }
            channelView.EnsureVisible(btnNum1);
            channelView.EnsureVisible(btnNum2);
        }

        private void btnClearList_Click(object sender, EventArgs e)
        {
            clearListChecked();
        }

        private void clearListChecked()
        {
            foreach (ListViewItem lvi in channelView.CheckedItems)
            {
                lvi.Checked = false;
            }
        }

        public bool OpenStatus
        {
            get
            {
                return bStatus;
            }

            set
            {
                bStatus = value;
            }
        }

        private void CloseDevice()
        {
            this.btnStop_Click(null, null);
            if (sEMG_close() != 0)
            {
                openToolStripMenuItem.Enabled = true;
                closeToolStripMenuItem.Enabled = false;
                configurationToolStripMenuItem.Enabled = true;
                channelView.Items.Clear();
                flowLayoutPanel1.Controls.Clear();
                bStatus = false;
            }
            else
            {
                MessageBox.Show("关闭设备失败");
            }
        }
        #endregion
        /// <summary>
        /// 每秒检查设备
        /// </summary>
        private void timer1_Tick(object sender, EventArgs e)
        {
            if (get_dev_stat() != 2)
            {
                timer1.Stop();
                closeToolStripMenuItem_Click(null, null);
                MessageBox.Show("设备已断开,请重启设备");
                Application.Exit();// get out
            }
        }

        private void CreateActionButtons()
        {
            foreach(DataRow  dr in Parameters.ActionGroup.Rows)
            {
                RadioButton btn = new RadioButton();
                btn.Text = dr["ActionName"].ToString();
                btn.Tag = dr["ActionValue"].ToString();
                btn.CheckedChanged += btn_CheckedChanged;
                btn.Size = new Size(50, 23);
                flowLayoutPanel2.Controls.Add(btn);
            }
        
        }

        void btn_CheckedChanged(object sender, EventArgs e)
        {
            RadioButton rb = sender as RadioButton;
            if(rb.Checked)
            {
                btntext = rb.Tag.ToString();
            }
        }
        private string btntext = "0";
   
        private void sEMGFrm_KeyDown(object sender, KeyEventArgs e)
        {
            selectedAction = btntext;
            progressBar1.Value = 100;
            
        }

        private void sEMGFrm_KeyUp(object sender, KeyEventArgs e)
        {
            selectedAction = "0";
            progressBar1.Value = 0;
        }

        private void analysisToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SysAnalysis sysanalysis = new SysAnalysis();
            sysanalysis.Show();
        }




    }
}
