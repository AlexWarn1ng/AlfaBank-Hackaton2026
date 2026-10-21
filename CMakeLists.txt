cmake_minimum_required(VERSION 3.8)
project(main CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(PNG REQUIRED)

add_executable(main 
pdfworker.h
customdatastructures.h
dbworker.h
company.h
main.cpp
pdfworker.cpp
dbworker.cpp
company.cpp
dataextractor.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libpqxx::pqxx
    Crow::Crow
    PkgConfig::POPPLER_CPP
    nlohmann_json::nlohmann_json
    CURL::libcurl
    PNG::PNG
)
