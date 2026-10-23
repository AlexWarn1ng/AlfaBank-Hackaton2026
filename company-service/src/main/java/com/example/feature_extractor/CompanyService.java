package com.example.feature_extractor;

import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

import java.util.LinkedHashMap;
import java.util.Map;

import static org.springframework.http.HttpStatus.NOT_FOUND;

@Service
public class CompanyService {

    private final companies_repository companiesRepository;
    private final payments_repository paymentsRepository;
    private final personcompanylinks_repository linksRepository;

    public CompanyService(
            companies_repository companiesRepository,
            payments_repository paymentsRepository,
            personcompanylinks_repository linksRepository
    ) {
        this.companiesRepository = companiesRepository;
        this.paymentsRepository = paymentsRepository;
        this.linksRepository = linksRepository;
    }

    @Transactional(readOnly = true)
    public Map<String, Object> getCompany(String uid) {

        companies company = companiesRepository.findById(uid).orElseThrow(() -> new ResponseStatusException(NOT_FOUND, "Company not found"));

        Map<String, Object> payments = paymentsRepository.getCompanyPayments(uid);

        Map<String, Object> links = linksRepository.getCompanyLinks(uid);

        Map<String, Object> result = new LinkedHashMap<>();

        result.put("uid", company.getSyntheticU());
        result.put("orgType", company.getOrgType());
        result.put("okfs_name", company.getOkfsName());
        result.put("companyAgeYears", company.getCompanyAgeYearsGradation());
        result.put("ecoMarkLabel", company.getEcoMarkLabel());
        result.put("mainOkvedCode", company.getMainokvedCode());
        result.put("mainOkvedName", company.getMainokvedName());
        result.put("negativeMark", company.getNegativeMark());
        result.put("amountWorkersFns", company.getAmountWorkersFnsGradation());
        result.put("amountWorkersSpark", company.getAmountWorkersSparkGradation());
        result.put("clType", company.getClType());
        result.put("clProductRkoMark", company.getClProductRkoMark());
        result.put("clProductGarMark", company.getClProductGarMark());
        result.put("clProductLoaMark", company.getClProductLoaMark());
        result.put("clProductLeaMark", company.getClProductLeaMark());
        result.put("clProductDepMark", company.getClProductDepMark());
        result.put("clProductZppMark", company.getClProductZppMark());
        result.put("ftsFlag", company.getFtsFlag());
        result.put("ftsCountryName", company.getFtsCountryName());
        result.put("factoringMark", company.getFactoringMark());
        result.put("revenueCompany", company.getRevenueCompany());
        result.put("groupId", company.getGroupId());
        result.put("city", company.getCity());
        result.put("region", company.getRegion());

        result.putAll(payments);
        result.putAll(links);

        result.put("counterpartyIds", paymentsRepository.getCounterpartyIds(uid));

        result.put("relatedPersonIds", linksRepository.getRelatedPersonIds(uid));

        result.put("relatedCompanyIds", linksRepository.getRelatedCompanyIds(uid));

        result.put("relatedRegions", linksRepository.getRelatedRegions(uid));

        return result;
    }
}