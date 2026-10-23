package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import java.util.List;

public interface companies_repository
        extends JpaRepository<companies, String> {

    List<companies> findByCity(String city);

    List<companies> findByRegion(String region);

    List<companies> findByOrgType(String orgType);

    List<companies> findByMainokvedCode(String mainokvedCode);

    List<companies> findByGroupId(String groupId);
}