package com.example.feature_extractor;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class endpointcontroller {

    private final newsService newsService;
    private final CompanyService companyservice;
    private final pdfprocessingservice pdfService;

    public endpointcontroller(CompanyService service, pdfprocessingservice pdfService, newsService newsService) {
        this.companyservice = service;
        this.pdfService = pdfService;
        this.newsService = newsService;
    }

    @PostMapping(value = "/upload-pdf", consumes = "application/pdf")
    public Map<String, Object> uploadPdf(@RequestBody byte[] pdf) throws Exception {

        int count = pdfService.process(pdf);

        return Map.of("status", "success", "extracted", count);
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
        catch (Exception e) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Map.of("error", e.getMessage()));
        }
    }

    @PostMapping("/news/get")
    public ResponseEntity<Map<String, Object>> getNews(@RequestBody String ids) {

        List<Long> newsIds = Arrays.stream(ids.trim().split("\\s+")).map(Long::parseLong).toList();

        Map<String, Object> result = newsService.getNews(newsIds);

        return ResponseEntity.ok(result);
    }


}