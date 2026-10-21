
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

var builder = WebApplication.CreateBuilder(args);

// Регистрируем HTTP-клиент для запросов к нейросети.
builder.Services.AddHttpClient();

var app = builder.Build();

// Проверка, что микросервис запущен.
app.MapGet("/", () => new
{
    service = "NewsAnalyzer",
    status = "running"
});

// Анализ релевантности новости для конкретного пользователя.
app.MapPost("/api/analysis/relevance", async (
    RelevanceRequest request,
    IHttpClientFactory httpClientFactory,
    IConfiguration configuration) =>
{
    // Проверяем входные данные.
    if (string.IsNullOrWhiteSpace(request.NewsTitle) ||
        string.IsNullOrWhiteSpace(request.NewsText))
    {
        return Results.BadRequest(new
        {
            error = "Название новости и её текст обязательны."
        });
    }

    // Ключ храним в переменной окружения, а не в исходном коде.
    var apiKey = configuration["AI_API_KEY"];

    if (string.IsNullOrWhiteSpace(apiKey))
    {
        return Results.Problem(
            "Не настроен ключ AI_API_KEY.",
            statusCode: 500);
    }

    var client = httpClientFactory.CreateClient();

    client.BaseAddress =
        new Uri("https://changellenge.up.railway.app/v1/");

    client.DefaultRequestHeaders.Authorization =
        new AuthenticationHeaderValue("Bearer", apiKey);

    var prompt = $$$"""
        Ты — аналитик бизнес-рисков и возможностей.
        Оцени, насколько новость важна для конкретного руководителя компании.

        ПРОФИЛЬ ПОЛЬЗОВАТЕЛЯ:
        Должность: {{{request.UserRole}}}
        Компания: {{{request.CompanyName}}}
        Отрасль: {{{request.Industry}}}
        Бизнес-цели: {{{request.BusinessGoals}}}
        Интересы и приоритеты: {{{request.Interests}}}

        НОВОСТЬ:
        Заголовок: {{{request.NewsTitle}}}
        Текст: {{{request.NewsText}}}

        Оцени релевантность новости именно для этого пользователя,
        а не её общую важность.

        Верни только JSON без Markdown и пояснений за его пределами:
        {{
          ""relevanceScore"": 0,
          ""summary"": ""Краткое содержание события"",
          ""relevanceReason"": ""Почему новость важна или не важна"",
          ""potentialRisks"": [""Возможный риск""],
          ""potentialOpportunities"": [""Возможность""],
          ""recommendedAction"": ""Что стоит сделать руководителю""
        }}

        relevanceScore — целое число от 0 до 100.
        Не выдумывай факты, которых нет в новости.
        Если данных недостаточно, явно укажи это.
        """;

    var payload = new
    {
        model = "deepseek/deepseek-v4.1-flash",
        messages = new[]
        {
            new
            {
                role = "system",
                content = "Ты выполняешь бизнес-анализ новостей."
            },
            new
            {
                role = "user",
                content = prompt
            }
        },
        temperature = 0.2
    };

    try
    {
        var json = JsonSerializer.Serialize(payload);

        using var content = new StringContent(
            json,
            Encoding.UTF8,
            "application/json");

        using var response = await client.PostAsync(
            "chat/completions",
            content);

        var responseBody = await response.Content.ReadAsStringAsync();

        if (!response.IsSuccessStatusCode)
        {
            return Results.Problem(
                title: "Ошибка при обращении к AI API",
                detail: $"HTTP {(int)response.StatusCode}: {responseBody}",
                statusCode: 502);
        }

        using var document = JsonDocument.Parse(responseBody);

        var analysis = document.RootElement
            .GetProperty("choices")[0]
            .GetProperty("message")
            .GetProperty("content")
            .GetString();

        if (string.IsNullOrWhiteSpace(analysis))
        {
            return Results.Problem(
                "Нейросеть вернула пустой ответ.",
                statusCode: 502);
        }

        // Пока возвращаем JSON-ответ нейросети как текст.
        // Позже будем преобразовывать его в типизированный объект.
        return Results.Ok(new
        {
            newsTitle = request.NewsTitle,
            analysis
        });
    }
    catch (HttpRequestException)
    {
        return Results.Problem(
            "Не удалось подключиться к AI API.",
            statusCode: 502);
    }
    catch (JsonException)
    {
        return Results.Problem(
            "Не удалось разобрать JSON-ответ AI API.",
            statusCode: 502);
    }
});

app.Run();

public class RelevanceRequest
{
    public string NewsTitle { get; set; } = "";
    public string NewsText { get; set; } = "";

    public string UserRole { get; set; } = "";
    public string CompanyName { get; set; } = "";
    public string Industry { get; set; } = "";
    public string BusinessGoals { get; set; } = "";
    public string Interests { get; set; } = "";
}
