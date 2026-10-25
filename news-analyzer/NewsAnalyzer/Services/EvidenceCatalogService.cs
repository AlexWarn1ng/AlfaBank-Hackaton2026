using System.Text.Json;
using NewsAnalyzer.Models;

namespace NewsAnalyzer.Services;

public sealed class EvidenceCatalogService
{
    public List<EvidenceItem> Build(AnalysisState state)
    {
        var sources = new List<EvidenceItem>();
        var context = state.Context;

        // Основная анализируемая новость.
        sources.Add(new EvidenceItem
        {
            SourceId = $"news:{context.News.Id}",
            SourceType = "news",
            Title = context.News.Title,
            Content = context.News.Text
        });

        // Дополнительные новости, полученные через RAG.
        foreach (var news in state.RetrievedNews)
        {
            sources.Add(new EvidenceItem
            {
                SourceId = $"news:{news.Id}",
                SourceType = "news",
                Title = news.Title,
                Content = news.Text
            });
        }

        // Компании.
        foreach (var company in context.Companies)
        {
            if (!company.TryGetProperty(
                    "uid", out var uidProperty))
                continue;

            var uid = uidProperty.GetString();

            if (string.IsNullOrWhiteSpace(uid))
                continue;

            sources.Add(new EvidenceItem
            {
                SourceId = $"company:{uid}",
                SourceType = "company",
                Title = $"Профиль компании {uid}",
                Content = company.GetRawText()
            });
        }

        // Экспертные материалы банка.
        foreach (var opinion in state.BankOpinions)
        {
            sources.Add(new EvidenceItem
            {
                SourceId = $"bank:{opinion.Id}",
                SourceType = "bank_opinion",
                Title = opinion.Title,
                Content = opinion.Text
            });
        }

        return sources
            .GroupBy(s => s.SourceId, StringComparer.Ordinal)
            .Select(g => g.First())
            .ToList();
    }
}