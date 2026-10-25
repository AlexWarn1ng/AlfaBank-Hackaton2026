package com.example.feature_extractor;

import org.springframework.http.HttpStatus;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

import java.util.List;
import java.util.Map;

@Service
public class newssearchservice {

    private final JdbcTemplate jdbcTemplate;

    public newssearchservice(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    public Map<String, Object> search(
            String query,
            Integer limit,
            Long excludeNewsId
    ) {

        if (query == null || query.isBlank()) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST,
                    "Query cannot be empty"
            );
        }

        if (limit == null) {
            limit = 5;
        }

        if (limit < 1 || limit > 10) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST,
                    "Limit must be between 1 and 10"
            );
        }

        String sql = """
                WITH search_query AS (
                    SELECT websearch_to_tsquery('russian', ?) AS q
                ),
                documents AS (
                    SELECT
                        id,
                        title,
                        text,
                        (
                            setweight(
                                to_tsvector('russian', coalesce(title, '')),
                                'A'
                            )
                            ||
                            setweight(
                                to_tsvector('russian', coalesce(text, '')),
                                'B'
                            )
                        ) AS document_vector
                    FROM ai_features
                )
                SELECT
                    d.id,
                    d.title,
                    d.text,
                    ts_rank_cd(d.document_vector, s.q) AS score
                FROM documents d
                CROSS JOIN search_query s
                WHERE d.document_vector @@ s.q
                  AND (? IS NULL OR d.id <> ?)
                ORDER BY score DESC, d.id DESC
                LIMIT ?
                """;

        List<Map<String, Object>> results = jdbcTemplate.queryForList(
                sql,
                query.trim(),
                excludeNewsId,
                excludeNewsId,
                limit
        );

        return Map.of("results", results);
    }
}