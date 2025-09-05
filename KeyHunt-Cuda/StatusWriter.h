//// StatusWriter.h
//#pragma once
//
//#include <string>
//#include <vector>
//
///**
// * @brief Writes initial status.json with full configuration
// *
// * This function is modular and can be called from main.cpp
// * without exposing JSON logic inside KeyHunt.
// */
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
//    // Range
//    const std::string& rangeStartHex,
//    const std::string& rangeEndHex,
//    int rangeStartBits,
//    int rangeEndBits,
//    const std::string& rangeDiffHex,
//    int rangeDiffBits,
//
//    // Bloom (only if used)
//    uint64_t bloomEntries,
//    int64_t bloomBits,
//    int64_t bloomBytes,
//    int bloomHashes,
//
//    // Output file
//    const std::string& outputFile
//);