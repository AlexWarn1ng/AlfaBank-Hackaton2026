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
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include "company.h"
#include "dbWorker.h"
#include "company.h"

namespace fs = std::filesystem;

std::unique_ptr<company> dbWorker::getCompany(const std::string& uid) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec_params(
        "SELECT * FROM companies WHERE synthetic_u = $1",
        uid
    );

    if (result.empty()) {
        return nullptr;
    }

    const auto& row = result[0];

    auto getString = [&](const char* column) -> std::string {
        const auto& field = row[column];
        return field.is_null() ? "" : field.as<std::string>();
    };

    return std::make_unique<company>(
        getString("synthetic_u"),
        getString("org_type"),
        getString("okfs_name"),
        getString("company_age_years_gradation"),
        getString("eco_mark_label"),
        getString("mainokved_code"),
        getString("mainokved_name"),
        getString("negativemark"),
        getString("amount_workers_fns_gradation"),
        getString("amount_workers_spark_gradation"),
        getString("cl_type"),
        getString("cl_product_rko_mark"),
        getString("cl_product_gar_mark"),
        getString("cl_product_loa_mark"),
        getString("cl_product_lea_mark"),
        getString("cl_product_dep_mark"),
        getString("cl_product_zpp_mark"),
        getString("fts_flag"),
        getString("fts_country_name"),
        getString("factoring_mark"),
        getString("revenue_company"),
        getString("group_id"),
        getString("city"),
        getString("region")
    );
}

CompanyPayments dbWorker::getCompanyPayments(const std::string& uid) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec_params(R"(
        SELECT
            COALESCE(SUM(
                CASE WHEN beneficiary = $1
                THEN NULLIF(pay_amt, '')::numeric
                ELSE 0 END
            ), 0) AS incoming_amount,

            COALESCE(SUM(
                CASE WHEN payer = $1
                THEN NULLIF(pay_amt, '')::numeric
                ELSE 0 END
            ), 0) AS outgoing_amount,

            COALESCE(SUM(
                CASE WHEN beneficiary = $1
                THEN NULLIF(pay_cnt, '')::bigint
                ELSE 0 END
            ), 0) AS incoming_count,

            COALESCE(SUM(
                CASE WHEN payer = $1
                THEN NULLIF(pay_cnt, '')::bigint
                ELSE 0 END
            ), 0) AS outgoing_count,

            COUNT(DISTINCT CASE
                WHEN beneficiary = $1 THEN payer
            END) AS payers_count,

            COUNT(DISTINCT CASE
                WHEN payer = $1 THEN beneficiary
            END) AS beneficiaries_count

        FROM payments
        WHERE payer = $1 OR beneficiary = $1
    )", uid);

    const auto& row = result[0];

    CompanyPayments data;

    data.incomingAmount = row["incoming_amount"].as<double>();
    data.outgoingAmount = row["outgoing_amount"].as<double>();

    data.incomingCount = row["incoming_count"].as<long long>();
    data.outgoingCount = row["outgoing_count"].as<long long>();

    data.payersCount = row["payers_count"].as<int>();
    data.beneficiariesCount = row["beneficiaries_count"].as<int>();

    // Получаем уникальные ID всех контрагентов
    auto counterparties = tx.exec_params(R"(
        SELECT DISTINCT
            CASE
                WHEN payer = $1 THEN beneficiary
                ELSE payer
            END AS counterparty_uid
        FROM payments
        WHERE payer = $1 OR beneficiary = $1
    )", uid);

    for (const auto& counterparty : counterparties) {
        if (!counterparty["counterparty_uid"].is_null()) {
            data.counterpartyIds.push_back(
                counterparty["counterparty_uid"].as<std::string>()
            );
        }
    }

    return data;
}



CompanyLinks dbWorker::getCompanyLinks(const std::string& uid) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec_params(R"(
        WITH persons AS (
            SELECT DISTINCT synthetic_f
            FROM person_company_links
            WHERE synthetic_u = $1
              AND synthetic_f IS NOT NULL
        ),
        related AS (
            SELECT DISTINCT
                p.synthetic_f,
                p.synthetic_u
            FROM person_company_links p
            JOIN persons x
                ON p.synthetic_f = x.synthetic_f
            WHERE p.synthetic_u <> $1
              AND p.synthetic_u IS NOT NULL
        )
        SELECT
            (SELECT COUNT(*) FROM persons) AS persons_count,

            (SELECT COUNT(DISTINCT synthetic_u)
             FROM related) AS companies_count,

            (SELECT COUNT(DISTINCT synthetic_f)
             FROM related) AS shared_persons_count,

            COALESCE(
                (SELECT array_agg(synthetic_f ORDER BY synthetic_f)
                 FROM persons),
                ARRAY[]::text[]
            ) AS person_ids,

            COALESCE(
                (SELECT array_agg(DISTINCT synthetic_u)
                 FROM related),
                ARRAY[]::text[]
            ) AS company_ids,

            COALESCE(
                (
                    SELECT array_agg(DISTINCT rc_name)
                    FROM person_company_links
                    WHERE synthetic_u = $1
                      AND rc_name IS NOT NULL
                      AND rc_name <> ''
                ),
                ARRAY[]::text[]
            ) AS related_regions
    )", uid);

    CompanyLinks data;

    if (result.empty()) {
        return data;
    }

    const auto& row = result[0];

    data.relatedPersonsCount =
        row["persons_count"].as<int>();

    data.relatedCompaniesCount =
        row["companies_count"].as<int>();

    data.sharedPersonsCount =
        row["shared_persons_count"].as<int>();

    auto readArray = [](const pqxx::field& field) {
        std::vector<std::string> values;

        if (field.is_null()) {
            return values;
        }

        std::string raw = field.as<std::string>();

        if (raw.size() < 2) {
            return values;
        }

        std::string current;

        for (size_t i = 1; i + 1 < raw.size(); ++i) {
            if (raw[i] == ',') {
                values.push_back(current);
                current.clear();
            } else if (raw[i] != '"') {
                current += raw[i];
            }
        }

        if (!current.empty()) {
            values.push_back(current);
        }

        return values;
    };

    data.relatedPersonIds = readArray(row["person_ids"]);
    data.relatedCompanyIds = readArray(row["company_ids"]);
    data.relatedRegions = readArray(row["related_regions"]);

    return data;
}

bool dbWorker::hasPdfDocuments() {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::read_transaction tx(*connection);

    auto result = tx.exec(
        "SELECT EXISTS(SELECT 1 FROM pdf_documents)"
    );

    return result[0][0].as<bool>();
}

std::vector<poppler::image> dbWorker::pdfToImages(int64_t documentId) {
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

poppler::page_renderer renderer;
renderer.set_image_format(poppler::image::format_rgb24);

int totalPages = doc->pages();

    std::cout << "Total pages: "
              << totalPages << std::endl;

    std::vector<poppler::image> images;
    images.reserve(totalPages);

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

        images.push_back(std::move(image));

        std::cout << "Rendered page " << i + 1 << "/" << totalPages << std::endl;
    }

    std::cout << "PDF rendering completed!" << std::endl;

    return images;
}



void dbWorker::importCsv(const fs::path& folder) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    fs::create_directories(folder);

    const std::unordered_map<std::string, std::string> tables = {
        {"df_Uid_data_hack.csv", "companies"},
        {"df_Fid_Uid_link_hack.csv", "person_company_links"},
        {"df_Uid_payment_hack.csv", "payments"}
    };

    // Разбираем строку CSV с поддержкой кавычек
    auto parseCsv = [](const std::string& line, char delimiter) {
        std::vector<std::string> values;
        std::string value;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (c == '"') {
                if (inQuotes && i + 1 < line.size() &&
                    line[i + 1] == '"') {
                    value += '"';
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == delimiter && !inQuotes) {
                values.push_back(value);
                value.clear();
            } else if (c != '\r') {
                value += c;
            }
        }

        if (inQuotes) {
            throw std::runtime_error("Unclosed CSV quote");
        }

        values.push_back(value);
        return values;
    };

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        fs::path path = entry.path();
        std::string filename = path.filename().string();

        auto it = tables.find(filename);
        if (it == tables.end()) continue;

        const std::string& table = it->second;

        try {
            std::ifstream file(path, std::ios::binary);

            if (!file) {
                throw std::runtime_error("Cannot open CSV");
            }

            std::string header;

            if (!std::getline(file, header)) {
                throw std::runtime_error("Empty CSV");
            }

            // Убираем UTF-8 BOM
            if (header.size() >= 3 &&
                (unsigned char)header[0] == 0xEF &&
                (unsigned char)header[1] == 0xBB &&
                (unsigned char)header[2] == 0xBF) {
                header.erase(0, 3);
            }

            // Определяем разделитель
            char delimiter =
                std::count(header.begin(), header.end(), ';') >
                std::count(header.begin(), header.end(), ',')
                ? ';' : ',';

            auto columns = parseCsv(header, delimiter);

            // Получаем список существующих столбцов БД
            pqxx::work tx(*connection);

            auto result = tx.exec_params(
                "SELECT column_name "
                "FROM information_schema.columns "
                "WHERE table_schema = 'public' "
                "AND table_name = $1",
                table
            );

            std::unordered_set<std::string> allowed;

            for (const auto& row : result) {
                allowed.insert(row[0].as<std::string>());
            }

            std::string columnSql;

            for (size_t i = 0; i < columns.size(); ++i) {
                std::string& col = columns[i];

                // Переводим имя столбца в нижний регистр
                for (char& c : col) {
                    c = static_cast<char>(
                        std::tolower((unsigned char)c)
                    );
                }

                // Убираем пробелы вокруг названия
                while (!col.empty() && col.front() == ' ')
                    col.erase(col.begin());

                while (!col.empty() && col.back() == ' ')
                    col.pop_back();

                if (!allowed.count(col)) {
                    throw std::runtime_error(
                        "Unknown column: " + col
                    );
                }

                if (i > 0) columnSql += ", ";
                columnSql += tx.quote_name(col);
            }

            // Ищем столбец ID компании
            size_t idIndex = columns.size();

            for (size_t i = 0; i < columns.size(); ++i) {
                if (columns[i] == "synthetic_u") {
                    idIndex = i;
                    break;
                }
            }

            if (table == "companies" &&
                idIndex == columns.size()) {
                throw std::runtime_error(
                    "CSV has no synthetic_u column"
                );
            }

            std::cout << "Importing: " << filename << std::endl;

            const size_t BATCH_SIZE = 5000;

            size_t imported = 0;
            size_t skipped = 0;
            size_t batchCount = 0;

            std::string batchSql;
            std::string line;

            // Отправляем накопленные строки одним INSERT
            auto flushBatch = [&]() {
                if (batchCount == 0) return;

                std::string sql =
                    "INSERT INTO " + tx.quote_name(table) +
                    " (" + columnSql + ") VALUES " + batchSql;

                // Повторный импорт компаний не создаёт дубликаты
                if (table == "companies") {
                    sql += " ON CONFLICT (synthetic_u) DO NOTHING";
                }

                tx.exec(sql);

                imported += batchCount;

                std::cout << "\rProcessed: " << imported
                          << " | Skipped: " << skipped
                          << std::flush;

                batchSql.clear();
                batchCount = 0;
            };

            while (std::getline(file, line)) {
                if (line.empty() || line == "\r") continue;

                auto values = parseCsv(line, delimiter);

                // Проверяем количество столбцов
                if (values.size() != columns.size()) {
                    ++skipped;
                    continue;
                }

                // Проверяем ID компании
                if (table == "companies" &&
                    values[idIndex].empty()) {
                    ++skipped;
                    continue;
                }

                // Формируем SQL для одной строки
                std::string rowSql = "(";

                for (size_t i = 0; i < values.size(); ++i) {
                    if (i > 0) rowSql += ", ";

                    rowSql += tx.quote(values[i]);
                }

                rowSql += ")";

                if (!batchSql.empty()) {
                    batchSql += ", ";
                }

                batchSql += rowSql;
                ++batchCount;

                if (batchCount >= BATCH_SIZE) {
                    flushBatch();
                }
            }

            if (file.bad()) {
                throw std::runtime_error("CSV read error");
            }

            flushBatch();

            file.close();

            tx.commit();

            std::cout << "\nFinished: " << filename
                      << " | Processed: " << imported
                      << " | Skipped: " << skipped
                      << std::endl;

            // Не удаляем файл, если часть строк пропущена
            if (skipped == 0) {
                if (fs::remove(path)) {
                    std::cout << "Deleted: "
                              << filename << std::endl;
                }
            } else {
                std::cout << "CSV kept because some rows "
                             "were skipped."
                          << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "\nCSV import failed: "
                      << filename << " | "
                      << e.what() << std::endl;
        }
    }

    std::cout << "CSV import finished!" << std::endl;
}

void dbWorker::deletePdf(int64_t documentId) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::work tx(*connection);

    auto result = tx.exec_params(
        "DELETE FROM public.pdf_documents WHERE id = $1",
        documentId
    );

    if (result.affected_rows() != 1) {
        throw std::runtime_error("PDF deletion failed");
    }

    tx.commit();

    std::cout << "PDF deleted: " << documentId << std::endl;
}
void dbWorker::saveAIResponse(const std::vector<std::string>& aiResponses) {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    pqxx::work tx(*connection);
    int inserted = 0;

    for (const auto& response : aiResponses) {
        nlohmann::json news = nlohmann::json::parse(response);

        if (!news.is_array()) {
            throw std::runtime_error("AI response must be a JSON array");
        }

        for (const auto& item : news) {
            std::string title = item.at("title").get<std::string>();
            std::string text = item.at("text").get<std::string>();

            int finance = item.at("finance").get<int>();
            int business = item.at("buisness").get<int>();
            int global = item.at("global").get<int>();

            tx.exec_params(
                "INSERT INTO public.ai_features "
                "(title, text, finance_importance, business_importance, global_importance) "
                "VALUES ($1, $2, $3, $4, $5)",
                title,
                text,
                finance,
                business,
                global
            );

            ++inserted;
        }
    }

    tx.commit();

    std::cout << "Inserted " << inserted
              << " news into database" << std::endl;
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
        title TEXT,
        text TEXT,
        finance_importance INT,
        business_importance INT,
        global_importance INT
    );
)");


    // Юридические лица
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS companies (
            synthetic_u TEXT PRIMARY KEY,
            org_type TEXT,
            okfs_name TEXT,
            company_age_years_gradation TEXT,
            eco_mark_label TEXT,
            mainokved_code TEXT,
            mainokved_name TEXT,
            negativemark TEXT,
            amount_workers_fns_gradation TEXT,
            amount_workers_spark_gradation TEXT,
            cl_type TEXT,
            cl_product_rko_mark TEXT,
            cl_product_gar_mark TEXT,
            cl_product_loa_mark TEXT,
            cl_product_lea_mark TEXT,
            cl_product_dep_mark TEXT,
            cl_product_zpp_mark TEXT,
            fts_flag TEXT,
            fts_country_name TEXT,
            factoring_mark TEXT,
            revenue_company TEXT,
            group_id TEXT,
            city TEXT,
            region TEXT
        );
    )");

    // Связи физических лиц с компаниями
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS person_company_links (
            id BIGSERIAL PRIMARY KEY,
            synthetic_f TEXT,
            city_name TEXT,
            rc_name TEXT,
            synthetic_u TEXT
        );
    )");

    // Платежи между компаниями
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS payments (
            id BIGSERIAL PRIMARY KEY,
            beneficiary TEXT,
            payer TEXT,
            pay_amt TEXT,
            pay_cnt TEXT
        );
    )");

    // Индексы для быстрого поиска связей
    tx.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_person_company
        ON person_company_links(synthetic_u);
    )");

    tx.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_payments_payer
        ON payments(payer);
    )");

    tx.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_payments_beneficiary
        ON payments(beneficiary);
    )");


        tx.commit();
    }

    