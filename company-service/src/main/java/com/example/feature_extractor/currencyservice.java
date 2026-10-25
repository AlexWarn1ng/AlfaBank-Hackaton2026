package com.example.feature_extractor;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.Locale;
import java.util.Map;

@Service
public class currencyservice {

    private final HttpClient client = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(5)).build();

    private final ObjectMapper mapper = new ObjectMapper();

    public Map<String, Object> getRate(String from, String to) {
        from = from.toUpperCase(Locale.ROOT);
        to = to.toUpperCase(Locale.ROOT);

        if (!from.matches("[A-Z]{3}") || !to.matches("[A-Z]{3}")) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST, "Invalid currency code"
            );
        }

        if (from.equals(to)) {
            return Map.of("from", from, "to", to, "rate", 1);
        }

        String url = "https://api.frankfurter.dev/v2/rate/"
                + from + "/" + to;

        try {
            HttpRequest request = HttpRequest.newBuilder().uri(URI.create(url)).timeout(Duration.ofSeconds(10)).GET().build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 404 || response.statusCode() == 422) {
                throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Currency pair not found");
            }

            if (response.statusCode() != 200) {throw new ResponseStatusException(HttpStatus.BAD_GATEWAY, "Currency API error");
            }

            JsonNode json = mapper.readTree(response.body());

            return Map.of(
                    "from", from,
                    "to", to,
                    "rate", json.get("rate").decimalValue(),
                    "date", json.get("date").asText()
            );

        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new ResponseStatusException(HttpStatus.BAD_GATEWAY, "Request interrupted", e);
        } catch (Exception e) {
            throw new ResponseStatusException(HttpStatus.BAD_GATEWAY, "Failed to fetch exchange rate", e);
        }
    }
}