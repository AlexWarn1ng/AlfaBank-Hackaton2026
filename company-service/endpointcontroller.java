package com.example.feature_extractor;

import org.springframework.web.bind.annotation.*;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class endpointcontroller {

    private final CompanyService service;
    private final pdfprocessingservice pdfService;

    public endpointcontroller(CompanyService service, pdfprocessingservice pdfService) {
        this.service = service;
        this.pdfService = pdfService;
    }

    @PostMapping(value = "/upload-pdf", consumes = "application/pdf")
    public Map<String, Object> uploadPdf(@RequestBody byte[] pdf) throws Exception {

        int count = pdfService.process(pdf);

        return Map.of("status", "success", "extracted", count);
    }

    @GetMapping("/company/{uid}")
    public Map<String, Object> getCompany(@PathVariable String uid) {
        return service.getCompany(uid);
    }


}