package com.example.feature_extractor;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Map;

public interface payments_repository
        extends JpaRepository<payments, Long> {

    List<payments> findByPayer(String payer);

    List<payments> findByBeneficiary(String beneficiary);

    List<payments> findByPayAmt(String payAmt);

    List<payments> findByPayCnt(String payCnt);

    @Query(value = """
    SELECT
        COALESCE(SUM(
            CASE WHEN beneficiary = :uid
            THEN NULLIF(pay_amt, '')::numeric
            ELSE 0 END
        ), 0) AS "incomingAmount",

        COALESCE(SUM(
            CASE WHEN payer = :uid
            THEN NULLIF(pay_amt, '')::numeric
            ELSE 0 END
        ), 0) AS "outgoingAmount",

        COALESCE(SUM(
            CASE WHEN beneficiary = :uid
            THEN NULLIF(pay_cnt, '')::bigint
            ELSE 0 END
        ), 0) AS "incomingCount",

        COALESCE(SUM(
            CASE WHEN payer = :uid
            THEN NULLIF(pay_cnt, '')::bigint
            ELSE 0 END
        ), 0) AS "outgoingCount",

        COUNT(DISTINCT CASE
            WHEN beneficiary = :uid THEN payer
        END) AS "payersCount",

        COUNT(DISTINCT CASE
            WHEN payer = :uid THEN beneficiary
        END) AS "beneficiariesCount"

    FROM payments
    WHERE payer = :uid OR beneficiary = :uid
    """, nativeQuery = true)
    Map<String, Object> getCompanyPayments(@Param("uid") String uid);


    @Query(value = """
    SELECT DISTINCT
        CASE
            WHEN payer = :uid THEN beneficiary
            ELSE payer
        END
    FROM payments
    WHERE payer = :uid OR beneficiary = :uid
    """, nativeQuery = true)
    List<String> getCounterpartyIds(@Param("uid") String uid);

}