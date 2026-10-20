#include "pdfworker.h"
#include "dbworker.h"
#include <iostream>
#include <exception>

int main() {
    try {
        auto PdfWorkerPtr = pdfWorker::getinst();
        auto DbWorkerPtr = dbWorker::getinst();

        DbWorkerPtr->connect(
            "host=localhost "
            "port=*redacted* "
            "dbname=main "
            "user=user "
            "password=123"
        );

        DbWorkerPtr->createTables();

        DbWorkerPtr->importPdfs(std::filesystem::current_path() / "data");
        // DbWorkerPtr->loadPdf(1, "restored.pdf");
        DbWorkerPtr->pdfToImages(21);
        std::cout << "Import completed!\n";
        std::cout << "Database connected, tables created\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    system("pause");
    return 0;
}