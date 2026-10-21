
#include "company.h"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <string>


#include "company.h"
#include <nlohmann/json.hpp>

nlohmann::json company::toJson() const {
    nlohmann::json json;

    // COMPANY
    json["company"] = {
        {"uid", uid},
        {"orgType", orgType},
        {"okfsName", okfs_name},
        {"companyAgeYears", companyAgeYears},
        {"ecoMarkLabel", ecoMarkLabel},
        {"mainOkvedCode", mainOkvedCode},
        {"mainOkvedName", mainOkvedName},
        {"negativeMark", negativeMark},
        {"amountWorkersFns", amountWorkersFns},
        {"amountWorkersSpark", amountWorkersSpark},
        {"clType", clType},
        {"clProductRkoMark", clProductRkoMark},
        {"clProductGarMark", clProductGarMark},
        {"clProductLoaMark", clProductLoaMark},
        {"clProductLeaMark", clProductLeaMark},
        {"clProductDepMark", clProductDepMark},
        {"clProductZppMark", clProductZppMark},
        {"ftsFlag", ftsFlag},
        {"ftsCountryName", ftsCountryName},
        {"factoringMark", factoringMark},
        {"revenueCompany", revenueCompany},
        {"groupId", groupId},
        {"city", city},
        {"region", region}
    };

    // LINKS
    json["links"] = {
        {"relatedPersonsCount", relatedPersonsCount},
        {"relatedCompaniesCount", relatedCompaniesCount},
        {"sharedPersonsCount", sharedPersonsCount},
        {"relatedPersonIds", relatedPersonIds},
        {"relatedCompanyIds", relatedCompanyIds},
        {"relatedRegions", relatedRegions}
    };

    // PAYMENTS
    json["payments"] = {
        {"incomingAmount", incomingPaymentsAmount},
        {"outgoingAmount", outgoingPaymentsAmount},
        {"incomingCount", incomingPaymentsCount},
        {"outgoingCount", outgoingPaymentsCount},
        {"payersCount", payersCount},
        {"beneficiariesCount", beneficiariesCount}
    };

    // COUNTERPARTIES
    json["counterparties"] = {
        {"total", counterpartyIds.size()},
        {"ids", counterpartyIds}
    };

    return json;
}

