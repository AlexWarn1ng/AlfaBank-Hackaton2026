using System.Text.Json;

namespace NewsAnalyzer.Models;

public sealed class AnalysisContext
{
    public required UserContext User { get; init; }
    public required NewsContext News { get; init; }
    public required List<JsonElement> Companies { get; init; }
}