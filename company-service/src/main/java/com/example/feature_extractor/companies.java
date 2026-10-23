package com.example.feature_extractor;

import jakarta.persistence.*;

@Entity
@Table(name = "companies")
public class companies {

    @Id
    @Column(name = "synthetic_u")
    private String syntheticU;

    @Column(name = "org_type")
    private String orgType;

    @Column(name = "okfs_name")
    private String okfsName;

    @Column(name = "company_age_years_gradation")
    private String companyAgeYearsGradation;

    @Column(name = "eco_mark_label")
    private String ecoMarkLabel;

    @Column(name = "mainokved_code")
    private String mainokvedCode;

    @Column(name = "mainokved_name", columnDefinition = "TEXT")
    private String mainokvedName;

    @Column(name = "negativemark")
    private String negativeMark;

    @Column(name = "amount_workers_fns_gradation")
    private String amountWorkersFnsGradation;

    @Column(name = "amount_workers_spark_gradation")
    private String amountWorkersSparkGradation;

    @Column(name = "cl_type")
    private String clType;

    @Column(name = "cl_product_rko_mark")
    private String clProductRkoMark;

    @Column(name = "cl_product_gar_mark")
    private String clProductGarMark;

    @Column(name = "cl_product_loa_mark")
    private String clProductLoaMark;

    @Column(name = "cl_product_lea_mark")
    private String clProductLeaMark;

    @Column(name = "cl_product_dep_mark")
    private String clProductDepMark;

    @Column(name = "cl_product_zpp_mark")
    private String clProductZppMark;

    @Column(name = "fts_flag")
    private String ftsFlag;

    @Column(name = "fts_country_name")
    private String ftsCountryName;

    @Column(name = "factoring_mark")
    private String factoringMark;

    @Column(name = "revenue_company")
    private String revenueCompany;

    @Column(name = "group_id")
    private String groupId;

    @Column(name = "city")
    private String city;

    @Column(name = "region")
    private String region;

    public companies() {}

    public String getSyntheticU() {
        return syntheticU;
    }

    public void setSyntheticU(String syntheticU) {
        this.syntheticU = syntheticU;
    }

    public String getOrgType() {
        return orgType;
    }

    public void setOrgType(String orgType) {
        this.orgType = orgType;
    }

    public String getOkfsName() {
        return okfsName;
    }

    public void setOkfsName(String okfsName) {
        this.okfsName = okfsName;
    }

    public String getCompanyAgeYearsGradation() {
        return companyAgeYearsGradation;
    }

    public void setCompanyAgeYearsGradation(String companyAgeYearsGradation) {
        this.companyAgeYearsGradation = companyAgeYearsGradation;
    }

    public String getEcoMarkLabel() {
        return ecoMarkLabel;
    }

    public void setEcoMarkLabel(String ecoMarkLabel) {
        this.ecoMarkLabel = ecoMarkLabel;
    }

    public String getMainokvedCode() {
        return mainokvedCode;
    }

    public void setMainokvedCode(String mainokvedCode) {
        this.mainokvedCode = mainokvedCode;
    }

    public String getMainokvedName() {
        return mainokvedName;
    }

    public void setMainokvedName(String mainokvedName) {
        this.mainokvedName = mainokvedName;
    }

    public String getNegativeMark() {
        return negativeMark;
    }

    public void setNegativeMark(String negativeMark) {
        this.negativeMark = negativeMark;
    }

    public String getAmountWorkersFnsGradation() {
        return amountWorkersFnsGradation;
    }

    public void setAmountWorkersFnsGradation(String amountWorkersFnsGradation) {
        this.amountWorkersFnsGradation = amountWorkersFnsGradation;
    }

    public String getAmountWorkersSparkGradation() {
        return amountWorkersSparkGradation;
    }

    public void setAmountWorkersSparkGradation(String amountWorkersSparkGradation) {
        this.amountWorkersSparkGradation = amountWorkersSparkGradation;
    }

    public String getClType() {
        return clType;
    }

    public void setClType(String clType) {
        this.clType = clType;
    }

    public String getClProductRkoMark() {
        return clProductRkoMark;
    }

    public void setClProductRkoMark(String clProductRkoMark) {
        this.clProductRkoMark = clProductRkoMark;
    }

    public String getClProductGarMark() {
        return clProductGarMark;
    }

    public void setClProductGarMark(String clProductGarMark) {
        this.clProductGarMark = clProductGarMark;
    }

    public String getClProductLoaMark() {
        return clProductLoaMark;
    }

    public void setClProductLoaMark(String clProductLoaMark) {
        this.clProductLoaMark = clProductLoaMark;
    }

    public String getClProductLeaMark() {
        return clProductLeaMark;
    }

    public void setClProductLeaMark(String clProductLeaMark) {
        this.clProductLeaMark = clProductLeaMark;
    }

    public String getClProductDepMark() {
        return clProductDepMark;
    }

    public void setClProductDepMark(String clProductDepMark) {
        this.clProductDepMark = clProductDepMark;
    }

    public String getClProductZppMark() {
        return clProductZppMark;
    }

    public void setClProductZppMark(String clProductZppMark) {
        this.clProductZppMark = clProductZppMark;
    }

    public String getFtsFlag() {
        return ftsFlag;
    }

    public void setFtsFlag(String ftsFlag) {
        this.ftsFlag = ftsFlag;
    }

    public String getFtsCountryName() {
        return ftsCountryName;
    }

    public void setFtsCountryName(String ftsCountryName) {
        this.ftsCountryName = ftsCountryName;
    }

    public String getFactoringMark() {
        return factoringMark;
    }

    public void setFactoringMark(String factoringMark) {
        this.factoringMark = factoringMark;
    }

    public String getRevenueCompany() {
        return revenueCompany;
    }

    public void setRevenueCompany(String revenueCompany) {
        this.revenueCompany = revenueCompany;
    }

    public String getGroupId() {
        return groupId;
    }

    public void setGroupId(String groupId) {
        this.groupId = groupId;
    }

    public String getCity() {
        return city;
    }

    public void setCity(String city) {
        this.city = city;
    }

    public String getRegion() {
        return region;
    }

    public void setRegion(String region) {
        this.region = region;
    }

}