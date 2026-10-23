package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;

public interface aifeatures_repository extends JpaRepository<aifeatures, Long> {

    List<aifeatures> findByTitle(String title);

    List<aifeatures> findByFinanceImportance(Integer financeImportance);

    List<aifeatures> findByBusinessImportance(Integer businessImportance);

    List<aifeatures> findByGlobalImportance(Integer globalImportance);

    //f.financeImportance > :financeVal OR f.businessImportance > :businessVal можно AND
    @Query("SELECT f.id FROM aifeatures f WHERE " +
            "(f.financeImportance > :financeVal OR f.businessImportance > :businessVal) " +
            "OR f.globalImportance > 80")
    List<Long> findMatchingNewsIds(@Param("financeVal") int financeVal, @Param("businessVal") int businessVal);

    List<aifeatures> findAllByIdIn(List<Long> ids);

}