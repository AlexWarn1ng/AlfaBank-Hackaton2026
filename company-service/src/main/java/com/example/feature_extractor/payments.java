package com.example.feature_extractor;

import jakarta.persistence.*;

import jakarta.persistence.*;

@Entity
@Table(name = "payments")
public class payments {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "id")
    private Long id;

    @Column(name = "beneficiary")
    private String beneficiary;

    @Column(name = "payer", columnDefinition = "TEXT")
    private String payer;

    @Column(name = "pay_amt", columnDefinition = "TEXT")
    private String payAmt;

    @Column(name = "pay_cnt", columnDefinition = "TEXT")
    private String payCnt;

    public payments() {}

    public Long getId() {
        return id;
    }

    public String getBeneficiary() {
        return beneficiary;
    }

    public void setBeneficiary(String beneficiary) {
        this.beneficiary = beneficiary;
    }

    public String getPayer() {
        return payer;
    }

    public void setPayer(String payer) {
        this.payer = payer;
    }

    public String getPayAmt() {
        return payAmt;
    }

    public void setPayAmt(String payAmt) {
        this.payAmt = payAmt;
    }

    public String getPayCnt() {
        return payCnt;
    }

    public void setPayCnt(String payCnt) {
        this.payCnt = payCnt;
    }
}