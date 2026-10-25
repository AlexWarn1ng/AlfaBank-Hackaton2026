package com.example.feature_extractor;

import java.util.UUID;

public record UserRequest(
        UUID id,
        String login,
        String passwordHash,
        String fUid,
        String name,
        String role
) {}