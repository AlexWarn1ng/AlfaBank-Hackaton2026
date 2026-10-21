
#pragma once
#include <string>
#include <vector>
#include "dbWorker.h"
#include "customdatastructures.h"
#include <iostream>
#include <utility>
#include <nlohmann/json.hpp>

class company {
public:
nlohmann::json toJson() const;

company(
        const std::string& uid,
        const std::string& orgType,
        const std::string& okfs_name,
        const std::string& companyAgeYears,
        const std::string& ecoMarkLabel,
        const std::string& mainOkvedCode,
        const std::string& mainOkvedName,
        const std::string& negativeMark,
        const std::string& amountWorkersFns,
        const std::string& amountWorkersSpark,
        const std::string& clType,
        const std::string& clProductRkoMark,
        const std::string& clProductGarMark,
        const std::string& clProductLoaMark,
        const std::string& clProductLeaMark,
        const std::string& clProductDepMark,
        const std::string& clProductZppMark,
        const std::string& ftsFlag,
        const std::string& ftsCountryName,
        const std::string& factoringMark,
        const std::string& revenueCompany,
        const std::string& groupId,
        const std::string& city,
        const std::string& region
    )
        : uid(uid),
          orgType(orgType),
          okfs_name(okfs_name),
          companyAgeYears(companyAgeYears),
          ecoMarkLabel(ecoMarkLabel),
          mainOkvedCode(mainOkvedCode),
          mainOkvedName(mainOkvedName),
          negativeMark(negativeMark),
          amountWorkersFns(amountWorkersFns),
          amountWorkersSpark(amountWorkersSpark),
          clType(clType),
          clProductRkoMark(clProductRkoMark),
          clProductGarMark(clProductGarMark),
          clProductLoaMark(clProductLoaMark),
          clProductLeaMark(clProductLeaMark),
          clProductDepMark(clProductDepMark),
          clProductZppMark(clProductZppMark),
          ftsFlag(ftsFlag),
          ftsCountryName(ftsCountryName),
          factoringMark(factoringMark),
          revenueCompany(revenueCompany),
          groupId(groupId),
          city(city),
          region(region)
    {}

    void loadLinks(dbWorker& db) {
    CompanyLinks data = db.getCompanyLinks(uid);
    relatedPersonsCount = data.relatedPersonsCount;
    relatedCompaniesCount = data.relatedCompaniesCount;
    sharedPersonsCount = data.sharedPersonsCount;
    relatedPersonIds = std::move(data.relatedPersonIds);
    relatedCompanyIds = std::move(data.relatedCompanyIds);
    relatedRegions = std::move(data.relatedRegions);
    }

    void loadPayments(dbWorker& db) {
    CompanyPayments data = db.getCompanyPayments(uid);
    incomingPaymentsAmount = data.incomingAmount;
    outgoingPaymentsAmount = data.outgoingAmount;
    incomingPaymentsCount = data.incomingCount;
    outgoingPaymentsCount = data.outgoingCount;
    counterpartyIds = data.counterpartyIds;
    payersCount = data.payersCount;
    beneficiariesCount = data.beneficiariesCount;
}

private:
    std::string uid;
    std::string orgType;
    std::string okfs_name;
    std::string companyAgeYears;
    std::string ecoMarkLabel;
    std::string mainOkvedCode;
    std::string mainOkvedName;
    std::string negativeMark;
    std::string amountWorkersFns;
    std::string amountWorkersSpark;
    std::string clType;
    std::string clProductRkoMark;
    std::string clProductGarMark;
    std::string clProductLoaMark;
    std::string clProductLeaMark;
    std::string clProductDepMark;
    std::string clProductZppMark;
    std::string ftsFlag;
    std::string ftsCountryName;
    std::string factoringMark;
    std::string revenueCompany;
    std::string groupId;
    std::string city;
    std::string region;

    int relatedPersonsCount = 0;
    int relatedCompaniesCount = 0; // Со сколькими компаниями мы связаны через людей тоесть, если U200 связана с F100 а с F100 связана U300 то мы прибавляем 1.
    int sharedPersonsCount = 0; // Количество уикальных физических лиц, через которых наша компания связана с другими компаниями
    std::vector<std::string> relatedPersonIds;
    std::vector<std::string> relatedCompanyIds;
    std::vector<std::string> relatedRegions;

    double incomingPaymentsAmount = 0;  // Сколько получили
    double outgoingPaymentsAmount = 0;  // Сколько отправили
    long long incomingPaymentsCount = 0; // Количество входящих платежей
    long long outgoingPaymentsCount = 0; // Количество исходящих платежей
    int payersCount = 0;        // Уникальных отправителей
    int beneficiariesCount = 0; // Уникальных получателей
    std::vector<std::string> counterpartyIds; 
    
};
