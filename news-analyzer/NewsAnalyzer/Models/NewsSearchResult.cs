namespace NewsAnalyzer.Models;

public sealed class NewsSearchResult
{
    public List<NewsSearchItem> Results { get; set; } = [];
}

public sealed class NewsSearchItem
{
    public long Id { get; set; }
    public string Title { get; set; } = "";
    public string Text { get; set; } = "";
    public double Score { get; set; }
}