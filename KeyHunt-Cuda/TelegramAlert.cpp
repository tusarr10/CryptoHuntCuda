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
#include "Settings.h"

// ---------------- Static Config ----------------
time_t TelegramAlert::lastPeriodicSend = 0;
//const std::string TelegramAlert::TELEGRAM_BOT_TOKEN = "8210902730:AAEMwfdlTiZ1APZhM7TTI6RSoYt7FNxu1is";
//const std::string TelegramAlert::TELEGRAM_CHAT_ID = "7374027384";
//const bool TelegramAlert::TELEGRAM_ENABLED = true;
//const bool TelegramAlert::ALERT_HIGH_TEMP = true;
//const float TelegramAlert::TEMP_THRESHOLD = 80.0f;
//const bool TelegramAlert::PROGRESS_UPDATES = true;
//const int TelegramAlert::PROGRESS_INTERVAL_MINUTES = 1;

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



void TelegramAlert::logTelegramStatus(const std::string& status, const std::string& messageBody, const std::string& error, const std::string& currentStatus)
{
	nlohmann::json root;

	// --- Step 1: Load existing file ---
	try {
		std::ifstream in("telegramstatus.json");
		if (in.is_open()) {
			in >> root;
			in.close();
		}
	}
	catch (...) {
		root = nlohmann::json::object();
	}

	// --- Step 2: System info ---
	std::string sysName = SystemMonitor::getSystemName();
	std::string sysId = SystemMonitor::getSystemMAC();

	// --- Step 3: Prepare new entry ---
	nlohmann::json entry;
	entry["time"] = TelegramAlert::getCurrentTime();
	entry["status"] = status;
	entry["message"] = encodeToUTF8(messageBody);
	entry["error"] = encodeToUTF8(error);
	entry["current_status"] = currentStatus;

	// --- Step 4: Append entry ---
	auto& logArray = root["systems"][sysId][sysName];
	logArray.push_back(entry);

	// --- Step 5: Enforce file size limit (10 MB) ---
	const size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
	std::string tempDump = root.dump();

	if (tempDump.size() > MAX_FILE_SIZE) {
		// First remove oldest "success" entries
		for (auto it = logArray.begin(); it != logArray.end() && tempDump.size() > MAX_FILE_SIZE;) {
			if ((*it)["status"] == "success") {
				it = logArray.erase(it);
				tempDump = root.dump();
			}
			else {
				++it;
			}
		}

		// If still too big, drop oldest entries (including errors) until under limit
		while (tempDump.size() > MAX_FILE_SIZE && logArray.size() > 1) {
			logArray.erase(logArray.begin());
			tempDump = root.dump();
		}
	}

	// --- Step 6: Save file ---
	try {
		std::ofstream out("telegramstatus.json", std::ios::trunc);
		if (!out.is_open()) {
			std::cerr << "[TelegramAlert][WARN] Could not open telegramstatus.json for writing\n";
			return;
		}
		out << root.dump(2);
		out.close();
	}
	catch (const std::exception& e) {
		std::cerr << "[TelegramAlert][ERROR] Failed to write telegramstatus.json: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "[TelegramAlert][ERROR] Unknown failure writing telegramstatus.json\n";
	}
}


//---------------------------------------------------------------------------------------
//------------------END Helper---------------------------------------------
//-----------------------------------------------------------------------


// ---------------- Core Telegram Send ----------------
void TelegramAlert::sendMessage(const std::string& token, const std::string& chatId, const std::string& message, const std::string& parseMode) {
	
		std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";
		std::string tempFile = "telegram_msg_utf8.txt";
		
			if (!Settings::Get().telegram.enabled) {
				return;  // 🚀 Do nothing if disabled
			}
		
		try {
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
				//std::cerr << "[" << getCurrentTime() << "] Telegram API error: " << response << "\n";
				logTelegramStatus("error", message, "Telegram API returned error: " + response, "failed");
			}
			else {
				//std::cout << "[" << getCurrentTime() << "] Telegram message sent successfully.\n";
				logTelegramStatus("", message, "success", "sent");
			}
		}
		catch (const std::exception& e) {
			logTelegramStatus("error", message, e.what(), "failed");
		}
		catch (...) {
			logTelegramStatus("error", message, "unknown exception", "failed");
		}
	
	
}
// ---------------- Alerts ----------------
void TelegramAlert::sendFoundKeyAlert(const std::string& token, const std::string& chatId, const std::string& hexKey, const std::string& wifCompressed, const std::string& p2pkh, const std::string& p2sh, const std::string& bech32) {
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

void TelegramAlert::sendFoundKeyAlertHTML(const std::string& token, const std::string& chatId, const std::string& hexKey, const std::string& wif, const std::string& p2pkh, const std::string& p2sh, const std::string& bech32) {
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

void TelegramAlert::sendHighTempAlert(const std::string& token,	const std::string& chatId,	float temp) {
	std::ostringstream oss;
	oss << "🔥 *HIGH GPU TEMPERATURE!*\n\n"
		<< "🌡️ Current: *" << static_cast<int>(temp) << "°C*\n\n"
		<< "🛑 Consider pausing or improving cooling.";
	sendMessage(token, chatId, escapeMarkdown(oss.str()), "MarkdownV2");
}

void TelegramAlert::sendProgressUpdate(const std::string& token,	const std::string& chatId,	double mkeysPerSec,	double progress) {
	char buf[512];
	snprintf(buf, sizeof(buf), "📊 *Search Progress Update*\n\n⚡ Speed: *%.2f Mk/s*\n📈 Progress: *%.2f%%*", mkeysPerSec, progress);
	sendMessage(token, chatId, escapeMarkdown(buf), "MarkdownV2");
}

// ---------------- Periodic Update ----------------
void TelegramAlert::sendPeriodicTelegramUpdate() {
	if (!Settings::Get().telegram.enabled) {		
			return;  // 🚀 Do nothing if disabled
		}
	
	
		time_t now = time(nullptr);

		// Only send every N minutes
		if (lastPeriodicSend != 0 &&
			now - lastPeriodicSend < static_cast<long long>(Settings::Get().telegram.progressIntervalMinutes) * 60)
			return;
		lastPeriodicSend = now;

		// Load status.json
		nlohmann::json root;
		std::string sysId;
		std::ostringstream oss;
		std::string Status_message;  // will hold final message (error or success)

		// --- Step 1: Open file ---
		try {
			std::ifstream f("status.json");
			if (!f.is_open()) {
				Status_message = "[FileError] Could not open status.json";

			}
			f >> root;
		}
		catch (const std::exception& e) {
			Status_message = std::string("[JSONParseError] Failed to parse status.json: ") + e.what();

		}
		catch (...) {
			Status_message = "[JSONParseError] Unknown exception parsing status.json";
		}

		// Get current node system ID
		 // --- Step 2: Get system ID ---
		try {
			sysId = SystemMonitor::getSystemIdentifier();
		}
		catch (const std::exception& e) {
			Status_message = std::string("[SystemIDError] Failed to get system identifier: ") + e.what();
		}
		catch (...) {
			Status_message = "[SystemIDError] Unknown exception getting system identifier";
		}

		// --- Step 3: Check system data ---
		if (!root.contains("systems") || !root["systems"].contains(sysId)) {
			Status_message = "[DataMissing] No entry for system ID in status.json (" + sysId + ")";

		}

		auto& sys = root["systems"][sysId];

		// --- Step 4: Extract values safely ---
		try {
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

			Status_message = oss.str();
			//std::cout << "[DEBUG] Markdown message:\n" << Status_message << std::endl;

		}
		catch (const std::exception& e) {
			Status_message = std::string("[DataExtractError] Failed extracting system data: ") + e.what();

		}
		catch (...) {
			Status_message = "[DataExtractError] Unknown error extracting system data";
		}
		//Status_message += "\n\n⏱ *Time:* " + getCurrentTime();
		//Status_message += Settings::Get().telegram.botToken +" CID = " + Settings::Get().telegram.chatId;
		//std::cout << "[DEBUG] Markdown message:\n" << Status_message << std::endl;
		// Send message using UTF-8 safe file approach
		sendMessage(Settings::Get().telegram.botToken, Settings::Get().telegram.chatId, Status_message, "HTML"); //or status message
	
}
