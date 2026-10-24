namespace NewsAnalyzer.Models;

public sealed class NewsContext
{
    public long Id { get; set; }
    public string Title { get; set; } = "";
    public string Text { get; set; } = "";
}