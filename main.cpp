#include "pdfworker.h"
#include "dbworker.h"
#include <iostream>
#include <exception>
#include <memory>
#include <iostream>
#include <utility>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "company.h"
#include <crow.h>

int main() {

        crow::SimpleApp app;
        auto PdfWorkerPtr = pdfWorker::getinst();
        auto DbWorkerPtr = dbWorker::getinst();

        DbWorkerPtr->connect(
        "host=*redacted* "
        "port=*redacted* "
        "dbname=*redacted* "
        "user=*redacted* "
        "password=*redacted* "
        );

        DbWorkerPtr->createTables();

        std::cout << "Import completed!\n";
        std::cout << "Database connected, tables created\n";

        //DbWorkerPtr->importPdfs(std::filesystem::current_path() / "data");
        //DbWorkerPtr->importCsv(std::filesystem::current_path() / "companiesdata");
        //DbWorkerPtr->loadPdf(1, "restored.pdf");
        //DbWorkerPtr->pdfToImages(1);


    CROW_ROUTE(app, "/getcompanyobject/<string>")
    ([DbWorkerPtr](std::string uid) {
     try {
            auto myCompany = DbWorkerPtr->getCompany(uid);

         if (myCompany == nullptr) {
               return crow::response(404, "Company not found");
           }

           myCompany->loadLinks(*DbWorkerPtr);
            myCompany->loadPayments(*DbWorkerPtr);

          nlohmann::json json = myCompany->toJson();

           crow::response res(200, json.dump(4));
          res.set_header("Content-Type", "application/json; charset=utf-8");

          return res;
       }
     catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
          return crow::response(500, e.what());
      }

    });

    app.port(8888).concurrency(1).run();

    system("pause");
    return 0;

}