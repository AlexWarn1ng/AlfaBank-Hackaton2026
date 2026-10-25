using System.Text.Json;
using NewsAnalyzer.Models;

namespace NewsAnalyzer.Services;

public sealed class EvidenceReferenceValidator
{
    private static readonly HashSet<string> AllowedTypes =
        ["fact", "inference", "bank_opinion"];

    public List<string> Validate(
        JsonElement analysis,
        IReadOnlyList<EvidenceItem> sources)
    {
        var errors = new List<string>();

        if (!analysis.TryGetProperty("claims", out var claims) ||
            claims.ValueKind != JsonValueKind.Array)
        {
            errors.Add("Отсутствует массив claims.");
            return errors;
        }

        if (claims.GetArrayLength() == 0)
        {
            errors.Add("Массив claims не должен быть пустым.");
            return errors;
        }

        var byId = sources.ToDictionary(
            s => s.SourceId,
            StringComparer.Ordinal);

        foreach (var claim in claims.EnumerateArray())
        {
            if (claim.ValueKind != JsonValueKind.Object)
            {
                errors.Add("Некорректный объект claims.");
                continue;
            }

            if (!claim.TryGetProperty("statement", out var statement) ||
                statement.ValueKind != JsonValueKind.String ||
                string.IsNullOrWhiteSpace(statement.GetString()))
            {
                errors.Add("У вывода отсутствует statement.");
            }

            if (!claim.TryGetProperty("type", out var typeNode) ||
                typeNode.ValueKind != JsonValueKind.String ||
                !AllowedTypes.Contains(typeNode.GetString() ?? ""))
            {
                errors.Add("Неизвестный тип вывода.");
                continue;
            }

            var type = typeNode.GetString();

            if (!claim.TryGetProperty(
                    "evidenceIds", out var evidenceIds) ||
                evidenceIds.ValueKind != JsonValueKind.Array ||
                evidenceIds.GetArrayLength() == 0)
            {
                errors.Add("У вывода отсутствуют источники.");
                continue;
            }

            var hasBankSource = false;

            foreach (var idNode in evidenceIds.EnumerateArray())
            {
                if (idNode.ValueKind != JsonValueKind.String)
                {
                    errors.Add("Некорректный ID источника.");
                    continue;
                }

                var id = idNode.GetString() ?? "";

                if (!byId.TryGetValue(id, out var source))
                {
                    errors.Add($"Неизвестный источник: {id}");
                    continue;
                }

                if (source.SourceType == "bank_opinion")
                    hasBankSource = true;
            }

            if (type == "bank_opinion" && !hasBankSource)
            {
                errors.Add(
                    "Мнение банка не подтверждено банковским источником.");
            }
        }

        // bankPerspective — самостоятельный блок с обязательными банковскими ссылками.
        if (analysis.TryGetProperty("bankPerspective", out var perspective) &&
            perspective.ValueKind != JsonValueKind.Null)
        {
            if (perspective.ValueKind != JsonValueKind.Object ||
                !perspective.TryGetProperty("summary", out var summary) ||
                summary.ValueKind != JsonValueKind.String ||
                string.IsNullOrWhiteSpace(summary.GetString()) ||
                !perspective.TryGetProperty("evidenceIds", out var ids) ||
                ids.ValueKind != JsonValueKind.Array || ids.GetArrayLength() == 0)
            {
                errors.Add("Некорректный bankPerspective.");
            }
            else
            {
                foreach (var id in ids.EnumerateArray())
                {
                    if (id.ValueKind != JsonValueKind.String ||
                        !byId.TryGetValue(id.GetString() ?? "", out var source) ||
                        source.SourceType != "bank_opinion")
                        errors.Add("bankPerspective должен ссылаться на существующую аналитику банка.");
                }
            }
        }

        return errors;
    }
}