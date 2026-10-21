#pragma once
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
#include <curl/curl.h>
#include <png.h>
#include <nlohmann/json.hpp>
#include <poppler/cpp/poppler-image.h>


std::string encodeBase64(const unsigned char* data, size_t size) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0*redacted*9+/";

    std::string result;
    result.reserve(((size + 2) / 3) * 4);

    for (size_t i = 0; i < size; i += 3) {
        unsigned int value = static_cast<unsigned int>(data[i]) << 16;

        if (i + 1 < size)
            value |= static_cast<unsigned int>(data[i + 1]) << 8;

        if (i + 2 < size)
            value |= static_cast<unsigned int>(data[i + 2]);

        result += table[(value >> 18) & 63];
        result += table[(value >> 12) & 63];
        result += (i + 1 < size) ? table[(value >> 6) & 63] : '=';
        result += (i + 2 < size) ? table[value & 63] : '=';
    }

    return result;
}

std::vector<unsigned char> imageToPng(const poppler::image& image) {
    if (!image.is_valid() ||
        image.format() != poppler::image::format_rgb24) {
        throw std::runtime_error("Image must be RGB24");
    }

    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = image.width();
    png.height = image.height();
    png.format = PNG_FORMAT_RGB;

    png_alloc_size_t size = 0;

    if (!png_image_write_to_memory(
            &png, nullptr, &size, 0,
            image.const_data(), image.bytes_per_row(), nullptr)) {
        throw std::runtime_error("Cannot calculate PNG size");
    }

    std::vector<unsigned char> buffer(size);

    if (!png_image_write_to_memory(
            &png, buffer.data(), &size, 0,
            image.const_data(), image.bytes_per_row(), nullptr)) {
        throw std::runtime_error("Cannot encode PNG");
    }

    buffer.resize(size);
    return buffer;
}

size_t responseCallback(
    char* data, size_t size, size_t count, void* userData
) {
    auto* response = static_cast<std::string*>(userData);
    size_t bytes = size * count;
    response->append(data, bytes);
    return bytes;
}


std::string sendToAI(const poppler::image& image,const std::string& prompt) {
    const char* apiKey = "*redacted*";
    if (!apiKey || !*apiKey) {
        throw std::runtime_error("AI_API_KEY is not set");
    }

    // 1. Изображение -> PNG -> Base64
    std::vector<unsigned char> png = imageToPng(image);
    std::string base64 = encodeBase64(png.data(), png.size());

    // 2. Формируем JSON
    nlohmann::json body = {
        {"model", "qwen/qwen3-vl-30b-a3b-instruct"},
        {"messages", nlohmann::json::array({
            {
                {"role", "user"},
                {"content", nlohmann::json::array({
                    {
                        {"type", "text"},
                        {"text", prompt}
                    },
                    {
                        {"type", "image_url"},
                        {"image_url", {
                            {"url", "data:image/png;base64," + base64}
                        }}
                    }
                })}
            }
        })},
        {"max_tokens", 6000}
    };

    std::string requestBody = body.dump();
    std::string response;

    // 3. Создаём HTTP-запрос
    using CurlPtr = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

    CurlPtr curl(curl_easy_init(), &curl_easy_cleanup);

    if (!curl) {
        throw std::runtime_error("Cannot initialize CURL");
    }

    curl_slist* headers = nullptr;

    std::string auth = std::string("Authorization: Bearer ") + apiKey;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth.c_str());

    curl_easy_setopt(
        curl.get(), CURLOPT_URL,
        "https://changellenge.up.railway.app/v1/chat/completions"
    );
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(
        curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(requestBody.size())
    );
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, responseCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 120L);

    // 4. Отправляем запрос и ждём ответ
    CURLcode result = curl_easy_perform(curl.get());

    long status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headers);

    if (result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }

    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "AI HTTP " + std::to_string(status) + ": " + response
        );
    }

    // 5. Извлекаем текст ответа
    nlohmann::json json = nlohmann::json::parse(response);

    return json.at("choices")
               .at(0)
               .at("message")
               .at("content")
               .get<std::string>();
}



void dbWorker::ExtractData() {
    if (!isConnected()) {
        throw std::runtime_error("Database is not connected");
    }

    int64_t documentId;

    {
        pqxx::read_transaction tx(*connection);
        auto result = tx.exec(
            "SELECT id FROM pdf_documents LIMIT 1"
        );
        if (result.empty()) {
            throw std::runtime_error("No PDF documents found");
        }
        documentId = result[0]["id"].as<int64_t>();
    }

std::vector<poppler::image> images = pdfToImages(documentId);
std::string prompt = R"PROMPT(
Ответь как специалист по аналитике новостей. Проанализируй страницу с новостями и извлеки из неё ключевые новости, идеи и события.

Верни строго JSON-массив объектов следующего формата:

[
  {
    "title": "Краткое описание новости",
    "text": "Полный текст новости с необходимой информацией о событии или действии",
    "finance": 75,
    "buisness": 60,
    "global": 30
  }
]

Правила:

1. Для каждой отдельной новости создавай отдельный объект.
2. title — краткий заголовок новости.
3. text — подробное описание новости со всеми доступными важными фактами.
4. finance — значимость для финансовых аналитиков от 0 до 100.
5. buisness — значимость для бизнес-аналитиков от 0 до 100.
6. global — значимость для всего рынка от 0 до 100. Высокие оценки ставь только событиям, влияющим на рынок в целом.
7. Все оценки должны быть целыми числами, а не строками.
8. Не придумывай факты, которых нет на странице.
9. Если информация неразборчива, пропусти её.
10. Если значимых новостей нет, верни [].
11. Возвращай исключительно валидный JSON без Markdown и пояснений.
)PROMPT";


std::vector<std::string> aiResponses;
aiResponses.reserve(images.size());

bool allPagesProcessed = !images.empty();

for (size_t i = 0; i < images.size(); ++i) {
    std::cout << "Sending page "
              << i + 1 << "/" << images.size()
              << std::endl;

    try {
        std::string answer = sendToAI(images[i], prompt);

        // Проверяем JSON сразу, но пока ничего не сохраняем
        nlohmann::json parsed = nlohmann::json::parse(answer);

        if (!parsed.is_array()) {
            throw std::runtime_error("AI response must be a JSON array");
        }

        aiResponses.push_back(std::move(answer));

        std::cout << "Page " << i + 1
                  << " processed successfully"
                  << std::endl;
    }
    catch (const std::exception& e) {
        allPagesProcessed = false;

        std::cerr << "Error on page "
                  << i + 1 << ": "
                  << e.what()
                  << std::endl;

        break;
    }
}

if (!allPagesProcessed) {
    std::cerr << "PDF not deleted. No news saved.\n";
    return;
}

// Сохраняем все страницы одной транзакцией
saveAIResponse(aiResponses);

// Удаляем PDF только после успешного сохранения
deletePdf(documentId);

std::cout << "PDF processed successfully!\n";
}