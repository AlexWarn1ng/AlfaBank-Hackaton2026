using System.Text.Json;
using NewsAnalyzer.Clients;
using NewsAnalyzer.Models;
using NewsAnalyzer.Services;

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

builder.Services.AddScoped<ContextBuilderService>();

var app = builder.Build();

app.MapGet("/", () => new
{
    service = "NewsAnalyzer",
    status = "running"
});

app.MapPost("/api/analysis", async (
    AnalysisRequest request,
    ContextBuilderService contextBuilder,
    AnalysisService analysisService,
    ILogger<Program> logger,
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
        var context = await contextBuilder.BuildAsync(
            request.UserId,
            request.NewsId,
            ct);

        if (context is null)
        {
            return Results.NotFound(new
            {
                error = "Пользователь или новость не найдены."
            });
        }

        var analysis = await analysisService.AnalyzeAsync(
            context,
            ct);

        return Results.Ok(new
        {
            analysis
        });
    }
    catch (HttpRequestException ex)
    {
        logger.LogError(ex, "Ошибка HTTP при анализе новости");

        return Results.Problem(
            "Ошибка обращения к внешнему сервису.",
            statusCode: 502);
    }
    catch (JsonException ex)
    {
        logger.LogError(ex, "Некорректный JSON от AI API");

        return Results.Problem(
            "ИИ вернул некорректный JSON.",
            statusCode: 502);
    }
    catch (InvalidOperationException ex)
    {
        logger.LogError(ex, "Ошибка конфигурации анализатора");

        return Results.Problem(
            "Ошибка конфигурации анализатора.",
            statusCode: 500);
    }
});

app.Run();
