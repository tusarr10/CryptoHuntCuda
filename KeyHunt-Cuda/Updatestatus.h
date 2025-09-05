#include <string>
#include <vector>

class Updatestatus {  
public:  
	static void updateStatusProgress(double mkeyRate, uint64_t totalKeys, double progressPercent, int foundKey);
	static void updateStatusFound(const std::string& hexKey, const std::string& wifCompressed,
		const std::string& p2pkh, const std::string& p2sh, const std::string& bech32);
	static void updateStatusLoad(uint64_t addressesLoaded);
	static void updateStatusBloom(uint64_t entries, double errorRate, int64_t bits, int64_t bytes, int hashFunctions);
	static void updateStatusInit(const std::string& rangeStart, const std::string& rangeEnd, int rangeBits,
		int compMode, int coinType, int searchMode, bool useGpu, int nbCPUThread,
		const std::vector<int>& gpuId, const std::vector<int>& gridSize,
		bool useSSE, uint64_t rKey, uint32_t maxFound,
		const std::string& inputFile, const std::string& outputFile);

	
};
