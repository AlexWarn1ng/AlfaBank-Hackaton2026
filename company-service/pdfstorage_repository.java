package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import java.util.List;

public interface pdfstorage_repository extends JpaRepository<pdfstorage, Long> {

    List<pdfstorage> findByFilename(String filename);
}