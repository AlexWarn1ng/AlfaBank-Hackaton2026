#pragma once
#include <vector>
#include <string>

struct CompanyLinks {
    int relatedPersonsCount = 0;
    int relatedCompaniesCount = 0;
    int sharedPersonsCount = 0;
    std::vector<std::string> relatedPersonIds;
    std::vector<std::string> relatedCompanyIds;
    std::vector<std::string> relatedRegions;
};

struct CompanyPayments {
    double incomingAmount = 0;
    double outgoingAmount = 0;
    long long incomingCount = 0;
    long long outgoingCount = 0;
    int payersCount = 0;
    int beneficiariesCount = 0;
    std::vector<std::string> counterpartyIds;
};