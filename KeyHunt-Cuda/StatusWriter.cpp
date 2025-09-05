//// StatusWriter.cpp
//#include "StatusWriter.h"
//#include <stdio.h>
//#include <time.h>
//#include <string>
//
//static void indent(FILE* f, int level) {
//    for (int i = 0; i < level; i++) fprintf(f, "  ");
//}
//
//void writeInitialStatus(
//    const std::string& compMode,
//    const std::string& coinType,
//    const std::string& searchMode,
//    bool useGpu,
//    int nbCPUThread,
//    const std::vector<int>& gpuId,
//    const std::vector<int>& gridSize,
//    bool useSSE,
//    uint64_t rKey,
//    uint32_t maxFound,
//
//    const std::string& rangeStartHex,
//    const std::string& rangeEndHex,
//    int rangeStartBits,
//    int rangeEndBits,
//    const std::string& rangeDiffHex,
//    int rangeDiffBits,
//
//    uint64_t bloomEntries,
//    int64_t bloomBits,
//    int64_t bloomBytes,
//    int bloomHashes,
//
//    const std::string& outputFile
//) {
//    FILE* f = fopen("status.json", "w");
//    if (!f) return;
//
//    time_t now = time(nullptr);
//    char timeStr[64];
//    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
//
//    fprintf(f, "{\n");
//
//    // --- Timestamps ---
//    indent(f, 1); fprintf(f, "\"current_time\": %llu,\n", (unsigned long long)now);
//    indent(f, 1); fprintf(f, "\"current_time_str\": \"%s\",\n", timeStr);
//    indent(f, 1); fprintf(f, "\"start_time\": %llu,\n", (unsigned long long)now);
//    indent(f, 1); fprintf(f, "\"start_time_str\": \"%s\",\n", timeStr);
//
//    // --- Search Range ---
//    indent(f, 1); fprintf(f, "\"range\": {\n");
//    indent(f, 2); fprintf(f, "\"start\": \"%s\",\n", rangeStartHex.c_str());
//    indent(f, 2); fprintf(f, "\"end\": \"%s\",\n", rangeEndHex.c_str());
//    indent(f, 2); fprintf(f, "\"start_bits\": %d,\n", rangeStartBits);
//    indent(f, 2); fprintf(f, "\"end_bits\": %d,\n", rangeEndBits);
//    indent(f, 2); fprintf(f, "\"range\": \"%s\",\n", rangeDiffHex.c_str());
//    indent(f, 2); fprintf(f, "\"range_bits\": %d\n", rangeDiffBits);
//    indent(f, 1); fprintf(f, "},\n");
//
//    // --- System ---
//    indent(f, 1); fprintf(f, "\"system\": {\n");
//    indent(f, 2); fprintf(f, "\"comp_mode\": \"%s\",\n", compMode.c_str());
//    indent(f, 2); fprintf(f, "\"coin_type\": \"%s\",\n", coinType.c_str());
//    indent(f, 2); fprintf(f, "\"search_mode\": \"%s\",\n", searchMode.c_str());
//
//    std::string device = "CPU";
//    if (useGpu && nbCPUThread > 0) device = "CPU & GPU";
//    else if (useGpu) device = "GPU";
//    indent(f, 2); fprintf(f, "\"device\": \"%s\",\n", device.c_str());
//
//    indent(f, 2); fprintf(f, "\"cpu_threads\": %d,\n", nbCPUThread);
//    indent(f, 2); fprintf(f, "\"gpu_enabled\": %s,\n", useGpu ? "true" : "false");
//
//    if (useGpu && !gpuId.empty()) {
//        indent(f, 2); fprintf(f, "\"gpu_id\": %d,\n", gpuId[0]);
//        if (gridSize.size() >= 2) {
//            indent(f, 2); fprintf(f, "\"gpu_gridsize\": \"%dx%d\",\n", gridSize[0], gridSize[1]);
//        }
//    }
//
//    indent(f, 2); fprintf(f, "\"sse\": %s,\n", useSSE ? "true" : "false");
//    indent(f, 2); fprintf(f, "\"rkey_mkeys\": %llu,\n", (unsigned long long)rKey);
//    indent(f, 2); fprintf(f, "\"max_found\": %u\n", maxFound);
//    indent(f, 1); fprintf(f, "},\n");
//
//    // --- Bloom Filter (only if multi-address/xpoint)
//    if (bloomEntries > 0) {
//        indent(f, 1); fprintf(f, "\"bloom\": {\n");
//        indent(f, 2); fprintf(f, "\"entries\": %llu,\n", (unsigned long long)bloomEntries);
//        indent(f, 2); fprintf(f, "\"error_rate\": 0.000001,\n");
//        indent(f, 2); fprintf(f, "\"bits\": %lld,\n", (long long)bloomBits);
//        indent(f, 2); fprintf(f, "\"bytes\": %lld,\n", (long long)bloomBytes);
//        indent(f, 2); fprintf(f, "\"hash_functions\": %d\n", bloomHashes);
//        indent(f, 1); fprintf(f, "},\n");
//    }
//
//    // --- Progress (initial) ---
//    indent(f, 1); fprintf(f, "\"progress\": {\n");
//    indent(f, 2); fprintf(f, "\"keys_scanned\": \"0\",\n");
//    indent(f, 2); fprintf(f, "\"keys_scanned_raw\": 0,\n");
//    indent(f, 2); fprintf(f, "\"progress_percent\": 0.00,\n");
//    indent(f, 2); fprintf(f, "\"keys_per_second\": 0.000,\n");
//    indent(f, 2); fprintf(f, "\"mkeys_per_second\": 0.000\n");
//    indent(f, 1); fprintf(f, "},\n");
//
//    // --- Found ---
//    indent(f, 1); fprintf(f, "\"found\": {\n");
//    indent(f, 2); fprintf(f, "\"keys\": 0\n");
//    indent(f, 1); fprintf(f, "}\n");
//
//    fprintf(f, "}\n");
//    fclose(f);
//}