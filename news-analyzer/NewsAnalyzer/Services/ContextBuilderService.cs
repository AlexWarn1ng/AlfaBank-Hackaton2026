using System.Text.Json;
using NewsAnalyzer.Clients;
using NewsAnalyzer.Models;

namespace NewsAnalyzer.Services;

public sealed class ContextBuilderService(
    CompanyServiceClient client)
{
    public async Task<AnalysisContext?> BuildAsync(
        Guid userId,
        long newsId,
        CancellationToken ct)
    {
        var user = await client.GetUserAsync(userId, ct);
        if (user is null)
            return null;

        var news = await client.GetNewsAsync(newsId, ct);
        if (news is null)
            return null;

        var companies = new List<JsonElement>();

        foreach (var uid in user.CompanyIds.Distinct())
        {
            var company = await client.GetCompanyAsync(uid, ct);

            if (company.HasValue)
                companies.Add(company.Value);
        }

        return new AnalysisContext
        {
            User = user,
            News = news,
            Companies = companies
        };
    }
}