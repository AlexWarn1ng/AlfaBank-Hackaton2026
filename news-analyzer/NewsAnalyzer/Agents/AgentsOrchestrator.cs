using System.Text.Json;
using NewsAnalyzer.Clients;
using NewsAnalyzer.Models;
using NewsAnalyzer.Services;

namespace NewsAnalyzer.Agents;

public sealed class AgentOrchestrator(
    ContextBuilderService contextBuilder,
    CompanyServiceClient companyClient,
    AnalysisPlanner planner,
    AnalysisService analysisService,
    AnalysisValidator validator,
    EvidenceCatalogService evidenceCatalog,
    EvidenceReferenceValidator referenceValidator,
    ILogger<AgentOrchestrator> logger)
{
    private const int MaxCorrections = 1;
    private const int MaxSearchResults = 5;

    public async Task<(AnalysisResult Analysis, AnalysisState State)?>
        ExecuteAsync(
            Guid userId,
            long newsId,
            CancellationToken ct)
    {
        // Этап 1. Получаем исходный контекст.
        var context = await contextBuilder.BuildAsync(
            userId, newsId, ct);

        if (context is null)
            return null;

        var state = new AnalysisState
        {
            Context = context
        };

        state.ExecutionLog.Add("Контекст собран.");

        // Этап 2. Первичное планирование.
        List<string> initialQueries;

        try
        {
            initialQueries = await planner.CreatePlanAsync(
                context, ct);
        }
        catch (JsonException ex)
        {
            logger.LogWarning(
                ex,
                "Planner failed. Continuing without additional search.");

            initialQueries = [];
        }

        logger.LogInformation(
            "Planner generated {Count} queries",
            initialQueries.Count);

        await ExecuteSearchesAsync(
            state, initialQueries, ct);

        // Этап 3. Загружаем банковскую аналитику.
        var opinions = await companyClient.GetBankOpinionsAsync(ct);

        state.BankOpinions.AddRange(opinions.Take(5));

        state.ExecutionLog.Add(
            $"Загружено банковских материалов: " +
            $"{state.BankOpinions.Count}");
    
        // Этап 4. Создаём первоначальный анализ.
        var analysis = await GenerateAnalysisAsync(
            state, null, ct);

        // Этап 5. Проверяем результат.
        var validation = await ValidateAnalysisAsync(
            state, analysis, ct);

        state.ExecutionLog.Add(
            $"Первичная проверка: isValid={validation.IsValid}");

        if (validation.IsValid)
            return (analysis, state);

        // Этап 6. Ограниченный цикл корректировки.
        for (int attempt = 0; attempt < MaxCorrections; attempt++)
        {
            logger.LogWarning(
                "Analysis validation failed, correction attempt {Attempt}",
                attempt + 1);

            state.ExecutionLog.Add(
                $"Корректировка №{attempt + 1}: " +
                string.Join("; ", validation.Issues));

            // Получаем дополнительные данные.
            var newResults = await ExecuteSearchesAsync(
                state,
                validation.SearchQueries,
                ct);

            state.ExecutionLog.Add(
                $"Найдено новых материалов: {newResults}");

            // Передаём замечания валидатора в повторный анализ.
            analysis = await GenerateAnalysisAsync(
                state,
                validation,
                ct);

            // Повторно проверяем исправленный результат.
            validation = await ValidateAnalysisAsync(
                state,
                analysis,
                ct);

            state.ExecutionLog.Add(
                $"Повторная проверка: isValid={validation.IsValid}");

            if (validation.IsValid)
                return (analysis, state);
        }

        // Ни одна попытка не дала обоснованного результата.
        logger.LogWarning(
            "Analysis could not be validated. Issues: {Issues}",
            string.Join("; ", validation.Issues));

        return (CreateInsufficientDataFallback(state), state);
    }

    private async Task<int> ExecuteSearchesAsync(
        AnalysisState state,
        IEnumerable<string> queries,
        CancellationToken ct)
    {
        var added = 0;

        foreach (var query in queries
                     .Where(q => !string.IsNullOrWhiteSpace(q))
                     .Take(2))
        {
            var normalized = query.Trim();

            if (normalized.Length > 120)
                continue;

            // Не выполняем одинаковые запросы повторно.
            if (state.SearchQueries.Contains(
                    normalized,
                    StringComparer.OrdinalIgnoreCase))
                continue;

            state.SearchQueries.Add(normalized);

            var result = await companyClient.SearchNewsAsync(
                normalized,
                MaxSearchResults,
                state.Context.News.Id,
                ct);

            var before = state.RetrievedNews.Count;

            state.AddNews(result.Results);

            var newCount = state.RetrievedNews.Count - before;
            added += newCount;

            state.ExecutionLog.Add(
                $"Поиск: {normalized}; новых записей: {newCount}");

            logger.LogInformation(
                "News search completed: {NewCount} new records",
                newCount);
        }

        return added;
    }

    private Task<AnalysisResult> GenerateAnalysisAsync(
        AnalysisState state,
        ValidationResult? feedback,
        CancellationToken ct)
    {
        // LLM получает именно те источники с ID, которые впоследствии проверяются.
        var sources = evidenceCatalog.Build(state);

        return analysisService.AnalyzeAsync(
            new
            {
                User = state.Context.User,
                EvidenceSources = sources,
                ValidationFeedback = feedback is null
                    ? null
                    : new
                    {
                        feedback.Issues,
                        feedback.MissingInformation
                    }
            },
            ct);
    }

    private async Task<ValidationResult> ValidateAnalysisAsync(
        AnalysisState state,
        AnalysisResult analysis,
        CancellationToken ct)
    {
        // Обе проверки используют одну и ту же сериализованную версию результата.
        var analysisJson = JsonSerializer.SerializeToElement(
            analysis, new JsonSerializerOptions(JsonSerializerDefaults.Web));
        var sources = evidenceCatalog.Build(state);
        var referenceErrors = referenceValidator.Validate(analysisJson, sources);

        if (referenceErrors.Count > 0)
        {
            state.ExecutionLog.Add(
                "Ошибки ссылок на источники: " +
                string.Join("; ", referenceErrors));

            return new ValidationResult
            {
                IsValid = false,
                Issues = referenceErrors,
                MissingInformation = [],
                SearchQueries = []
            };
        }

        return await validator.ValidateAsync(state, analysisJson, ct);
    }

        private static AnalysisResult CreateInsufficientDataFallback(
        AnalysisState state)
    {
        var opinions = state.BankOpinions
            .Where(opinion => !string.IsNullOrWhiteSpace(opinion.Text))
            .ToList();

        return new AnalysisResult
        {
            InsufficientData = true,
            Summary = "Недостаточно подтверждённых данных для персонализированного анализа.",
            RelevanceReason =
                "Не удалось собрать достаточно надёжный контекст, чтобы дать вывод о влиянии события на конкретный бизнес. Ниже показаны доступные материалы Альфа-Банка.",
            PotentialRisks = [],
            PotentialOpportunities = [],
            RecommendedAction = "",
            Claims = [],
            BankPerspective = opinions.Count == 0
                ? null
                : new BankPerspective
                {
                    Summary = string.Join(
                        "\n\n",
                        opinions.Select(opinion =>
                            $"{opinion.Title}: {opinion.Text}"
                        )
                    ),
                    EvidenceIds = opinions
                        .Select(opinion => $"bank:{opinion.Id}")
                        .ToList()
                }
        };
    }
}
