// SystemMonitor.h
#pragma once

#include <string>

struct GPUStats {
    std::string name;
    float temp_c = -1;
    float usage_percent = -1;
    int memory_used_mb = -1;
    int memory_total_mb = -1;
    float fan_percent = -1;
    float power_w = -1;
    int clock_mhz = -1;
};

struct CPUStats {
    float usage_percent = -1;
    float temp_c = -1;
    int cores = 0;
};

struct MemoryStats {
    int ram_used_mb = 0;
    int ram_total_mb = 0;
    float usage_percent = 0;
};

struct SystemStats {
    CPUStats cpu;
    GPUStats gpu;
    MemoryStats memory;
};

class SystemMonitor {
public:
    static SystemStats getStats();
    static std::string getSystemIdentifier();
private:
    static std::string exec(const char* cmd);
    static float parseLoadAvg(const std::string& load);
};