#include "SystemMonitor.h"
#include "KeyHunt.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

// Keep system-specific headers last
#if defined(_WIN32)
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <netdb.h>
#include <thread>
#endif

// ----------------------------------------------------------------------------
// Execute shell command and return output
// ----------------------------------------------------------------------------
std::string SystemMonitor::exec(const char* cmd) {
    std::string result;
    FILE* pipe = nullptr;

#ifdef _WIN32
    pipe = _popen(cmd, "r");
#else
    pipe = popen(cmd, "r");
#endif

    if (!pipe) return "";

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}

// ----------------------------------------------------------------------------
// Parse Linux load average (1min, 5min, 15min)
// ----------------------------------------------------------------------------
float SystemMonitor::parseLoadAvg(const std::string& load) {
    std::stringstream ss(load);
    float avg;
    ss >> avg;
    return avg;
}

// ===== Unique System Identifier =====
std::string SystemMonitor::getSystemIdentifier() {
#if defined(_WIN32)
    // --- Try MAC Address ---
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD buflen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &buflen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            if (pAdapter->AddressLength > 0) {
                std::ostringstream mac;
                mac << std::hex << std::setfill('0');
                for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                    mac << std::setw(2) << (int)pAdapter->Address[i];
                    if (i < pAdapter->AddressLength - 1) mac << ":";
                }

                char hostname[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD size = sizeof(hostname);
                std::string host = (GetComputerNameA(hostname, &size)) ? hostname : "UnknownHost";

                return mac.str() + " (" + host + ")";
            }
            pAdapter = pAdapter->Next;
        }
    }

    // Fallback hostname
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        return std::string(hostname);
    }
    return "Windows-Unknown";

#else
    // --- Linux / Unix ---
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            if (ifa->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;
                if (s->sll_halen > 0) {
                    std::ostringstream mac;
                    mac << std::hex << std::setfill('0');
                    for (int i = 0; i < s->sll_halen; i++) {
                        mac << std::setw(2) << (int)s->sll_addr[i];
                        if (i < s->sll_halen - 1) mac << ":";
                    }

                    char hostname[256];
                    std::string host = (gethostname(hostname, sizeof(hostname)) == 0) ? hostname : "UnknownHost";

                    freeifaddrs(ifaddr);
                    return mac.str() + " (" + host + ")";
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    // Fallback hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Linux-Unknown";
#endif
}
std::string SystemMonitor::getSystemMAC() {
#if defined(_WIN32)
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD buflen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &buflen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            if (pAdapter->AddressLength > 0) {
                std::ostringstream mac;
                mac << std::hex << std::setfill('0');
                for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                    mac << std::setw(2) << (int)pAdapter->Address[i];
                    if (i < pAdapter->AddressLength - 1) mac << ":";
                }
                return mac.str(); // ? only MAC address
            }
            pAdapter = pAdapter->Next;
        }
    }
    return "Windows-Unknown-ID";

#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            if (ifa->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;
                if (s->sll_halen > 0) {
                    std::ostringstream mac;
                    mac << std::hex << std::setfill('0');
                    for (int i = 0; i < s->sll_halen; i++) {
                        mac << std::setw(2) << (int)s->sll_addr[i];
                        if (i < s->sll_halen - 1) mac << ":";
                    }
                    freeifaddrs(ifaddr);
                    return mac.str(); // ? only MAC address
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return "Linux-Unknown-ID";
#endif
}
std::string SystemMonitor::getSystemName() {
#if defined(_WIN32)
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        return std::string(hostname); // ? only hostname
    }
    return "Windows-UnknownHost";
#else
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname); // ? only hostname
    }
    return "Linux-UnknownHost";
#endif
}


// ----------------------------------------------------------------------------
// Get full system stats (GPU, CPU, RAM)
// ----------------------------------------------------------------------------
SystemStats SystemMonitor::getStats() {
    SystemStats stats;

    // --- GPU Stats (NVIDIA only) ---
    const char* gpuCmd = R"(nvidia-smi --query-gpu=name,temperature.gpu,utilization.gpu,memory.used,memory.total,fan.speed,power.draw,clocks.gr --format=csv,noheader,nounits)";
    std::string gpuOut = exec(gpuCmd);

    if (!gpuOut.empty() && gpuOut.find("Unknown") == std::string::npos) {
        std::stringstream ss(gpuOut);
        std::string name, temp, usage, memUsed, memTotal, fan, power, clock;
        std::getline(ss, name, ',');
        std::getline(ss, temp, ',');
        std::getline(ss, usage, ',');
        std::getline(ss, memUsed, ',');
        std::getline(ss, memTotal, ',');
        std::getline(ss, fan, ',');
        std::getline(ss, power, ',');
        std::getline(ss, clock, ',');

        stats.gpu.name = name;
        try { stats.gpu.temp_c = std::stof(temp); }
        catch (...) {}
        try { stats.gpu.usage_percent = std::stof(usage); }
        catch (...) {}
        try { stats.gpu.memory_used_mb = std::stoi(memUsed); }
        catch (...) {}
        try { stats.gpu.memory_total_mb = std::stoi(memTotal); }
        catch (...) {}
        try { stats.gpu.fan_percent = std::stof(fan); }
        catch (...) {}
        try { stats.gpu.power_w = std::stof(power); }
        catch (...) {}
        try { stats.gpu.clock_mhz = std::stoi(clock); }
        catch (...) {}
    }

    // --- CPU Stats ---
#ifdef _WIN32
    // Windows: CPU Usage
    auto cpuOut = exec("wmic cpu get loadpercentage 2>&1");
    size_t pos = cpuOut.find_first_of("0123456789");
    if (pos != std::string::npos) {
        try {
            stats.cpu.usage_percent = std::stof(cpuOut.substr(pos));
        }
        catch (...) {}
    }

    // Windows: CPU Temp
    auto tempOut = exec(R"(wmic /namespace:\\root\wmi PATH MSAcpi_ThermalZoneTemperature get CurrentTemperature 2>&1)");
    if (tempOut.find("CurrentTemperature") != std::string::npos) {
        pos = tempOut.find_first_of("0123456789", 20);
        if (pos != std::string::npos) {
            try {
                int temp = std::stoi(tempOut.substr(pos));
                stats.cpu.temp_c = (temp / 10.0f) - 273.15f; // Kelvin to Celsius
            }
            catch (...) {}
        }
    }

    // Windows: CPU Cores
    auto coreOut = exec("wmic cpu get NumberOfCores 2>&1");
    pos = coreOut.find_first_of("0123456789");
    if (pos != std::string::npos) {
        try {
            stats.cpu.cores = std::stoi(coreOut.substr(pos));
        }
        catch (...) {}
    }

#else
    // Linux: CPU Usage from /proc/stat
    std::ifstream statFile("/proc/stat");
    std::string line_cpu;
    if (std::getline(statFile, line_cpu) && line_cpu.find("cpu ") != std::string::npos) {
        std::stringstream ss(line_cpu);
        std::string cpu;
        long user, nice, system, idle, iowait, irq, softirq;
        ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
        long total = user + nice + system + idle + iowait + irq + softirq;
        long idleTime = idle + iowait;

        static long lastTotal = 0, lastIdle = 0;
        float totalDelta = total - lastTotal;
        float idleDelta = idleTime - lastIdle;
        if (totalDelta > 0) {
            stats.cpu.usage_percent = 100.0f * (totalDelta - idleDelta) / totalDelta;
        }

        lastTotal = total;
        lastIdle = idleTime;
    }

    // Linux: CPU Cores
    stats.cpu.cores = std::thread::hardware_concurrency();

    // Linux: CPU Temp
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (tempFile.is_open()) {
        float temp;
        tempFile >> temp;
        stats.cpu.temp_c = temp / 1000.0f;
        tempFile.close();
    }
#endif

    // --- RAM Stats ---
#ifdef _WIN32
    // Windows: RAM Usage
    auto ramOut = exec(R"(wmic OS get FreePhysicalMemory,TotalVisibleMemorySize /value)");
    size_t freePos = ramOut.find("FreePhysicalMemory");
    size_t totalPos = ramOut.find("TotalVisibleMemorySize");
    if (freePos != std::string::npos && totalPos != std::string::npos) {
        auto freeStr = ramOut.substr(freePos);
        auto totalStr = ramOut.substr(totalPos);
        int freeKB = std::stoi(freeStr.substr(freeStr.find("=") + 1).c_str());
        int totalKB = std::stoi(totalStr.substr(totalStr.find("=") + 1).c_str());
        stats.memory.ram_total_mb = totalKB / 1024;
        stats.memory.ram_used_mb = (totalKB - freeKB) / 1024;
        stats.memory.usage_percent = (float)(totalKB - freeKB) / totalKB * 100.0f;
    }
#else
    // Linux: RAM Usage from /proc/meminfo
    std::ifstream memfile("/proc/meminfo");
    std::string line_memory;
    long total = 0, free = 0, available = 0;
    while (std::getline(memfile, line_memory)) {
        if (line_memory.find("MemTotal") != std::string::npos) {
            total = std::stol(line_memory.substr(10));
        }
        else if (line_memory.find("MemFree") != std::string::npos) {
            free = std::stol(line_memory.substr(9));
        }
        else if (line_memory.find("MemAvailable") != std::string::npos) {
            available = std::stol(line_memory.substr(13));
        }
    }
    stats.memory.ram_total_mb = total / 1024;
    stats.memory.ram_used_mb = (total - available) / 1024;
    stats.memory.usage_percent = (float)(total - available) / total * 100.0f;
#endif

    return stats;
}