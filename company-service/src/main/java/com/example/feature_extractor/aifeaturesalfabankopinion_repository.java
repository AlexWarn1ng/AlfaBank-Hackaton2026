package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface aifeaturesalfabankopinion_repository extends JpaRepository<aifeaturesalfabankopinion, Long> {

    List<aifeaturesalfabankopinion> findTop20ByOrderByIdAsc();

}
