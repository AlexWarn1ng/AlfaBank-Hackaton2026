#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>
#include <memory>
#include "dbworker.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstddef>
#include <cstdint>


namespace fs = std::filesystem;

void dbWorker::pdfToImages(int64_t documentId) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec_params(
        "SELECT content FROM pdf_documents WHERE id = $1",
        documentId
    );

    if (result.empty()) {
        throw std::runtime_error("PDF not found in database");
    }

    pqxx::bytes pdfBytes =
        result[0]["content"].as<pqxx::bytes>();

auto pdfData = std::make_unique<poppler::byte_array>();

const char* begin =
    reinterpret_cast<const char*>(pdfBytes.data());

pdfData->assign(
    begin,
    begin + pdfBytes.size()
);

std::unique_ptr<poppler::document> doc(
    poppler::document::load_from_data(pdfData.get())
);

    if (!doc) {
        throw std::runtime_error("Cannot open PDF from database");
    }

    std::filesystem::path outputFolder =
        std::filesystem::current_path() /
        "pages" /
        std::to_string(documentId);

    std::filesystem::create_directories(outputFolder);

    poppler::page_renderer renderer;

    int totalPages = doc->pages();

    std::cout << "Total pages: "
              << totalPages << std::endl;

    for (int i = 0; i < totalPages; ++i) {

        std::unique_ptr<poppler::page> page(
            doc->create_page(i)
        );

        if (!page) {
            std::cerr << "Cannot read page "
                      << i + 1 << std::endl;
            continue;
        }

        poppler::image image = renderer.render_page(
            page.get(),
            150.0,
            150.0
        );

        if (!image.is_valid()) {
            std::cerr << "Cannot render page "
                      << i + 1 << std::endl;
            continue;
        }

        std::filesystem::path outputPath =
            outputFolder /
            ("page_" + std::to_string(i + 1) + ".png");

        auto utf8Path = outputPath.u8string();

        std::string filename(
            reinterpret_cast<const char*>(utf8Path.data()),
            utf8Path.size()
        );

        if (!image.save(filename, "png")) {
            std::cerr << "Cannot save page "
                      << i + 1 << std::endl;
            continue;
        }

        std::cout << "Saved page "
                  << i + 1 << "/" << totalPages
                  << std::endl;
    }

    std::cout << "PDF rendering completed!"
              << std::endl;
}

void dbWorker::loadPdf(int64_t id, const std::string& outputPath) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec_params(
        "SELECT content FROM pdf_documents WHERE id = $1",
        id
    );

    if (result.empty()) {
        throw std::runtime_error("PDF not found");
    }

    // Получаем бинарное содержимое PDF из PostgreSQL
    pqxx::bytes bytes = result[0]["content"].as<pqxx::bytes>();

    // Создаём PDF-файл на диске
    std::ofstream file(outputPath, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Cannot create output file");
    }

    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );

    if (!file) {
        throw std::runtime_error("Cannot write PDF file");
    }
}

  
void dbWorker::importPdfs(const fs::path& folder) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    fs::create_directories(folder);

    for (const auto& entry : fs::directory_iterator(folder)) {
        try {
            if (!entry.is_regular_file()) {
                continue;
            }

            fs::path path = entry.path();

            if (path.extension() != ".pdf" &&
                path.extension() != ".PDF") {
                continue;
            }

            auto utf8 = path.filename().u8string();

            std::string filename(
                reinterpret_cast<const char*>(utf8.data()),
                utf8.size()
            );

            std::cout << "Checking: " << filename << std::endl;

            std::ifstream file(path, std::ios::binary);

            if (!file) {
                std::cerr << "Cannot open PDF: "
                          << filename << std::endl;
                continue;
            }

            std::vector<unsigned char> bytes(
                std::istreambuf_iterator<char>{file},
                std::istreambuf_iterator<char>{}
            );

            if (file.bad()) {
                std::cerr << "Cannot read PDF: "
                          << filename << std::endl;
                continue;
            }

            file.close();

            if (bytes.empty()) {
                std::cerr << "Empty PDF: "
                          << filename << std::endl;
                continue;
            }

            std::cout << "Read "
                      << bytes.size()
                      << " bytes" << std::endl;

            pqxx::work tx(*connection);

            auto result = tx.exec_params(
                "SELECT content FROM pdf_documents WHERE filename = $1",
                filename
            );

            bool alreadyExists = false;

            for (const auto& row : result) {
                pqxx::bytes stored =
                    row["content"].as<pqxx::bytes>();

                if (stored.size() == bytes.size() &&
                    std::equal(
                        stored.begin(),
                        stored.end(),
                        bytes.begin(),
                        [](std::byte a, unsigned char b) {
                            return std::to_integer<unsigned char>(a) == b;
                        }
                    )) {
                    alreadyExists = true;
                    break;
                }
            }

            if (alreadyExists) {
                std::cout << "Already imported: "
                          << filename << std::endl;

                tx.commit();

                if (fs::remove(path)) {
                    std::cout << "Deleted: "
                              << filename << std::endl;
                } else {
                    std::cerr << "Cannot delete: "
                              << filename << std::endl;
                }

                continue;
            }

            pqxx::bytes pdfBytes;
            pdfBytes.reserve(bytes.size());

            for (unsigned char byte : bytes) {
                pdfBytes.push_back(
                    static_cast<std::byte>(byte)
                );
            }

            std::cout << "Inserting PDF..." << std::endl;

            tx.exec_params(
                "INSERT INTO pdf_documents (filename, content) "
                "VALUES ($1, $2)",
                filename,
                pdfBytes
            );

            tx.commit();

            std::cout << "Imported: "
                      << filename << std::endl;

            if (fs::remove(path)) {
                std::cout << "Deleted: "
                          << filename << std::endl;
            } else {
                std::cerr << "Cannot delete: "
                          << filename << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Import or delete failed: "
                      << e.what() << std::endl;
        }
    }

    std::cout << "Finished scanning folder!" << std::endl;
}

    void dbWorker::createTables() {
        if (!isConnected()) {
            throw std::runtime_error("Database is not connected");
        }

        pqxx::work tx(*connection);

        tx.exec(R"(
            CREATE TABLE IF NOT EXISTS pdf_documents (
                id BIGSERIAL PRIMARY KEY,
                filename TEXT NOT NULL,
                content BYTEA NOT NULL,
                uploaded_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
            );
        )");

        tx.exec(R"(
            CREATE TABLE IF NOT EXISTS ai_features (
                id BIGSERIAL PRIMARY KEY,

                document_id BIGINT NOT NULL
                    REFERENCES pdf_documents(id)
                    ON DELETE CASCADE,

                name TEXT NOT NULL,
                value TEXT,
                description TEXT,
                category TEXT,

                importance DOUBLE PRECISION
                    CHECK (importance BETWEEN 0 AND 1),

                confidence DOUBLE PRECISION
                    CHECK (confidence BETWEEN 0 AND 1),

                page_number INTEGER
                    CHECK (page_number > 0),

                properties JSONB NOT NULL DEFAULT '{}'::jsonb,

                created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
            );
        )");

        tx.exec(R"(
            CREATE INDEX IF NOT EXISTS idx_ai_features_document
            ON ai_features(document_id);
        )");

        tx.commit();
    }

    