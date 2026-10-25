namespace NewsAnalyzer.Models;

public sealed class ValidationResult
{
    public bool IsValid { get; set; }

    public List<string> Issues { get; set; } = [];

    public List<string> MissingInformation { get; set; } = [];

    public List<string> SearchQueries { get; set; } = [];
}