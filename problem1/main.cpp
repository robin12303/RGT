#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class LogFileManager {
public:
    LogFileManager() = default;
    ~LogFileManager() = default;

    // 복사 생성자: 열린 파일 목록(파일명들)만 복사해서 새로 reopen
    LogFileManager(const LogFileManager& other) {
        // strong exception safety: 임시 맵 만들고 성공하면 swap
        FileMap temp;
        temp.reserve(other.files_.size());
        for (const auto& [filename, _] : other.files_) {
            auto ofs = std::make_unique<std::ofstream>(filename, std::ios::app);
            if (!ofs->is_open()) {
                throw std::runtime_error("Copy failed: cannot open file '" + filename + "'");
            }
            temp.emplace(filename, std::move(ofs));
        }
        files_.swap(temp);
    }

    // 복사 대입: copy-and-swap 패턴
    LogFileManager& operator=(const LogFileManager& other) {
        if (this == &other) return *this;
        LogFileManager temp(other);
        files_.swap(temp.files_);
        return *this;
    }

    // 이동은 기본으로 OK (unique_ptr + unordered_map)
    LogFileManager(LogFileManager&&) noexcept = default;
    LogFileManager& operator=(LogFileManager&&) noexcept = default;

public:
    void openLogFile(const std::string& filename) {
        if (filename.empty()) {
            throw std::invalid_argument("openLogFile: filename is empty");
        }
        if (files_.find(filename) != files_.end()) {
            throw std::runtime_error("openLogFile: file already opened '" + filename + "'");
        }

        auto ofs = std::make_unique<std::ofstream>(filename, std::ios::app);
        if (!ofs->is_open()) {
            throw std::runtime_error("openLogFile: cannot open file '" + filename + "'");
        }

        files_.emplace(filename, std::move(ofs));
    }

    void writeLog(const std::string& filename, const std::string& message) {
        auto it = files_.find(filename);
        if (it == files_.end() || !it->second) {
            throw std::runtime_error("writeLog: file not opened '" + filename + "'");
        }

        std::ofstream& out = *(it->second);
        if (!out.good()) {
            throw std::runtime_error("writeLog: stream is not good for '" + filename + "'");
        }

        out << "[" << nowTimestamp() << "] " << message << "\n";
        if (!out.good()) {
            throw std::runtime_error("writeLog: write failed for '" + filename + "'");
        }
        out.flush();
    }

    std::vector<std::string> readLogs(const std::string& filename) const {
        std::ifstream in(filename);
        if (!in.is_open()) {
            throw std::runtime_error("readLogs: cannot open file '" + filename + "'");
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }

        if (in.bad()) {
            throw std::runtime_error("readLogs: read failed for '" + filename + "'");
        }
        return lines;
    }

    void closeLogFile(const std::string& filename) {
        auto it = files_.find(filename);
        if (it == files_.end()) {

            throw std::runtime_error("closeLogFile: file not opened '" + filename + "'");
        }

        files_.erase(it);
    }

private:
    using FileMap = std::unordered_map<std::string, std::unique_ptr<std::ofstream>>;
    FileMap files_;

private:
    static std::string nowTimestamp() {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        std::time_t t = clock::to_time_t(now);

        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};


int main() {
    try {
        LogFileManager manager;
        manager.openLogFile("error.log");
        manager.openLogFile("debug.log");
        manager.openLogFile("info.log");

        manager.writeLog("error.log", "Database connection failed");
        manager.writeLog("debug.log", "User login attempt");
        manager.writeLog("info.log", "Server started successfully");

        auto errorLogs = manager.readLogs("error.log");
        if (!errorLogs.empty()) {
            std::cout << "errorLogs[0] = \"" << errorLogs[0] << "\"\n";
        }

        manager.closeLogFile("error.log");
        manager.closeLogFile("debug.log");
        manager.closeLogFile("info.log");
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
