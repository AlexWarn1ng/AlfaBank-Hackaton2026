package com.example.feature_extractor;

import jakarta.persistence.*;

@Entity
@Table(name = "ai_features")
public class aifeatures {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "id")
    private Long id;

    @Column(name = "title")
    private String title;

    @Column(name = "text", columnDefinition = "TEXT")
    private String text;

    @Column(name = "finance_importance")
    private Integer financeImportance;

    @Column(name = "business_importance")
    private Integer businessImportance;

    @Column(name = "global_importance")
    private Integer globalImportance;

    public aifeatures() {}

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public Integer getFinanceImportance() {
        return financeImportance;
    }

    public void setFinanceImportance(Integer financeImportance) {
        this.financeImportance = financeImportance;
    }

    public Integer getBusinessImportance() {
        return businessImportance;
    }

    public void setBusinessImportance(Integer businessImportance) {
        this.businessImportance = businessImportance;
    }

    public Integer getGlobalImportance() {
        return globalImportance;
    }

    public void setGlobalImportance(Integer globalImportance) {
        this.globalImportance = globalImportance;
    }

}