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

        List<String> companyIds = fId == null ? List.of()
                : linksRepository.getCompanyIdsByPerson(fId);

        String position = JobRoles.normalize(user.getRole());

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

    public Map<String, Object> createUser(UserRequest request) {

        if (request.id() == null) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST, "User ID is required"
            );
        }

        if (request.fUid() == null || request.fUid().isBlank()) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST, "fUid is required"
            );
        }

        if (usersrepository.existsById(request.id())) {
            throw new ResponseStatusException(
                    HttpStatus.CONFLICT, "User already exists"
            );
        }

        // Проверяем существование физлица в person_company_links
        List<personcompanylinks> links =
                linksRepository.findBySyntheticF(request.fUid());

        if (links.isEmpty()) {
            throw new ResponseStatusException(
                    HttpStatus.BAD_REQUEST,
                    "fUid not found in person_company_links"
            );
        }

        users user = new users();

        user.setId(request.id());
        user.setLogin(request.login());
        user.setPasswordHash(request.passwordHash());
        user.setFGuid(request.fUid());
        user.setName(request.name());
        user.setRole(request.role());

        usersrepository.save(user);

        Map<String, Object> result = new LinkedHashMap<>();

        result.put("id", user.getId());
        result.put("login", user.getLogin());
        result.put("fUid", user.getFGuid());
        result.put("name", user.getName());
        result.put("role", user.getRole());

        return result;
    }
    public Map<String, Object> updateUser(UUID userId, Map<String, String> request) {

        users user = usersrepository.findById(userId)
                .orElseThrow(() -> new ResponseStatusException(
                        HttpStatus.NOT_FOUND, "User not found"
                ));

        Set<String> allowedFields = Set.of(
                "login", "passwordHash", "fUid", "name", "role"
        );

        for (String field : request.keySet()) {
            if (!allowedFields.contains(field)) {
                throw new ResponseStatusException(
                        HttpStatus.BAD_REQUEST,
                        "Unknown or forbidden field: " + field
                );
            }

            if (request.get(field) == null || request.get(field).isBlank()) {
                throw new ResponseStatusException(
                        HttpStatus.BAD_REQUEST,
                        "Field cannot be empty: " + field
                );
            }
        }

        if (request.containsKey("fUid")) {
            String fUid = request.get("fUid");

            if (linksRepository.findBySyntheticF(fUid).isEmpty()) {
                throw new ResponseStatusException(
                        HttpStatus.BAD_REQUEST,
                        "fUid not found in person_company_links"
                );
            }

            user.setFGuid(fUid);
        }

        if (request.containsKey("login")) {
            user.setLogin(request.get("login"));
        }

        if (request.containsKey("passwordHash")) {
            user.setPasswordHash(request.get("passwordHash"));
        }

        if (request.containsKey("name")) {
            user.setName(request.get("name"));
        }

        if (request.containsKey("role")) {
            user.setRole(request.get("role"));
        }

        usersrepository.save(user);

        Map<String, Object> result = new LinkedHashMap<>();

        result.put("id", user.getId());
        result.put("login", user.getLogin());
        result.put("fUid", user.getFGuid());
        result.put("name", user.getName());
        result.put("role", user.getRole());

        return result;
    }
}