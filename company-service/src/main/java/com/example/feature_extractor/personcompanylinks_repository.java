package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Map;

public interface personcompanylinks_repository extends JpaRepository<personcompanylinks, Long> {

    List<personcompanylinks> findBySyntheticF(String syntheticF);

    List<personcompanylinks> findBySyntheticU(String syntheticU);

    List<personcompanylinks> findByCityName(String cityName);

    List<personcompanylinks> findByRcName(String rcName);

    @Query(value = """
    SELECT DISTINCT synthetic_u
    FROM person_company_links
    WHERE synthetic_f = :fid
      AND synthetic_u IS NOT NULL
    """, nativeQuery = true)
    List<String> getCompanyIdsByPerson(@Param("fid") String fid);

    @Query(value = """
    WITH persons AS (
        SELECT DISTINCT synthetic_f
        FROM person_company_links
        WHERE synthetic_u = :uid
          AND synthetic_f IS NOT NULL
    ),
    related AS (
        SELECT DISTINCT
            p.synthetic_f,
            p.synthetic_u
        FROM person_company_links p
        JOIN persons x
            ON p.synthetic_f = x.synthetic_f
        WHERE p.synthetic_u <> :uid
          AND p.synthetic_u IS NOT NULL
    )
    SELECT
        (SELECT COUNT(*) FROM persons)
            AS "relatedPersonsCount",

        (SELECT COUNT(DISTINCT synthetic_u)
         FROM related)
            AS "relatedCompaniesCount",

        (SELECT COUNT(DISTINCT synthetic_f)
         FROM related)
            AS "sharedPersonsCount"
    """, nativeQuery = true)
    Map<String, Object> getCompanyLinks(@Param("uid") String uid);


    @Query(value = """
    SELECT DISTINCT synthetic_f
    FROM person_company_links
    WHERE synthetic_u = :uid
      AND synthetic_f IS NOT NULL
    ORDER BY synthetic_f
    """, nativeQuery = true)
    List<String> getRelatedPersonIds(@Param("uid") String uid);


    @Query(value = """
    SELECT DISTINCT p.synthetic_u
    FROM person_company_links p
    WHERE p.synthetic_f IN (
        SELECT DISTINCT synthetic_f
        FROM person_company_links
        WHERE synthetic_u = :uid
          AND synthetic_f IS NOT NULL
    )
      AND p.synthetic_u <> :uid
      AND p.synthetic_u IS NOT NULL
    """, nativeQuery = true)
    List<String> getRelatedCompanyIds(@Param("uid") String uid);


    @Query(value = """
    SELECT DISTINCT rc_name
    FROM person_company_links
    WHERE synthetic_u = :uid
      AND rc_name IS NOT NULL
      AND rc_name <> ''
    """, nativeQuery = true)
    List<String> getRelatedRegions(@Param("uid") String uid);
}