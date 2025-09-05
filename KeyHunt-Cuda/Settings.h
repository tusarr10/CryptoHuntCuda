#pragma once
#include <string>
#include "json.hpp"

class Settings {
public:
    struct Telegram {
        bool enabled = true;
        std::string botToken;
        std::string chatId;
        bool alertHighTemp = true;
        float tempThreshold = 80.0f;
        bool progressUpdates = true;
        int progressIntervalMinutes = 1;

        void load(const nlohmann::json& j);
        nlohmann::json toJson() const;
    };

    struct Server {
        std::string host = "127.0.0.1";
        int port = 8080;
        bool enableApi = false;

        void load(const nlohmann::json& j);
        nlohmann::json toJson() const;
    };

    struct App {
        bool gpuEnable = false;
        bool gpuAutoGrid = true;
        int compMode = 0;
        int coinType = 0;
        int searchMode = 0;
        int maxFound = 65536;
        std::string inputFile;
        std::string outputFile = "Found.txt";

        void load(const nlohmann::json& j);
        nlohmann::json toJson() const;
    };

    struct Status {
        bool resume = false;
        std::string resumeFile = "resume.json";
        size_t maxStatusFileSize = 10 * 1024 * 1024; // 10 MB

        void load(const nlohmann::json& j);
        nlohmann::json toJson() const;
    };

    Telegram telegram;
    Server server;
    App app;
    Status status;
    
    static void LoadFromFile(const std::string& file);
    static Settings& Get();                     // Global access
    nlohmann::json toJson() const;
private:
    
    static Settings instance;
};
