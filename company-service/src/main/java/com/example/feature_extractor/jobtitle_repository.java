package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Optional;

public interface jobtitle_repository extends JpaRepository<jobtitle, Long> {

    Optional<jobtitle> findByPosition(String position);

    List<jobtitle> findByFinanceImportanceRate(String financeImportanceRate);

    List<jobtitle> findByBusinessImportanceRate(String businessImportanceRate);

}