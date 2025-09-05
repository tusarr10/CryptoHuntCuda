#pragma once
#include <string>

class TelegramAlert {
public:
    // Core send method (supports MarkdownV2 or HTML)
    static void sendMessage(const std::string& token,
        const std::string& chatId,
        const std::string& message,
        const std::string& parseMode = "MarkdownV2");

    // Alerts
    static void sendFoundKeyAlertHTML(const std::string& token,
        const std::string& chatId,
        const std::string& hexKey,
        const std::string& wif,
        const std::string& p2pkh,
        const std::string& p2sh,
        const std::string& bech32);

    static void sendFoundKeyAlert(const std::string& token,
        const std::string& chatId,
        const std::string& hexKey,
        const std::string& wifCompressed,
        const std::string& p2pkh,
        const std::string& p2sh,
        const std::string& bech32);

    static void sendHighTempAlert(const std::string& token,
        const std::string& chatId,
        float temp);

    static void sendProgressUpdate(const std::string& token,
        const std::string& chatId,
        double mkeysPerSec,
        double progress);

    static void sendPeriodicTelegramUpdate();
    static std::string getCurrentTime();

    // --- Config ---
   /* static const std::string TELEGRAM_BOT_TOKEN;
    static const std::string TELEGRAM_CHAT_ID;
    static const bool TELEGRAM_ENABLED;
    static const bool ALERT_HIGH_TEMP;
    static const float TEMP_THRESHOLD;
    static const bool PROGRESS_UPDATES;
    static const int PROGRESS_INTERVAL_MINUTES;*/

private:
   
    static std::string urlEncode(const std::string& value);
    static std::string escapeMarkdown(const std::string& text);
    static std::string encodeToUTF8(const std::string& text);
   static  void logTelegramStatus(const std::string& status, const std::string& messageBody, const std::string& error, const std::string& currentStatus);
    static time_t lastPeriodicSend;
};
