#pragma once
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <stdexcept>
#include <filesystem>

class dbWorker {
public:
    static dbWorker* getinst() {
        std::lock_guard<std::mutex> lock(sinMut);

        if (inst == nullptr) {
            inst.reset(new dbWorker);
        }

        return inst.get();
    }

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
    void pdfToImages(int64_t documentId);

private:
    inline static std::mutex sinMut;
    inline static std::unique_ptr<dbWorker> inst = nullptr;

    std::unique_ptr<pqxx::connection> connection = nullptr;

    dbWorker() = default;
};