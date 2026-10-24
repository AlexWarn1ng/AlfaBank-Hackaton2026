package com.example.feature_extractor;

import org.springframework.stereotype.Service;
import org.springframework.http.HttpStatus;
import org.springframework.web.server.ResponseStatusException;

import java.util.*;

@Service
public class userService {

    private final users_repository usersrepository;
    private final personcompanylinks_repository linksRepository;
    private final jobtitle_repository jobtitlerepository;
    public userService(users_repository usersrepository, personcompanylinks_repository linksRepository, jobtitle_repository jobtitlerepository) {
        this.usersrepository = usersrepository;
        this.linksRepository = linksRepository;
        this.jobtitlerepository = jobtitlerepository;
    }

    public Map<String, Object> getAnalysisContext(UUID userId) {

        users user = usersrepository.findById(userId)
                .orElseThrow(() -> new ResponseStatusException(
                        HttpStatus.NOT_FOUND, "User not found"
                ));

        String fId = user.getFGuid();

        List<personcompanylinks> links = fId == null
                ? List.of()
                : linksRepository.findBySyntheticF(fId);

        List<String> companyIds = links.stream()
                .map(personcompanylinks::getSyntheticU)
                .filter(Objects::nonNull)
                .distinct()
                .toList();

        // Должность берётся из users.role
        String position = user.getRole();

        // По должности из users ищем только веса
        Optional<jobtitle> job = jobtitlerepository.findByPosition(position);

        Map<String, Object> result = new LinkedHashMap<>();

        result.put("userId", user.getId());
        result.put("fId", fId);
        result.put("position", position);

        result.put("city", links.isEmpty() ? null : links.get(0).getCityName());
        result.put("region", links.isEmpty() ? null : links.get(0).getRcName());

        result.put("companyIds", companyIds);

        result.put("financeImportanceRate", job.map(jobtitle::getFinanceImportanceRate).orElse(null));

        result.put("businessImportanceRate", job.map(jobtitle::getBusinessImportanceRate).orElse(null));

        return result;
    }
}