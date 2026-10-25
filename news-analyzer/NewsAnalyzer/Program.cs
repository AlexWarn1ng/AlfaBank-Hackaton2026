using System.Text.Json;
using NewsAnalyzer.Clients;
using NewsAnalyzer.Models;
using NewsAnalyzer.Services;
using NewsAnalyzer.Agents;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddHttpClient<CompanyServiceClient>(client =>
{
    var url = builder.Configuration["COMPANY_SERVICE_URL"]
              ?? "http://localhost:8888";

    client.BaseAddress = new Uri(url.TrimEnd('/') + "/");
    client.Timeout = TimeSpan.FromSeconds(30);
});

builder.Services.AddHttpClient<AnalysisService>(client =>
{
    var url = builder.Configuration["AI_BASE_URL"]
              ?? "https://changellenge.up.railway.app/v1/";

    client.BaseAddress = new Uri(url.TrimEnd('/') + "/");
    client.Timeout = TimeSpan.FromSeconds(120);
});

builder.Services.AddHttpClient<AnalysisPlanner>(client =>
{
    var url = builder.Configuration["AI_BASE_URL"]
              ?? "https://changellenge.up.railway.app/v1/";

    client.BaseAddress = new Uri(url.TrimEnd('/') + "/");
    client.Timeout = TimeSpan.FromSeconds(120);
});

builder.Services.AddHttpClient<AnalysisValidator>(client =>
{
    var url = builder.Configuration["AI_BASE_URL"]
              ?? "https://changellenge.up.railway.app/v1/";

    client.BaseAddress = new Uri(url.TrimEnd('/') + "/");
    client.Timeout = TimeSpan.FromSeconds(120);
});

builder.Services.AddScoped<AgentOrchestrator>();

builder.Services.AddScoped<ContextBuilderService>();

builder.Services.AddScoped<EvidenceCatalogService>();
builder.Services.AddScoped<EvidenceReferenceValidator>();

var app = builder.Build();

app.MapGet("/", () => new
{
    service = "NewsAnalyzer",
    status = "running"
});

app.MapPost("/api/analysis", async (
    AnalysisRequest request,
    AgentOrchestrator agent,
    ILogger<Program> logger,
    EvidenceCatalogService evidenceCatalog,
    CancellationToken ct) =>
{
    if (request.UserId == Guid.Empty || request.NewsId <= 0)
    {
        return Results.BadRequest(new
        {
            error = "Некорректные идентификаторы."
        });
    }

    try
    {
        var result = await agent.ExecuteAsync(
            request.UserId,
            request.NewsId,
            ct);

        if (result is null)
        {
            return Results.NotFound(new
            {
                error = "Пользователь или новость не найдены."
            });
        }

        // Сохраняем прежний контракт с Flask.
        var catalog = evidenceCatalog.Build(result.Value.State);

        var referencedIds = result.Value.Analysis.Claims
            .Where(c => c.EvidenceIds is not null)
            .SelectMany(c => c.EvidenceIds)
            .Concat(result.Value.Analysis.BankPerspective?.EvidenceIds ?? [])
            .ToHashSet(StringComparer.Ordinal);

        var sources = catalog
            .Where(s => referencedIds.Contains(s.SourceId))
            .Select(s => new { s.SourceId, s.SourceType, s.Title })
            .ToList();

        var hasCompany = result.Value.State.Context.Companies.Count > 0;
        const string limitation = "Связанная компания не найдена. Анализ основан на указанной должности и известных данных человека; влияние на конкретную компанию оценить нельзя.";
        if (!hasCompany)
            result.Value.Analysis.RelevanceReason = limitation + " " + result.Value.Analysis.RelevanceReason;

        return Results.Ok(new
        {
            analysis = result.Value.Analysis,
            sources,
            hasCompany,
            contextNotice = hasCompany ? null : limitation
        });
    }
    catch (HttpRequestException ex)
    {
        logger.LogError(ex, "Ошибка внешнего сервиса");

        return Results.Problem(
            "Ошибка обращения к внешнему сервису.",
            statusCode: 502);
    }
    catch (JsonException ex)
    {
        logger.LogError(ex, "Некорректный JSON");

        return Results.Problem(
            "Внешний AI-сервис вернул некорректные данные.",
            statusCode: 502);
    }
    catch (InvalidOperationException ex)
    {
        logger.LogError(ex, "Ошибка конфигурации");

        return Results.Problem(
            "Ошибка конфигурации анализатора.",
            statusCode: 500);
    }
});

if (app.Environment.IsDevelopment())
{
    app.MapPost("/api/debug/search", async (
        string query,
        long? excludeNewsId,
        CompanyServiceClient client,
        CancellationToken ct) =>
    {
        var news = await client.SearchNewsAsync(
            query,
            5,
            excludeNewsId,
            ct);

        var opinions = await client.GetBankOpinionsAsync(ct);

        return Results.Ok(new
        {
            news.Results,
            BankOpinions = opinions
        });
    });
}

app.Run();
