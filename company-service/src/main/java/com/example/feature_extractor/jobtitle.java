package com.example.feature_extractor;

import jakarta.persistence.*;

@Entity
@Table(name = "jobtitle")
public class jobtitle {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "id")
    private Long id;

    @Column(name = "position", unique = true, nullable = false)
    private String position; // Должность

    @Column(name = "finance_importance_rate")
    private String financeImportanceRate;

    @Column(name = "business_importance_rate")
    private String businessImportanceRate;

    public jobtitle() {}

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getPosition() {
        return position;
    }

    public void setPosition(String position) {
        this.position = position;
    }

    public String getFinanceImportanceRate() {
        return financeImportanceRate;
    }

    public void setFinanceImportanceRate(String financeImportanceRate) {
        this.financeImportanceRate = financeImportanceRate;
    }

    public String getBusinessImportanceRate() {
        return businessImportanceRate;
    }

    public void setBusinessImportanceRate(String businessImportanceRate) {
        this.businessImportanceRate = businessImportanceRate;
    }
}