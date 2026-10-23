
package com.example.feature_extractor;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.pdfbox.Loader;
import org.apache.pdfbox.rendering.PDFRenderer;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Service;
import org.springframework.transaction.support.TransactionTemplate;

import javax.imageio.ImageIO;
import java.io.ByteArrayOutputStream;
import java.net.URI;
import java.net.http.*;
import java.time.Duration;
import java.util.*;

@Service
public class pdfprocessingservice {

    private final JdbcTemplate db;
    private final TransactionTemplate transaction;
    private final ObjectMapper mapper = new ObjectMapper();
    private final HttpClient http = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(60)).build();

    private final String apiKey = "sk-CUL4k-wnWuGQkkF5ZsZ91w";

    private static final String API =
            "https://changellenge.up.railway.app/v1/chat/completions";

    private static final String PROMPT = """
        Ответь как специалист по аналитике новостей.
        Проанализируй страницу и извлеки ключевые новости,
        идеи и события.

        Верни строго JSON-массив:
        [{"title":"Заголовок","text":"Описание",
          "finance":75,"business":60,"global":30}]

        Для каждой новости создавай отдельный объект.
        finance — значимость для финансовых аналитиков 0-100.
        business — значимость для бизнес-аналитиков 0-100.
        global — значимость для всего рынка 0-100.
        Все оценки целые. Не придумывай факты.
        Неразборчивую информацию пропускай.
        Если новостей нет, верни [].
        Возвращай только валидный JSON без Markdown. Не используй в своем ответе MARKDOWN форматирование.
        """;

    public pdfprocessingservice(
            JdbcTemplate db,
            TransactionTemplate transaction) {
        this.db = db;
        this.transaction = transaction;
    }

    public int process(byte[] pdf) throws Exception {
        if (pdf.length < 5 || !Arrays.equals(Arrays.copyOf(pdf, 5), "%PDF-".getBytes(java.nio.charset.StandardCharsets.US_ASCII))) {
            throw new IllegalArgumentException("Not a PDF");
        }

        List<JsonNode> news = new ArrayList<>();

        try (var document = Loader.loadPDF(pdf)) {
            PDFRenderer renderer = new PDFRenderer(document);

            for (int i = 0; i < document.getNumberOfPages(); i++) {
                long startbefore = System.nanoTime();
                var image = renderer.renderImageWithDPI(i, 100);

                ByteArrayOutputStream output = new ByteArrayOutputStream();

                if (!ImageIO.write(image, "png", output))
                    throw new IllegalStateException("PNG encoding failed");

                byte[] png = output.toByteArray();

                System.out.println("PNG size: " + png.length / 1024 + " KB");

                String base64 = Base64.getEncoder().encodeToString(png);

                System.out.println("Base64 size: " + base64.length() / 1024 + " KB");
                long startbeforeend = System.nanoTime();
                System.out.println("Preparations before zapros: " + (startbefore - startbeforeend) / 1_000_000_000.0 + " sec");
                JsonNode result = sendToAI(base64);

                if (!result.isArray())
                    throw new IllegalStateException("AI returned invalid JSON");

                for (JsonNode item : result) {
                    // Проверяем все поля до записи в БД
                    item.path("title").require().textValue();
                    item.path("text").require().textValue();

                    for (String field : List.of("finance", "business", "global")) {
                        JsonNode node = item.path(field);

                        if (!node.isIntegralNumber()) {
                            throw new IllegalStateException(
                                    "Missing or invalid field: " + field +
                                            ", item: " + item
                            );
                        }

                        int value = node.intValue();

                        if (value < 0 || value > 100) {
                            throw new IllegalStateException("Invalid " + field);
                        }
                    }

                    news.add(item);
                }

                System.out.println("Processed page " + (i + 1));
            }
        }

        // Записываем только после успешной обработки ВСЕХ страниц
        transaction.executeWithoutResult(status -> {
            for (JsonNode item : news) {
                db.update("""
                    INSERT INTO ai_features
                    (title, text, finance_importance,
                     business_importance, global_importance)
                    VALUES (?, ?, ?, ?, ?)
                    """,
                        item.get("title").asText(),
                        item.get("text").asText(),
                        item.get("finance").asInt(),
                        item.get("business").asInt(),
                        item.get("global").asInt()
                );
            }
        });
        System.out.println("EndOFinsert ");

        return news.size();
    }

    private JsonNode sendToAI(String base64) throws Exception {
        System.out.println("sending to ai");
        var body = Map.of(
                "model", "google/gemma-4-26b-a4b-it",
                "max_tokens", 6000,
                "messages", List.of(Map.of(
                        "role", "user",
                        "content", List.of(
                                Map.of("type", "text", "text", PROMPT),
                                Map.of("type", "image_url",
                                        "image_url", Map.of(
                                                "url", "data:image/png;base64," + base64))
                        )
                ))
        );

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(API))
                .timeout(Duration.ofSeconds(240))
                .header("Authorization", "Bearer " + apiKey)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(mapper.writeValueAsString(body)))
                .build();
        long start = System.nanoTime();
        HttpResponse<String> response = http.send(request, HttpResponse.BodyHandlers.ofString());
        long end = System.nanoTime();
        System.out.println("AI server response time: " + (end - start) / 1_000_000_000.0 + " sec");
        if (response.statusCode() / 100 != 2)
            throw new IllegalStateException("AI HTTP " + response.statusCode() + ": " + response.body());

        String content = mapper.readTree(response.body()).path("choices").get(0).path("message").path("content").asText();

        return mapper.readTree(content);
    }
}
