#include "TelegramAlert.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

#include "json.hpp"
#include "SystemMonitor.h"

// ---------------- Static Config ----------------
time_t TelegramAlert::lastPeriodicSend = 0;
const std::string TelegramAlert::TELEGRAM_BOT_TOKEN = "8210902730:AAEMwfdlTiZ1APZhM7TTI6RSoYt7FNxu1is";
const std::string TelegramAlert::TELEGRAM_CHAT_ID = "7374027384";
const bool TelegramAlert::TELEGRAM_ENABLED = true;
const bool TelegramAlert::ALERT_HIGH_TEMP = true;
const float TelegramAlert::TEMP_THRESHOLD = 80.0f;
const bool TelegramAlert::PROGRESS_UPDATES = true;
const int TelegramAlert::PROGRESS_INTERVAL_MINUTES = 1;

// ---------------- Helpers ----------------
std::string TelegramAlert::getCurrentTime() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string TelegramAlert::escapeMarkdown(const std::string& text) {
    static const std::string specials = "_*[]()~`>#+-=|{}.!";
    std::string escaped;
    for (char c : text) {
        if (specials.find(c) != std::string::npos) escaped += '\\';
        escaped += c;
    }
    return escaped;
}

std::string TelegramAlert::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            escaped << c;
        else
            escaped << '%' << std::setw(2) << int(c);
    }
    return escaped.str();
}

// Ensure UTF-8 safe string (replace invalid chars)
std::string TelegramAlert::encodeToUTF8(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        if (c < 128) out += c;
        else out += '?';
    }
    return out;
}

// ---------------- Core Telegram Send ----------------
void TelegramAlert::sendMessage(const std::string& token,
    const std::string& chatId,
    const std::string& message,
    const std::string& parseMode) {
    std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";
    std::string tempFile = "telegram_msg_utf8.txt";

    // Write message to file
    std::ofstream out(tempFile, std::ios::binary);
    out << encodeToUTF8(message);
    out.close();

    std::ostringstream cmd;
    cmd << "curl -s -X POST \"" << url << "\" "
        << "--data-urlencode \"chat_id=" << chatId << "\" "
        << "--data-urlencode \"parse_mode=" << parseMode << "\" "
        << "--data-urlencode \"disable_web_page_preview=true\" "
        << "--data-urlencode text@" << tempFile;

    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        std::cerr << "[" << getCurrentTime() << "] Failed to execute curl!\n";
        return;
    }

    char buffer[512];
    std::string response;
    while (fgets(buffer, sizeof(buffer), pipe)) response += buffer;
    pclose(pipe);

    remove(tempFile.c_str());

    if (response.find("\"ok\":false") != std::string::npos) {
        std::cerr << "[" << getCurrentTime() << "] Telegram API error: " << response << "\n";
    }
    else {
        std::cout << "[" << getCurrentTime() << "] Telegram message sent successfully.\n";
    }
}

// ---------------- Alerts ----------------
void TelegramAlert::sendFoundKeyAlert(const std::string& token,
    const std::string& chatId,
    const std::string& hexKey,
    const std::string& wifCompressed,
    const std::string& p2pkh,
    const std::string& p2sh,
    const std::string& bech32) {
    std::ostringstream oss;
    oss << "✅ *KEY FOUND!*\n\n"
        << "🔑 *Private Key (HEX):*\n`" << hexKey << "`\n\n"
        << "🔐 *WIF (Compressed):*\n`" << wifCompressed << "`\n\n"
        << "🧾 *Bitcoin Addresses:*\n"
        << "• P2PKH: `" << p2pkh << "`\n"
        << "• P2SH: `" << p2sh << "`\n"
        << "• Bech32: `" << bech32 << "`\n\n"
        << "⏱ *Time:* " << getCurrentTime();
    sendMessage(token, chatId, escapeMarkdown(oss.str()), "MarkdownV2");
}

void TelegramAlert::sendFoundKeyAlertHTML(const std::string& token,
    const std::string& chatId,
    const std::string& hexKey,
    const std::string& wif,
    const std::string& p2pkh,
    const std::string& p2sh,
    const std::string& bech32) {
    std::ostringstream oss;
    oss << "<b>🎯 Match Found!</b>\n"
        << "🕓 <b>Time:</b> <code>" << getCurrentTime() << "</code>\n"
        << "🧠 <b>Hex Key:</b> <code>" << hexKey << "</code>\n"
        << "🔐 <b>Private Key (WIF):</b> <code>" << wif << "</code>\n\n"
        << "🏦 <b>Legacy:</b> <a href=\"https://www.blockchain.com/explorer/addresses/btc/" << p2pkh << "\">" << p2pkh << "</a>\n"
        << "🧾 <b>P2SH:</b> <a href=\"https://www.blockchain.com/explorer/addresses/btc/" << p2sh << "\">" << p2sh << "</a>\n"
        << "📬 <b>SegWit:</b> <a href=\"https://www.blockchain.com/explorer/addresses/btc/" << bech32 << "\">" << bech32 << "</a>\n";

    sendMessage(token, chatId, oss.str(), "HTML");
}

void TelegramAlert::sendHighTempAlert(const std::string& token,
    const std::string& chatId,
    float temp) {
    std::ostringstream oss;
    oss << "🔥 *HIGH GPU TEMPERATURE!*\n\n"
        << "🌡️ Current: *" << static_cast<int>(temp) << "°C*\n\n"
        << "🛑 Consider pausing or improving cooling.";
    sendMessage(token, chatId, escapeMarkdown(oss.str()), "MarkdownV2");
}

void TelegramAlert::sendProgressUpdate(const std::string& token,
    const std::string& chatId,
    double mkeysPerSec,
    double progress) {
    char buf[512];
    snprintf(buf, sizeof(buf), "📊 *Search Progress Update*\n\n⚡ Speed: *%.2f Mk/s*\n📈 Progress: *%.2f%%*", mkeysPerSec, progress);
    sendMessage(token, chatId, escapeMarkdown(buf), "MarkdownV2");
}

// ---------------- Periodic Update ----------------
void TelegramAlert::sendPeriodicTelegramUpdate() {
   /* time_t now = time(nullptr);
    if (lastPeriodicSend != 0 && now - lastPeriodicSend < PROGRESS_INTERVAL_MINUTES * 60) return;
    lastPeriodicSend = now;

    std::ifstream f("status.json");
    if (!f) return;

    nlohmann::json root;
    try { f >> root; }
    catch (...) { return; }

    std::string sysId = SystemMonitor::getSystemIdentifier();
    if (!root.contains("systems") || !root["systems"].contains(sysId)) return;

    auto& sys = root["systems"][sysId];
    uint64_t scannedKeys = sys["progress"].value("keys_scanned_raw", 0ULL);
    double progress = sys["progress"].value("progress_percent", 0.0);
    double mkeysRate = sys["progress"].value("mkeys_per_second", 0.0);
    int found = sys["progress"].value("found_count", 0);

    std::ostringstream oss;
    oss << "📊 *CryptoHunt Status Update* \\- `" << escapeMarkdown(sysId) << "`\n\n"
        << "🔢 Keys Scanned: *" << scannedKeys << "*\n"
        << "📈 Progress: *" << std::fixed << std::setprecision(2) << progress << "%*\n"
        << "⚡ Speed: *" << mkeysRate << " Mk/s*\n"
        << "✅ Found Keys: *" << found << "*\n";

    sendMessage(TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, oss.str(), "HTML");*/
    time_t now = time(nullptr);

    // Only send every N minutes
    if (lastPeriodicSend != 0 &&
        now - lastPeriodicSend < PROGRESS_INTERVAL_MINUTES * 60)
        return;
    lastPeriodicSend = now;

    // Load status.json
    std::ifstream f("status.json");
    if (!f) return;

    nlohmann::json root;
    try { f >> root; }
    catch (...) { return; }

    // Get current node system ID
    std::string sysId = SystemMonitor::getSystemIdentifier();

    if (!root.contains("systems") || !root["systems"].contains(sysId))
        return;

    auto& sys = root["systems"][sysId];

    // Extract values
    uint64_t totalKeys = sys["progress"].value("keys_scanned_raw", 0ULL);
    double   progress = sys["progress"].value("progress_percent", 0.0);
    double   mkeysRate = sys["progress"].value("mkeys_per_second", 0.0);
    int      found = sys["progress"].value("found_count", 0);

    auto cpu = sys["system"]["cpu"];
    auto gpu = sys["system"]["gpu"];
    auto mem = sys["system"]["memory"];

    // Escape MarkdownV2 special characters
    auto escapeMarkdown = [](const std::string& input) -> std::string {
        std::string out;
        std::string specials = "_*[]()~`>#+-=|{}.!";

        for (char c : input) {
            if (specials.find(c) != std::string::npos) out.push_back('\\');
            out.push_back(c);
        }
        return out;
        };

    // Format MarkdownV2 message
    std::ostringstream oss;
    oss << "📊 *CryptoHunt Status Update* \\- `" << escapeMarkdown(sysId) << "`\n\n"
        << "🔢 Keys Scanned: *" << totalKeys << "*\n"
        << "📈 Progress: *" << std::fixed << std::setprecision(2) << progress << "%*\n"
        << "⚡ Speed: *" << mkeysRate << " Mk/s*\n"
        << "✅ Found Keys: *" << found << "*\n\n"
        << "🖥️ CPU: " << cpu.value("usage_percent", 0.0) << "% | "
        << cpu.value("temp_c", 0.0) << "°C | "
        << "Cores: " << cpu.value("cores", 0) << "\n"
        << "🎮 GPU: " << gpu.value("usage_percent", 0.0) << "% | "
        << gpu.value("temp_c", 0.0) << "°C | "
        << "Fan: " << gpu.value("fan_percent", 0.0) << "% | "
        << "Power: " << gpu.value("power_w", 0.0) << " W | "
        << "Clock: " << gpu.value("clock_mhz", 0) << " MHz\n"
        << "💾 RAM: " << mem.value("ram_used_mb", 0) << " / "
        << mem.value("ram_total_mb", 0) << " MB ("
        << mem.value("ram_usage_percent", 0.0) << "%)\n";

    std::string message = oss.str();
    std::cout << "[DEBUG] Markdown message:\n" << message << std::endl;

    // Send message using UTF-8 safe file approach
    sendMessage(TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, oss.str(),"HTML");
}
