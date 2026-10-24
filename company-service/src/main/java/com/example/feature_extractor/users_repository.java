package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import java.util.UUID;

public interface users_repository extends JpaRepository<users, UUID> {

}