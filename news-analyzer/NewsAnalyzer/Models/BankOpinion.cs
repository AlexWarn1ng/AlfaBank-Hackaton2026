namespace NewsAnalyzer.Models;

public sealed class BankOpinion
{
    public long Id { get; set; }
    public string Title { get; set; } = "";
    public string Text { get; set; } = "";

    public int? FinanceImportance { get; set; }
    public int? BusinessImportance { get; set; }
    public int? GlobalImportance { get; set; }
}