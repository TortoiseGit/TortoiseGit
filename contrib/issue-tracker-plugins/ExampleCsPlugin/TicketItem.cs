namespace ExampleCsPlugin
{
    public class TicketItem
    {
        private readonly int _ticketNumber;
        private readonly string _ticketSummary;

        public TicketItem(int ticketNumber, string ticketSummary)
        {
            _ticketNumber = ticketNumber;
            _ticketSummary = ticketSummary;
        }

        public int Number
        {
            get { return _ticketNumber; }
        }

        public string Summary
        {
            get { return _ticketSummary; }
        }
    }
}