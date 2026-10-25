using System.Text.Json;

namespace NewsAnalyzer.Models;

public sealed class AnalysisState
{
    public required AnalysisContext Context { get; init; }

    public List<NewsSearchItem> RetrievedNews { get; } = [];

    public List<BankOpinion> BankOpinions { get; } = [];

    public List<string> SearchQueries { get; } = [];

    public List<string> ExecutionLog { get; } = [];

    public void AddNews(IEnumerable<NewsSearchItem> news)
    {
        var knownIds = RetrievedNews
            .Select(n => n.Id)
            .ToHashSet();

        knownIds.Add(Context.News.Id);

        foreach (var item in news)
        {
            if (knownIds.Add(item.Id))
                RetrievedNews.Add(item);
        }
    }
}