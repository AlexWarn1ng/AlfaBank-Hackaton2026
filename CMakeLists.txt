cmake_minimum_required(VERSION 3.8)
project(main CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

add_executable(main 
pdfworker.h
dbworker.h
main.cpp
pdfworker.cpp
dbworker.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libpqxx::pqxx
    PkgConfig::POPPLER_CPP
)