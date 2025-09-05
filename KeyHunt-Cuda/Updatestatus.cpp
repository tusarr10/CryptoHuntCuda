
#include "Updatestatus.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>
#include <vector>
#include <iomanip>
#include "json.hpp"
#include "KeyHunt.h"
#include "Settings.h"
#include "SystemMonitor.h"
#include "TelegramAlert.h"
#

using json = nlohmann::json;
//----------------------------------------------------------------------------
// Add to KeyHunt.cpp (outside class)

//-----------------------------------------------------------------------------

// ===== Helpers =====
static std::string currentDateTime() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
	localtime_s(&tm, &now_c);
#else
	localtime_r(&now_c, &tm);
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}

static std::string readFile(const std::string& path) {
	std::ifstream f(path);
	if (!f) return "{}";
	std::ostringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

static void writeFile(const std::string& path, const std::string& data) {
	std::ofstream f(path);
	f << data;
}

static std::string getTimestampStr(time_t* out) {
	time_t now = time(nullptr);
	if (out) *out = now;
	char buf[64];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
	return std::string(buf);
}

static void logError(const std::string& message, const std::exception* ex = nullptr) {
	std::ofstream log("Updatestatus_error.log", std::ios::app);
	if (!log) return; // fail silently if log cannot be opened
	log << "[" << currentDateTime() << "] " << message;
	if (ex) {
		log << " | Exception: " << ex->what();
	}
	log << std::endl;
}


static void writeOrderedStatus(const std::string& sysId, json& jSystem) {
	json root;
	try {
		root = json::parse(readFile("status.json"));
	}
	catch (const std::exception& e) {
		logError("Failed to parse status.json", &e);
		root = json::object();
	}
	catch (...) {
		logError("Unknown error while parsing status.json");
		root = json::object();
	}

	try {
		if (!root.contains("systems")) root["systems"] = json::object();
		if (!root["systems"].contains(sysId)) root["systems"][sysId] = json::object();

		// Merge current system JSON with new data
		for (auto it = jSystem.begin(); it != jSystem.end(); ++it) {
			root["systems"][sysId][it.key()] = it.value();
		}

		// Rebuild in fixed order
		json orderedSystem = json::object();
		static const std::vector<std::string> order = {
			"init", "bloom", "load", "progress", "system", "found"
		};

		for (const auto& key : order) {
			if (root["systems"][sysId].contains(key)) {
				orderedSystem[key] = root["systems"][sysId][key];
			}
		}

		root["systems"][sysId] = orderedSystem;

		writeFile("status.json", root.dump(2));
	}
	catch (const std::exception& e) {
		logError("Failed to writeOrderedStatus", &e);
	}
	catch (...) {
		logError("Unknown error in writeOrderedStatus");
	}
}


// ===== Implementations =====
void Updatestatus::updateStatusInit(const std::string& rangeStart, const std::string& rangeEnd, int rangeBits, int compMode, int coinType, int searchMode, bool useGpu, int nbCPUThread, const std::vector<int>& gpuId, const std::vector<int>& gridSize, bool useSSE, uint64_t rKey, uint32_t maxFound, const std::string& inputFile, const std::string& outputFile)
{
	std::string sysId = SystemMonitor::getSystemIdentifier();

	json root;
	try { root = json::parse(readFile("status.json")); }
	catch (...) { root = json::object(); }

	json& j = root["systems"][sysId]; // system-specific section

	time_t ts;
	std::string tsStr = getTimestampStr(&ts);

	j["init"] = {
		{"version", "CryptoHunt-Cuda v1.00"},
		{"timestamp", ts},
		{"timestamp_str", tsStr},
		{"system_identifier", sysId},
		{"config", {
			{"range_start", rangeStart},
			{"range_end", rangeEnd},
			{"range_bits", rangeBits},
			{"comp_mode",
	compMode == 0 ? "COMPRESSED" :
	(compMode == 1 ? "UNCOMPRESSED" :
	(compMode == 2 ? "COMPRESSED & UNCOMPRESSED" : "UNKNOWN"))},
			{"coin_type", coinType == 1 ? "BITCOIN" : (coinType == 2 ? "ETHEREUM" : "UNKNOWN")},
			{"search_mode", searchMode == 0 ? "Single" : "Multi Address"},
			{"device", useGpu ? "GPU" : "CPU"},
			{"cpu_threads", nbCPUThread},
			{"gpu_ids", gpuId},
			{"gpu_gridsize", std::to_string(gridSize[0]) + "x" + std::to_string(gridSize[1])},
			{"sse", useSSE},
			{"rkey_mkeys", rKey},
			{"max_found", maxFound},
			{"input_file", inputFile},
			{"output_file", outputFile}
		}}
	};

	writeOrderedStatus(sysId, j);
}

void Updatestatus::updateStatusBloom(uint64_t entries, double errorRate,
	int64_t bits, int64_t bytes, int hashFunctions)
{
	std::string sysId = SystemMonitor::getSystemIdentifier();

	json root;
	try { root = json::parse(readFile("status.json")); }
	catch (...) { root = json::object(); }

	if (!root.contains("systems")) root["systems"] = json::object();
	json& j = root["systems"][sysId];  // full system section

	time_t ts;
	std::string tsStr = getTimestampStr(&ts);

	j["bloom"] = {
		{"timestamp", ts},
		{"timestamp_str", tsStr},
		{"entries", entries},
		{"error_rate", errorRate},
		{"bits", bits},
		{"bytes", bytes},
		{"hash_functions", hashFunctions}
	};

	// ✅ Pass the whole root, not just j
	writeOrderedStatus(sysId, root["systems"][sysId]);
}


void Updatestatus::updateStatusLoad(uint64_t addressesLoaded) {
	std::string sysId = SystemMonitor::getSystemIdentifier();

	json root;
	try { root = json::parse(readFile("status.json")); }
	catch (...) { root = json::object(); }

	json& j = root["systems"][sysId];

	time_t ts;
	std::string tsStr = getTimestampStr(&ts);

	j["load"] = {
		{"timestamp", ts},
		{"timestamp_str", tsStr},
		{"addresses_loaded", addressesLoaded},
		{"message", "Loaded " + std::to_string(addressesLoaded) + " Bitcoin addresses"}
	};

	writeOrderedStatus(sysId, j);
}

void Updatestatus::updateStatusProgress(double mkeyRate, uint64_t totalKeys, double progressPercent, int foundKeys) {
	std::string sysId = SystemMonitor::getSystemIdentifier();

	json root;
	try { root = json::parse(readFile("status.json")); }
	catch (...) { root = json::object(); }

	json& j = root["systems"][sysId];

	j["progress"] = {
		{"keys_scanned", KeyHunt::formatThousands(totalKeys)},
		{"keys_scanned_raw", totalKeys},
		{"mkeys_per_second", mkeyRate},
		{"progress_percent", progressPercent},
		{"found_count", foundKeys},
		{"last_update", (uint64_t)time(nullptr)},
		{"last_update_str", currentDateTime()}
	};

	SystemStats stats = SystemMonitor::getStats();
	j["system"] = {
		{"cpu", {
			{"usage_percent", stats.cpu.usage_percent},
			{"temp_c", stats.cpu.temp_c},
			{"cores", stats.cpu.cores}
		}},
		{"gpu", {
			{"name", stats.gpu.name},
			{"temp_c", stats.gpu.temp_c},
			{"usage_percent", stats.gpu.usage_percent},
			{"memory_used_mb", stats.gpu.memory_used_mb},
			{"memory_total_mb", stats.gpu.memory_total_mb},
			{"fan_percent", stats.gpu.fan_percent},
			{"power_w", stats.gpu.power_w},
			{"clock_mhz", stats.gpu.clock_mhz}
		}},
		{"memory", {
			{"ram_used_mb", stats.memory.ram_used_mb},
			{"ram_total_mb", stats.memory.ram_total_mb},
			{"ram_usage_percent", stats.memory.usage_percent}
		}}
	};

	writeOrderedStatus(sysId, j);
}

void Updatestatus::updateStatusFound(const std::string& hexKey, const std::string& wifCompressed, const std::string& p2pkh, const std::string& p2sh, const std::string& bech32)
{
	std::string sysId = SystemMonitor::getSystemIdentifier();

	//--------------------------------------
	if (Settings::Get().telegram.enabled) {
		TelegramAlert::sendFoundKeyAlertHTML(
			Settings::Get().telegram.botToken,
			Settings::Get().telegram.chatId,
			hexKey,
			wifCompressed,
			p2pkh,
			p2sh,
			bech32
		);
	}
	//------------------------------------------

	// Load full JSON
	json root;
	try { root = json::parse(readFile("status.json")); }
	catch (...) { root = json::object(); }

	if (!root.contains("systems")) root["systems"] = json::object();
	if (!root["systems"].contains(sysId)) root["systems"][sysId] = json::object();

	json& systemObj = root["systems"][sysId];

	// Ensure "found" exists
	if (!systemObj.contains("found") || !systemObj["found"].is_array())
		systemObj["found"] = json::array();

	int nextId = static_cast<int>(systemObj["found"].size()) + 1;
	time_t ts;
	std::string tsStr = getTimestampStr(&ts);

	// Append new found entry
	systemObj["found"].push_back({
		{"id", nextId},
		{"private_key_hex", hexKey},
		{"wif_compressed", wifCompressed},
		{"p2pkh", p2pkh},
		{"p2sh", p2sh},
		{"bech32", bech32},
		{"timestamp", ts},
		{"timestamp_str", tsStr}
		});

	writeOrderedStatus(sysId, systemObj);
}


