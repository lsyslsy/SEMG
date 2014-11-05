using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;


namespace SEMG.UIL
{
    public partial class Setting : Form
    {

        string regex_ip = "^((0{0,2}[0-9]|0?[0-9]{2}|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}(0{0,2}[0-9]|0?[0" +
    "-9]{2}|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$";
        string regex_duration = @"^[1-9]((\d{0,1}00)|0000)$";
        string regex_channelCount = @"^[1-8]$";
        public Setting()
        {
            InitializeComponent();
            txtIP.Text = Parameters.ip;
            txtDuration.Text = Parameters.duration.ToString();
            txtShowChannel.Text = Parameters.showChannelCount.ToString();
            txtScaling.Text =  Parameters.scaling.ToString();
            txtAxis.Text = Parameters.AxisScale.ToString();
            txtTotal.Text = Parameters.totalChannelCount.ToString();
            txtUsing.Text = Parameters.usingChannelCount.ToString();
            int H = Parameters.filterOption & 0xff00;
            int L = Parameters.filterOption & 0x00ff;
            radioButton2.Checked = (L == (int)filter_selector.FILTER_BUTTERWORTH) ? true : false;
            radioButton3.Checked = (L == (int)filter_selector.FILTER_RLS) ? true : false;
            radioButton4.Checked = (L == (int)filter_selector.FILTER_FOURIER) ? true : false;
            if (!(radioButton2.Checked | radioButton3.Checked | radioButton4.Checked))
                radioButton1.Checked = true;
            checkBox1.Checked = (H == (int)filter_selector.FILTER_BASELINE_YES) ? true : false;

        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.Close();
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            bool err = false;
            if (!System.Text.RegularExpressions.Regex.IsMatch(txtIP.Text.Trim(), regex_ip))
            {
                errorProvider1.SetError(txtIP, "not a valid ip");
                err = true;
            }
            if (!System.Text.RegularExpressions.Regex.IsMatch(txtDuration.Text.Trim(), regex_duration))
            {
                errorProvider1.SetError(txtDuration, "not a valid duration");
                err = true;
            }
            if (!System.Text.RegularExpressions.Regex.IsMatch(txtShowChannel.Text.Trim(), regex_channelCount))
            {
                errorProvider1.SetError(txtShowChannel, "not a valid channelCount");
                err = true;
            }

            int i;
            if (int.TryParse(txtUsing.Text, out i) && i > 0 && i <= 128)
                Parameters.usingChannelCount = i;
            else
            {
                errorProvider1.SetError(txtUsing, "格式错误,1-128");
                err = true;
            }
            double f;
            if (double.TryParse(txtScaling.Text, out f))
                Parameters.scaling = f;
            else
            {
                errorProvider1.SetError(txtScaling, "格式错误");
                err = true;
            }
            if (double.TryParse(txtAxis.Text, out f))
                Parameters.AxisScale = f;
            else
            {
                errorProvider1.SetError(txtAxis, "格式错误");
                err = true;
            }
            int filter_option;


            if (radioButton2.Checked)
            {
                filter_option = (int)filter_selector.FILTER_BUTTERWORTH;
            }
            else if (radioButton3.Checked)
            {
                filter_option = (int)filter_selector.FILTER_RLS;
            }
            else if (radioButton4.Checked)
            {
                filter_option = (int)filter_selector.FILTER_FOURIER;
            }
            else
            {
                filter_option = (int)filter_selector.FILTER_NONE;

            }

            if (checkBox1.Checked)
            {
                filter_option = filter_option + (int)filter_selector.FILTER_BASELINE_YES;
            }
            else
                filter_option = filter_option + (int)filter_selector.FILTER_BASELINE_NONE;
            Parameters.filterOption = filter_option;
            if (err == true)
            {
                return;
            }
            Parameters.ip = txtIP.Text.Trim();
            Parameters.duration = int.Parse(txtDuration.Text.Trim());
            Parameters.showChannelCount = int.Parse(txtShowChannel.Text.Trim());
            Parameters.writeSetting();
            MessageBox.Show("参数保存成功");
            this.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.Close();
        }
    }
}
