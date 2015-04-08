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
    public partial class ActionFrm : Form
    {
        BindingSource bs = new BindingSource();
        public ActionFrm()
        {
            InitializeComponent();
            bs.DataSource = Parameters.ActionGroup;
           // bs.Sort = "order asc";
            dgvAction.DataSource = bs;
            
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
           
            for(int i =0;i< dgvAction.Rows.Count;i++)
            {
                dgvAction.Rows[i].Cells["order"].Value = i;
            } 
            Parameters.writeActions();
            this.Close();
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.Close();
        }

        private void dgvAction_DataError(object sender, DataGridViewDataErrorEventArgs e)
        {
            if(e.Exception is ConstraintException)
            {
                MessageBox.Show("值不能重复");
                e.ThrowException = false;
            }
        }

    }
}
