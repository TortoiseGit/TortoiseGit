using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace ExampleCsPlugin
{
    partial class MyIssuesForm : Form
    {
        private readonly IEnumerable<TicketItem> _tickets;
        private readonly List<TicketItem> _ticketsAffected = new List<TicketItem>();

        public MyIssuesForm(IEnumerable<TicketItem> tickets)
        {
            InitializeComponent();
            _tickets = tickets;
        }

        public IEnumerable<TicketItem> TicketsFixed
        {
            get { return _ticketsAffected; }
        }

        private void MyIssuesForm_Load(object sender, EventArgs e)
        {
            listView1.Columns.Add("");
            listView1.Columns.Add("#");
            listView1.Columns.Add("Summary");

            foreach(TicketItem ticketItem in _tickets)
            {
                ListViewItem lvi = new ListViewItem();
                lvi.Text = "";
                lvi.SubItems.Add(ticketItem.Number.ToString());
                lvi.SubItems.Add(ticketItem.Summary);
                lvi.Tag = ticketItem;

                listView1.Items.Add(lvi);
            }

            listView1.Columns[0].Width = -1;
            listView1.Columns[1].Width = -1;
            listView1.Columns[2].Width = -1;
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            foreach (ListViewItem lvi in listView1.Items)
            {
                TicketItem ticketItem = lvi.Tag as TicketItem;
                if (ticketItem != null && lvi.Checked)
                    _ticketsAffected.Add(ticketItem);
            }
        }
    }
}
