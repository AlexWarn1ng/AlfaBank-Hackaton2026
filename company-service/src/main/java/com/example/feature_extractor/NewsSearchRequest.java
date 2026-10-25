package com.example.feature_extractor;

public record NewsSearchRequest(
        String query,
        Integer limit,
        Long excludeNewsId
) {}