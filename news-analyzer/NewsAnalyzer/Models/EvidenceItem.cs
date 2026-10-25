namespace NewsAnalyzer.Models;

public sealed class EvidenceItem
{
    public required string SourceId { get; init; }

    // news, company, bank_opinion
    public required string SourceType { get; init; }

    public required string Title { get; init; }

    // Исходные данные, доступные модели.
    public required string Content { get; init; }
}