#include "Settings.h"
#include <fstream>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

Settings Settings::instance; // define static instance

void Settings::LoadFromFile(const std::string& path) {
    Settings s;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[WARN] Config file not found. Creating default " << path << "\n";

        // Create defaults
        s.telegram.enabled = true;
        s.telegram.botToken = "8210902730:AAEMwfdlTiZ1APZhM7TTI6RSoYt7FNxu1is";
        s.telegram.chatId = "7374027384";
        s.telegram.alertHighTemp = true;
        s.telegram.tempThreshold = 80.0f;
        s.telegram.progressUpdates = true;
        s.telegram.progressIntervalMinutes = 1;

        s.server.host = "127.0.0.1";
        s.server.port = 9090;
        s.server.enableApi = true;

        s.app.gpuEnable = true;
        s.app.gpuAutoGrid = false;
        s.app.compMode = 1;
        s.app.coinType = 0;
        s.app.searchMode = 1;
        s.app.maxFound = 10000;
        s.app.inputFile = "input.txt";
        s.app.outputFile = "results.txt";

        s.status.resume = false;
        s.status.resumeFile = "resume.json";
        s.status.maxStatusFileSize = 10485760;

        // Save defaults
        json j = {
            {"telegram", {
                {"enabled", s.telegram.enabled},
                {"botToken", s.telegram.botToken},
                {"chatId", s.telegram.chatId},
                {"alertHighTemp", s.telegram.alertHighTemp},
                {"tempThreshold", s.telegram.tempThreshold},
                {"progressUpdates", s.telegram.progressUpdates},
                {"progressIntervalMinutes", s.telegram.progressIntervalMinutes}
            }},
            {"server", {
                {"host", s.server.host},
                {"port", s.server.port},
                {"enableApi", s.server.enableApi}
            }},
            {"app", {
                {"gpuEnable", s.app.gpuEnable},
                {"gpuAutoGrid", s.app.gpuAutoGrid},
                {"compMode", s.app.compMode},
                {"coinType", s.app.coinType},
                {"searchMode", s.app.searchMode},
                {"maxFound", s.app.maxFound},
                {"inputFile", s.app.inputFile},
                {"outputFile", s.app.outputFile}
            }},
            {"status", {
                {"resume", s.status.resume},
                {"resumeFile", s.status.resumeFile},
                {"maxStatusFileSize", s.status.maxStatusFileSize}
            }}
        };

        std::ofstream out(path);
        out << j.dump(4);
        return;
    }

    // ✅ If file exists → load it
    try {
        json j;
        f >> j;
        f.close();
        if (j.contains("telegram")) {
            auto t = j["telegram"];
            instance.telegram.enabled = t.value("enabled", instance.telegram.enabled);
            instance.telegram.botToken = t.value("botToken", instance.telegram.botToken);
            instance.telegram.chatId = t.value("chatId", instance.telegram.chatId);
            instance.telegram.alertHighTemp = t.value("alertHighTemp", instance.telegram.alertHighTemp);
            instance.telegram.tempThreshold = t.value("tempThreshold", instance.telegram.tempThreshold);
            instance.telegram.progressUpdates = t.value("progressUpdates", instance.telegram.progressUpdates);
            instance.telegram.progressIntervalMinutes = t.value("progressIntervalMinutes", instance.telegram.progressIntervalMinutes);
        }

        if (j.contains("server")) {
            auto s = j["server"];
            instance.server.host = s.value("host", instance.server.host);
            instance.server.port = s.value("port", instance.server.port);
            instance.server.enableApi = s.value("enableApi", instance.server.enableApi);
        }

        if (j.contains("app")) {
            auto a = j["app"];
            instance.app.gpuEnable = a.value("gpuEnable", instance.app.gpuEnable);
            instance.app.gpuAutoGrid = a.value("gpuAutoGrid", instance.app.gpuAutoGrid);
            instance.app.compMode = a.value("compMode", instance.app.compMode);
            instance.app.coinType = a.value("coinType", instance.app.coinType);
            instance.app.searchMode = a.value("searchMode", instance.app.searchMode);
            instance.app.maxFound = a.value("maxFound", instance.app.maxFound);
            instance.app.inputFile = a.value("inputFile", instance.app.inputFile);
            instance.app.outputFile = a.value("outputFile", instance.app.outputFile);
        }

        if (j.contains("status")) {
            auto st = j["status"];
            instance.status.resume = st.value("resume", instance.status.resume);
            instance.status.resumeFile = st.value("resumeFile", instance.status.resumeFile);
            instance.status.maxStatusFileSize = st.value("maxStatusFileSize", instance.status.maxStatusFileSize);
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse " << path << ": " << e.what() << "\n";
    }
}
    
    Settings& Settings::Get() {
        return instance;
}
