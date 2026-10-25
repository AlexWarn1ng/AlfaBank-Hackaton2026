using System.Net;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using NewsAnalyzer.Models;

namespace NewsAnalyzer.Clients;

public sealed class CompanyServiceClient(HttpClient http)
{
    public async Task<UserContext?> GetUserAsync(
        Guid userId,
        CancellationToken ct)
    {
        using var response = await http.GetAsync(
            $"api/analysis-context/users/{userId}", ct);

        if (response.StatusCode == HttpStatusCode.NotFound)
            return null;

        response.EnsureSuccessStatusCode();

        return await response.Content.ReadFromJsonAsync<UserContext>(
            cancellationToken: ct);
    }

    public async Task<JsonElement?> GetCompanyAsync(
        string uid,
        CancellationToken ct)
    {
        using var response = await http.GetAsync(
            $"api/company/{Uri.EscapeDataString(uid)}", ct);

        if (response.StatusCode == HttpStatusCode.NotFound)
            return null;

        response.EnsureSuccessStatusCode();

        return await response.Content.ReadFromJsonAsync<JsonElement>(
            cancellationToken: ct);
    }

    public async Task<NewsContext?> GetNewsAsync(
        long newsId,
        CancellationToken ct)
    {
        using var content = new StringContent(
            newsId.ToString(),
            Encoding.UTF8,
            "text/plain");

        using var response = await http.PostAsync(
            "api/news/get", content, ct);

        response.EnsureSuccessStatusCode();

        var result = await response.Content
            .ReadFromJsonAsync<NewsListResponse>(
                cancellationToken: ct);

        return result?.News.FirstOrDefault(n => n.Id == newsId);
    }

    private sealed class NewsListResponse
    {
        public List<NewsContext> News { get; set; } = [];
        public List<long> NotFoundIds { get; set; } = [];
    }
    
    public async Task<NewsSearchResult> SearchNewsAsync(
        string query,
        int limit,
        long? excludeNewsId,
        CancellationToken ct)
    {
        if (string.IsNullOrWhiteSpace(query))
            throw new ArgumentException(
                "Поисковый запрос не может быть пустым.",
                nameof(query));

        if (limit is < 1 or > 10)
            throw new ArgumentOutOfRangeException(nameof(limit));

        var request = new
        {
            query,
            limit,
            excludeNewsId
        };

        using var response = await http.PostAsJsonAsync(
            "api/news/search",
            request,
            ct);

        response.EnsureSuccessStatusCode();

        return await response.Content
                   .ReadFromJsonAsync<NewsSearchResult>(
                       cancellationToken: ct)
               ?? throw new JsonException(
                   "Java вернула пустой результат поиска.");
    }

    public async Task<List<BankOpinion>> GetBankOpinionsAsync(
        CancellationToken ct)
    {
        return await http.GetFromJsonAsync<List<BankOpinion>>(
                   "api/opinions/get",
                   ct)
               ?? throw new JsonException(
                   "Java вернула пустой ответ с аналитикой банка.");
    }
}