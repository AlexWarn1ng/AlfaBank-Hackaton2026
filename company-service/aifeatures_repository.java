package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import java.util.List;

public interface aifeatures_repository extends JpaRepository<aifeatures, Long> {

    List<aifeatures> findByTitle(String title);

    List<aifeatures> findByFinanceImportance(Integer financeImportance);

    List<aifeatures> findByBusinessImportance(Integer businessImportance);

    List<aifeatures> findByGlobalImportance(Integer globalImportance);
}