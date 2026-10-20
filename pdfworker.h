#include <mutex>
#include <memory>
 #include <filesystem>

class pdfWorker {
public:
    static pdfWorker* getinst() {
        std::lock_guard<std::mutex> lock(sinMut);
        if(inst == nullptr){
            inst.reset(new pdfWorker);
        }
        return inst.get();
        
    }


void checkDir() {
    std::filesystem::path dataPath = std::filesystem::current_path() / "data";
    std::filesystem::create_directories(dataPath);
}

~pdfWorker() = default;
pdfWorker(const pdfWorker &other) = delete;
pdfWorker(pdfWorker &&other) = delete;
pdfWorker& operator=(const pdfWorker&) = delete;
pdfWorker& operator=(pdfWorker&&) = delete;   

private:
inline static std::mutex sinMut;
inline static std::unique_ptr<pdfWorker> inst = nullptr;
pdfWorker() = default;

};