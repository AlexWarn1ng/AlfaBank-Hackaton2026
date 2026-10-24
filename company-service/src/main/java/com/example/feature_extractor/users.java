package com.example.feature_extractor;

import jakarta.persistence.*;
import java.util.UUID;
import java.time.OffsetDateTime;

@Entity
@Table(name = "users")
public class users {

    @Id
    @Column(name = "id")
    private UUID id;

    @Column(name = "login")
    private String login;

    @Column(name = "password_hash")
    private String passwordHash;

    @Column(name = "f_uid")
    private String fGuid;

    @Column(name = "role")
    private String role;

    @Column(name = "created_at")
    private OffsetDateTime createdAt;

    @Column(name = "name")
    private String name;

    public users() {}

    public UUID getId() {
        return id;
    }

    public void setId(UUID id) {
        this.id = id;
    }

    public String getFGuid() {
        return fGuid;
    }

    public void setFGuid(String fGuid) {
        this.fGuid = fGuid;
    }

    public String getLogin() {
        return login;
    }

    public String getRole() {
        return role;
    }

    public String getName() {
        return name;
    }
}