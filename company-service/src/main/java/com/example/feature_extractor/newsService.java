package com.example.feature_extractor;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Service;
import org.springframework.transaction.support.TransactionTemplate;

import java.net.http.HttpClient;
import java.time.Duration;
import java.util.*;
import java.util.stream.Collectors;

@Service
public class newsService {

    private final JdbcTemplate db;
    private final TransactionTemplate transaction;
    private final ObjectMapper mapper = new ObjectMapper();
    private final personcompanylinks_repository linksRepository;
    private final jobtitle_repository jobtitlerepository;
    private final aifeatures_repository aifeaturesrepository;
    private final HttpClient http = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(60)).build();

    private final String apiKey;

    public newsService(JdbcTemplate db, TransactionTemplate transaction, personcompanylinks_repository linksRepository, @Value("${ai.api.key}") String apiKey, jobtitle_repository jobtitlerepository, aifeatures_repository aifeaturesrepository) {
        this.db = db;
        this.jobtitlerepository = jobtitlerepository;
        this.transaction = transaction;
        this.linksRepository = linksRepository;
        this.apiKey = apiKey;
        this.aifeaturesrepository = aifeaturesrepository;
    }

    public List<Long> getNewsIds(String fid, String position) {
        List<String> companyIds = linksRepository.getCompanyIdsByPerson(fid);
        if(companyIds.isEmpty()){
            throw new RuntimeException("Can not find your company");
        }
        Optional<jobtitle> jobTitleCurrent = jobtitlerepository.findByPosition(position);

        jobtitle currentJob = jobTitleCurrent.orElseThrow(() -> new RuntimeException("Job Title not Found"));

        String financeCriterion = currentJob.getFinanceImportanceRate();
        String businessCriterion = currentJob.getBusinessImportanceRate();

        int financeVal = Integer.parseInt(financeCriterion.replaceAll("[^0-9]", ""));
        int businessVal = Integer.parseInt(businessCriterion.replaceAll("[^0-9]", ""));
        List<Long> relevantNewsIds = aifeaturesrepository.findMatchingNewsIds(financeVal, businessVal);
        return relevantNewsIds;

    }

    public Map<String, Object> getNews(List<Long> ids) {

        List<aifeatures> news = aifeaturesrepository.findAllById(ids);
        Set<Long> foundIds = news.stream().map(aifeatures::getId).collect(Collectors.toSet());
        List<Long> notFoundIds = ids.stream().filter(id -> !foundIds.contains(id)).distinct().toList();

        List<Map<String, Object>> result = news.stream().map(item -> {
            Map<String, Object> obj = new LinkedHashMap<>();
            obj.put("id", item.getId());
            obj.put("title", item.getTitle());
            obj.put("text", item.getText());
            return obj;
        }).toList();

        return Map.of("news", result, "notFoundIds", notFoundIds);
    }

}