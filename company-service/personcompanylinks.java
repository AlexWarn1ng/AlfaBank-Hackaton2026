package com.example.feature_extractor;

import jakarta.persistence.*;

@Entity
@Table(name = "person_company_links")
public class personcompanylinks {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "synthetic_f")
    private String syntheticF;

    @Column(name = "city_name")
    private String cityName;

    @Column(name = "rc_name")
    private String rcName;

    @Column(name = "synthetic_u")
    private String syntheticU;

    public personcompanylinks() {}

    public Long getId() {
        return id;
    }

    public String getSyntheticF() {
        return syntheticF;
    }

    public void setSyntheticF(String syntheticF) {
        this.syntheticF = syntheticF;
    }

    public String getCityName() {
        return cityName;
    }

    public void setCityName(String cityName) {
        this.cityName = cityName;
    }

    public String getRcName() {
        return rcName;
    }

    public void setRcName(String rcName) {
        this.rcName = rcName;
    }

    public String getSyntheticU() {
        return syntheticU;
    }

    public void setSyntheticU(String syntheticU) {
        this.syntheticU = syntheticU;
    }
}