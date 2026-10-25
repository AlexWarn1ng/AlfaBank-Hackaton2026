using System.Text.Json.Serialization;

namespace NewsAnalyzer.Models;

public sealed class AnalysisResult
{
    [JsonPropertyName("summary")]
    public string Summary { get; set; } = "";

    [JsonPropertyName("relevanceReason")]
    public string RelevanceReason { get; set; } = "";

    [JsonPropertyName("potentialRisks")]
    public List<string> PotentialRisks { get; set; } = [];

    [JsonPropertyName("potentialOpportunities")]
    public List<string> PotentialOpportunities { get; set; } = [];

    [JsonPropertyName("recommendedAction")]
    public string RecommendedAction { get; set; } = "";

    [JsonPropertyName("bankPerspective")]
    public BankPerspective? BankPerspective { get; set; }

    [JsonPropertyName("claims")]
    public List<AnalysisClaim> Claims { get; set; } = [];
}

public sealed class BankPerspective
{
    [JsonPropertyName("summary")]
    public string Summary { get; set; } = "";

    [JsonPropertyName("evidenceIds")]
    public List<string> EvidenceIds { get; set; } = [];
}

public sealed class AnalysisClaim
{
    [JsonPropertyName("statement")]
    public string Statement { get; set; } = "";

    // fact, inference, bank_opinion
    [JsonPropertyName("type")]
    public string Type { get; set; } = "";

    [JsonPropertyName("evidenceIds")]
    public List<string> EvidenceIds { get; set; } = [];
}
