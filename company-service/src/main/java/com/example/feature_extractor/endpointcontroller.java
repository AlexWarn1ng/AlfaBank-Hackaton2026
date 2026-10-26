package com.example.feature_extractor;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.UUID;

@RestController
@RequestMapping("/api")
public class endpointcontroller {

    private final newsService newsService;
    private final CompanyService companyservice;
    private final pdfprocessingservice pdfService;
    private final userService userservice;
    private final currencyservice currencyservice;
    private final aifeaturesalfabankopinion_repository opinionRepository;
    private final ragservice ragservice;
    private final newssearchservice newssearchservice;
    public endpointcontroller(newssearchservice newssearchservice,ragservice ragservice, CompanyService service,aifeaturesalfabankopinion_repository opinionRepository ,pdfprocessingservice pdfService, newsService newsService,userService userservice, currencyservice currencyservice) {
        this.companyservice = service;
        this.pdfService = pdfService;
        this.newsService = newsService;
        this.userservice = userservice;
        this.currencyservice = currencyservice;
        this.opinionRepository = opinionRepository;
        this.ragservice = ragservice;
        this.newssearchservice = newssearchservice;
    }

    @PostMapping(value = "/upload-pdf", consumes = "application/pdf")
    public Map<String, Object> uploadPdf(@RequestBody byte[] pdf) throws Exception {

        int count = pdfService.process(pdf);

        return Map.of("status", "success", "extracted", count);
    }

    @PostMapping("/news/search")
    public Map<String, Object> searchNews(
            @RequestBody NewsSearchRequest request) {
        return newssearchservice.search(request.query(), request.limit(), request.excludeNewsId()
        );
    }

    @GetMapping("/company/{uid}")
    public Map<String, Object> getCompany(@PathVariable String uid) {
        return companyservice.getCompany(uid);
    }

    @GetMapping("/news/{fid}/{position}")
    public ResponseEntity<Object> getNews(@PathVariable String fid, @PathVariable String position) {
        try {
            List<Long> newsIds = newsService.getNewsIds(fid, position);

            return ResponseEntity.ok(Map.of("ID", newsIds));
        }
        catch (org.springframework.web.server.ResponseStatusException e) {
            return ResponseEntity.status(e.getStatusCode()).body(Map.of("error", e.getReason()));
        }
        catch (Exception e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(Map.of("error", e.getMessage()));
        }
    }

    @GetMapping("/opinions/get")
    public ResponseEntity<List<aifeaturesalfabankopinion>> getOpinions() {
        return ResponseEntity.ok(
                opinionRepository.findTop20ByOrderByIdAsc()
        );
    }

    @PostMapping("/news/get")
    public ResponseEntity<Map<String, Object>> getNews(@RequestBody String ids) {

        List<Long> newsIds = Arrays.stream(ids.trim().split("\\s+")).map(Long::parseLong).toList();

        Map<String, Object> result = newsService.getNews(newsIds);

        return ResponseEntity.ok(result);
    }

    @GetMapping("/news/get/{n}")
    public ResponseEntity<Map<String, Object>> getNews(@PathVariable int n) {
        return ResponseEntity.ok(newsService.getFirstNews(n));
    }

    @PostMapping("/news/rss/import")
    public ResponseEntity<Map<String, Object>> importRss()
            throws Exception {
        return ResponseEntity.ok(newsService.getNewsRss());
    }

    @GetMapping("/analysis-context/users/{userId}")
    public Map<String, Object> getAnalysisContext(@PathVariable UUID userId) {
        return userservice.getAnalysisContext(userId);
    }

    @PostMapping("/users")
    @ResponseStatus(HttpStatus.CREATED)
    public Map<String, Object> createUser(@RequestBody UserRequest request) {
        return userservice.createUser(request);
    }

    @PatchMapping("/users/{userId}")
    public Map<String, Object> updateUser(@PathVariable UUID userId, @RequestBody Map<String, String> request) {
        return userservice.updateUser(userId, request);
    }

    @GetMapping("/currency")
    public Map<String, Object> getCurrency(@RequestParam String from, @RequestParam String to) {
        return currencyservice.getRate(from, to);
    }

}