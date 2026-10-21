#pragma once
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <stdexcept>
#include <filesystem>
#include "customdatastructures.h"
#include <iostream>
#include <utility>
#include <vector>
#include <poppler/cpp/poppler-image.h>

class company;

class dbWorker {
public:
    dbWorker() = default;
    ~dbWorker() = default;

    dbWorker(const dbWorker&) = delete;
    dbWorker(dbWorker&&) = delete;
    dbWorker& operator=(const dbWorker&) = delete;
    dbWorker& operator=(dbWorker&&) = delete;

    void connect(const std::string& connectionString) {
        connection = std::make_unique<pqxx::connection>(connectionString);
    }

    bool isConnected() const {
        return connection != nullptr && connection->is_open();
    }

    void createTables();
    void importPdfs(const std::filesystem::path& folder);
    void loadPdf(int64_t id, const std::string& outputPath);
    std::vector<poppler::image> pdfToImages(int64_t documentId);
    void importCsv(const std::filesystem::path& folder);
    void ExtractData();
void saveAIResponse(const std::vector<std::string>& aiResponses);
    std::unique_ptr<company> getCompany(const std::string& uid);
    void deletePdf(int64_t documentId);
    CompanyLinks getCompanyLinks(const std::string& uid);
    CompanyPayments getCompanyPayments(const std::string& uid);
    bool hasPdfDocuments();

private:
    std::unique_ptr<pqxx::connection> connection = nullptr;
};