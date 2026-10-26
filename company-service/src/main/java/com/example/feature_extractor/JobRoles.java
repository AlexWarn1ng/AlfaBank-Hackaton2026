package com.example.feature_extractor;

import java.util.Locale;

public final class JobRoles {
    private JobRoles() {}

    public static String normalize(String role) {
        if (role == null) return "";
        return switch (role.toLowerCase(Locale.ROOT)) {
            case "ceo" -> "CEO";
            case "cfo" -> "CFO";
            case "accountant", "chiefaccountant" -> "ChiefAccountant";
            case "treasurer", "headoftreasury" -> "HeadofTreasury";
            default -> role;
        };
    }
}
