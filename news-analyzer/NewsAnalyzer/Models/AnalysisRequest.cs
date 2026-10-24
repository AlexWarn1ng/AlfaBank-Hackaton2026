namespace NewsAnalyzer.Models;

public sealed record AnalysisRequest(
    Guid UserId,
    long NewsId
);