using System.Text.Json.Serialization;

namespace NewsAnalyzer.Models;

public sealed class UserContext
{
    public Guid UserId { get; set; }
    public string? FId { get; set; }
    public string? Position { get; set; }
    public string? City { get; set; }
    public string? Region { get; set; }

    public List<string> CompanyIds { get; set; } = [];

    public string? FinanceImportanceRate { get; set; }
    public string? BusinessImportanceRate { get; set; }
}