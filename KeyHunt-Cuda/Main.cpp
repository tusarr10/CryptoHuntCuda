#include "Timer.h"            // ⏱️ High-resolution timer for benchmarking (keys/sec)
// Example: Timer::Init(); auto t = Timer::getSeed32();

#include "KeyHunt.h"          // 🧠 Core class: manages search logic (CPU/GPU), output, matching
// Contains the `KeyHunt` class that orchestrates the entire key search process.

#include "Base58.h"           // 🔤 Decodes Bitcoin Base58Check addresses to hash160
// Converts "1A1zP1..." → 20-byte RIPEMD-160(SHA-256(pubkey))

#include "CmdParse.h"         // 🧰 Command-line argument parser (supports long/short flags)
// Handles -g, --gpu, -m, --mode, etc., with validation.

#include <fstream>            // 📁 File I/O: read hash160/xpoint lists from binary files
#include <string>             // 🧵 C++ string handling
#include <string.h>           // 🛠️ C string functions: strcmp, strlen, etc.
#include <stdexcept>          // 🚨 Exception handling (e.g., invalid args)
#include <cassert>            // 🧪 Debug assertions (e.g., hashORxpoint.size() == 20)
#include <algorithm>          // 🔍 std::transform, std::find, etc.

#ifndef WIN64
#include <signal.h>       // 🚩 Unix signal handling (Ctrl+C → graceful exit)
#include <unistd.h>       // 🐧 Unix system calls (sleep, getpid, etc.)
#endif

// Project version
#define RELEASE "1.00"        // Shown in --version and help

using namespace std;

// Global flag for graceful shutdown on Ctrl+C
bool should_exit = false;

// =============================
// 2. Help Menu: Usage()
// =============================

/**
 * Displays detailed help message showing all available options.
 * Called when user runs `-h` or provides invalid arguments.
 */
void usage()
{
	printf("CryptoHunt-Cuda [OPTIONS...] [TARGETS]\n");
	printf("Where TARGETS is one address/xpont, or multiple hashes/xpoints file\n\n");

	printf("-h, --help                               : Display this message\n");
	printf("-c, --check                              : Check the working of the codes\n");
	printf("-u, --uncomp                             : Search uncompressed points\n");
	printf("-b, --both                               : Search both uncompressed or compressed points\n");
	printf("-g, --gpu                                : Enable GPU calculation\n");
	printf("--gpui GPU ids: 0,1,...                  : List of GPU(s) to use, default is 0\n");
	printf("--gpux GPU gridsize: g0x,g0y,g1x,g1y,... : Specify GPU(s) kernel gridsize, default is 8*(Device MP count),128\n");
	printf("-t, --thread N                           : Specify number of CPU thread, default is number of core\n");
	printf("-i, --in FILE                            : Read rmd160 hashes or xpoints from FILE, should be in binary format with sorted\n");
	printf("-o, --out FILE                           : Write keys to FILE, default: Found.txt\n");
	printf("-m, --mode MODE                          : Specify search mode where MODE is\n");
	printf("                                               ADDRESS  : for single address\n");
	printf("                                               ADDRESSES: for multiple hashes/addresses\n");
	printf("                                               XPOINT   : for single xpoint\n");
	printf("                                               XPOINTS  : for multiple xpoints\n");
	printf("--coin BTC/ETH                           : Specify Coin name to search\n");
	printf("                                               BTC: available mode :-\n");
	printf("                                                   ADDRESS, ADDRESSES, XPOINT, XPOINTS\n");
	printf("                                               ETH: available mode :-\n");
	printf("                                                   ADDRESS, ADDRESSES\n");
	printf("-l, --list                               : List cuda enabled devices\n");
	printf("--range KEYSPACE                         : Specify the range:\n");
	printf("                                               START:END\n");
	printf("                                               START:+COUNT\n");
	printf("                                               START\n");
	printf("                                               :END\n");
	printf("                                               :+COUNT\n");
	printf("                                               Where START, END, COUNT are in hex format\n");
	printf("-r, --rkey Rkey                          : Random key interval in MegaKeys, default is disabled\n");
	printf("-v, --version                            : Show version\n");
}

// =============================
// 3. Helper: getInts()
// =============================

/**
 * Parses comma-separated integers into a vector.
 * Used for --gpui (GPU IDs) and --gpux (grid sizes).
 *
 * Example:
 *   Input: "0,1,2", Output: tokens = {0,1,2}
 *
 * @param name    Name of argument (for error messages)
 * @param tokens  Output vector to fill
 * @param text    Input string (e.g., "0,1")
 * @param sep     Separator (usually ',')
 */

void getInts(string name, vector<int>& tokens, const string& text, char sep)
{

	size_t start = 0, end = 0;
	tokens.clear();
	int item;

	try {

		while ((end = text.find(sep, start)) != string::npos) {
			item = std::stoi(text.substr(start, end - start));
			tokens.push_back(item);
			start = end + 1;
		}

		item = std::stoi(text.substr(start));
		tokens.push_back(item);

	}
	catch (std::invalid_argument&) {

		printf("Invalid %s argument, number expected\n", name.c_str());
		usage();
		exit(-1);

	}

}
// =============================

// =============================
// 4. Helper: parseSearchMode()
// =============================

/**
 * Converts string mode to internal enum.
 *
 * Example:
 *   "address" → SEARCH_MODE_SA
 *   "xpoints" → SEARCH_MODE_MX
 *
 * @param s Mode string
 * @return  One of SEARCH_MODE_SA, SEARCH_MODE_SX, SEARCH_MODE_MA, SEARCH_MODE_MX
 */
int parseSearchMode(const std::string& s)
{
	std::string stype = s;
	std::transform(stype.begin(), stype.end(), stype.begin(), ::tolower);

	if (stype == "address") {
		return SEARCH_MODE_SA;
	}

	if (stype == "xpoint") {
		return SEARCH_MODE_SX;
	}

	if (stype == "addresses") {
		return SEARCH_MODE_MA;
	}

	if (stype == "xpoints") {
		return SEARCH_MODE_MX;
	}

	printf("Invalid search mode format: %s", stype.c_str());
	usage();
	exit(-1);
}

// =============================
// 5. Helper: parseCoinType()
// =============================

/**
 * Parse coin type: BTC or ETH
 *
 * Example:
 *   "btc" → COIN_BTC
 *   "eth" → COIN_ETH
 */
int parseCoinType(const std::string& s)
{
	std::string stype = s;
	std::transform(stype.begin(), stype.end(), stype.begin(), ::tolower);

	if (stype == "btc") {
		return COIN_BTC;
	}

	if (stype == "eth") {
		return COIN_ETH;
	}

	printf("Invalid coin name: %s", stype.c_str());
	usage();
	exit(-1);
}

// =============================
// 6. Helper: parseRange()
// =============================

/**
 * Parse key range string into start/end Int objects.
 *
 * Supported formats:
 *   "a000:+1000000" → start = a000, end = a000 + 1M
 *   ":+1000000"     → random start, scan 1M keys
 *   "a000:b000"     → from a000 to b000
 *   "a000"          → from a000 to a000 + FFFFFFFFFFFFFF
 *
 * @param s      Range string
 * @param start  Output: start key
 * @param end    Output: end key
 * @return true on success
 */
bool parseRange(const std::string& s, Int& start, Int& end)
{
	size_t pos = s.find(':');

	if (pos == std::string::npos) {
		start.SetBase16(s.c_str());
		end.Set(&start);
		end.Add(0xFFFFFFFFFFFFULL);
	}
	else {
		std::string left = s.substr(0, pos);

		if (left.length() == 0) {
			start.SetInt32(1);
		}
		else {
			start.SetBase16(left.c_str());
		}

		std::string right = s.substr(pos + 1);

		if (right[0] == '+') {
			Int t;
			t.SetBase16(right.substr(1).c_str());
			end.Set(&start);
			end.Add(&t);
		}
		else {
			end.SetBase16(right.c_str());
		}
	}

	return true;
}
// =============================
// 7. Ctrl+C Handler
//
#ifdef WIN64
/**
 * Windows: Handle Ctrl+C to trigger graceful shutdown
 */
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		//printf("\n\nCtrl-C event\n\n");
		should_exit = true;
		return TRUE;

	default:
		return TRUE;
	}
}
/**
 * Unix/Linux: Signal handler for SIGINT (Ctrl+C)
 */
#else
void CtrlHandler(int signum) {
	printf("\n\nBYE\n");
	exit(signum);
}
#endif
int main(int argc, char** argv)
{
	// -----------------------------
	// Initialize Timer & Random Seed
	// -----------------------------
	Timer::Init();  // Initialize high-resolution timer system
	// This prepares the Timer class for measuring execution time and generating seeds.

	rseed(Timer::getSeed32());  // Seed the random number generator
	// Uses current time + CPU cycle count to generate a unique seed.
	// Important for random key generation mode (-r).

	// -----------------------------
	// Configuration Variables
	// -----------------------------
	bool gpuEnable = false;           // Will we use GPU? Default: no
	bool gpuAutoGrid = true;          // Should GPU grid size be auto-calculated?
	int compMode = SEARCH_COMPRESSED; // Search mode: compressed keys (default for BTC)
	vector<int> gpuId = { 0 };        // Which GPU(s) to use? Default: GPU 0
	vector<int> gridSize;             // Custom grid/block dimensions for GPU kernels
	string outputFile = "Found.txt";  // Output file to save found private keys
	string inputFile = "";            // Input file path (for multi-target search)
	string address = "";              // Target Bitcoin/Ethereum address (if single)
	string xpoint = "";               // Public key X-coordinate (for XPoint search)
	std::vector<unsigned char> hashORxpoint; // Stores either:
	// - 20-byte hash160 (for addresses), or
	// - 32-byte xpoint (public key X)
	bool singleAddress = false;       // Flag: are we searching for a single target?
	int nbCPUThread = Timer::getCoreNumber(); // Number of CPU threads to use
	// Default: number of CPU cores detected
	bool tSpecified = false;          // Was -t used? (user manually set thread count)
	bool useSSE = true;               // Use SSE optimizations? (faster CPU searches)
	uint32_t maxFound = 1024 * 64;    // Max number of matching keys to store before stopping
	uint64_t rKey = 0;                // Random mode: how many million keys per batch?
	Int rangeStart, rangeEnd;         // Start and end of private key range to scan
	rangeStart.SetInt32(0);           // Initialize to zero
	rangeEnd.SetInt32(0);
	int searchMode = 0;               // What are we searching for? (e.g., single address)
	int coinType = COIN_BTC;          // Which cryptocurrency? BTC or ETH

	hashORxpoint.clear();             // Ensure vector is empty at start

	// -----------------------------
	// Parse Command-Line Arguments
	// -----------------------------
	CmdParse parser;  // Create command-line parser object
	// This class handles both short (-h) and long (--help) options.

	// Add all supported options to the parser
	parser.add("-h", "--help", false);        // Help (no argument)
	parser.add("-c", "--check", false);       // Self-test crypto modules
	parser.add("-l", "--list", false);        // List CUDA GPUs
	parser.add("-u", "--uncomp", false);      // Search uncompressed keys only
	parser.add("-b", "--both", false);        // Search both compressed & uncompressed
	parser.add("-g", "--gpu", false);         // Enable GPU
	parser.add("", "--gpui", true);           // Specify GPU device IDs (requires arg)
	parser.add("", "--gpux", true);           // Specify GPU grid/block size (requires arg)
	parser.add("-t", "--thread", true);       // Set CPU thread count
	parser.add("-i", "--in", true);           // Input file for multiple targets
	parser.add("-o", "--out", true);          // Output file for results
	parser.add("-m", "--mode", true);         // Search mode: ADDRESS, XPOINT, etc.
	parser.add("", "--coin", true);           // Coin type: BTC or ETH
	parser.add("", "--range", true);          // Key range: START:END or START:+COUNT
	parser.add("-r", "--rkey", true);         // Random mode: scan Rkey million keys
	parser.add("-v", "--version", false);     // Show version and exit

	if (argc == 1) {
		usage();  // No arguments → show help
		return 0;
	}

	try {
		parser.parse(argc, argv);  // Parse all command-line inputs
	}
	catch (std::string err) {
		printf("Error: %s\n", err.c_str());
		usage();
		exit(-1);
	}

	std::vector<OptArg> args = parser.getArgs();  // Get list of parsed options

	// Loop through each parsed argument
	for (unsigned int i = 0; i < args.size(); i++) {
		OptArg optArg = args[i];
		std::string opt = args[i].option;

		try {
			// Handle each option
			if (optArg.equals("-h", "--help")) {
				usage();  // Show help and exit
				return 0;
			}
			else if (optArg.equals("-c", "--check")) {
				// Run internal self-tests on cryptographic components
				printf("CryptoHunt-Cuda v" RELEASE "\n\n");
				printf("\nChecking... Secp256K1\n\n");
				Secp256K1* secp = new Secp256K1();
				secp->Init();   // Initialize secp256k1 curve parameters
				secp->Check();  // Perform internal consistency checks
				printf("\n\nChecking... Int\n\n");
				Int* K = new Int();
				// Test big integer math with known hex value
				K->SetBase16("3EF7CEF65557B61DC4FF2313D0049C584017659A32B002C105D04A19DA52CB47");
				K->Check();  // Validate arithmetic operations
				delete secp;
				delete K;
				printf("\n\nChecked successfully\n\n");
				return 0;
			}
			else if (optArg.equals("-l", "--list")) {
#ifdef WIN64
				GPUEngine::PrintCudaInfo();  // Print all CUDA-capable GPUs
#else
				printf("GPU code not compiled, use -DWITHGPU when compiling.\n");
#endif
				return 0;
			}
			else if (optArg.equals("-u", "--uncomp")) {
				compMode = SEARCH_UNCOMPRESSED;  // Only search uncompressed public keys
			}
			else if (optArg.equals("-b", "--both")) {
				compMode = SEARCH_BOTH;  // Search both compressed and uncompressed
			}
			else if (optArg.equals("-g", "--gpu")) {
				gpuEnable = true;  // Enable GPU acceleration
				// Note: original comment says CPU may be disabled here, but it's not enforced
			}
			else if (optArg.equals("", "--gpui")) {
				string ids = optArg.arg;
				getInts("--gpui", gpuId, ids, ',');  // Parse GPU IDs: "0,1,2" → [0,1,2]
			}
			else if (optArg.equals("", "--gpux")) {
				string grids = optArg.arg;
				getInts("--gpux", gridSize, grids, ',');  // Parse grid sizes: "1024,128" → gridX=1024, blockY=128
				gpuAutoGrid = false;  // User specified grid → disable auto-sizing
			}
			else if (optArg.equals("-t", "--thread")) {
				nbCPUThread = std::stoi(optArg.arg);  // Set CPU thread count
				tSpecified = true;  // Mark that user manually set this
			}
			else if (optArg.equals("-i", "--in")) {
				inputFile = optArg.arg;  // Set input file path
			}
			else if (optArg.equals("-o", "--out")) {
				outputFile = optArg.arg;  // Set output file
			}
			else if (optArg.equals("-m", "--mode")) {
				searchMode = parseSearchMode(optArg.arg);  // e.g., "ADDRESS" → SEARCH_MODE_SA
			}
			else if (optArg.equals("", "--coin")) {
				coinType = parseCoinType(optArg.arg);  // e.g., "ETH" → COIN_ETH
			}
			else if (optArg.equals("", "--range")) {
				std::string range = optArg.arg;
				parseRange(range, rangeStart, rangeEnd);  // Parse key range string
				// Examples: "a000:+1000000", ":10000", "a000:b000"
			}
			else if (optArg.equals("-r", "--rkey")) {
				rKey = std::stoull(optArg.arg);  // Set random mode batch size in Mkeys
			}
			else if (optArg.equals("-v", "--version")) {
				printf("CryptoHunt-Cuda v" RELEASE "\n");
				return 0;
			}
		}
		catch (std::string err) {
			printf("Error: %s\n", err.c_str());
			usage();
			return -1;
		}
	}

	// -----------------------------
	// Validate Coin & Search Mode Compatibility
	// -----------------------------
	if (coinType == COIN_ETH && (searchMode == SEARCH_MODE_SX || searchMode == SEARCH_MODE_MX)) {
		// Ethereum does NOT support XPoint-based search
		printf("Error: %s\n", "Wrong search or compress mode provided for ETH coin type");
		usage();
		return -1;
	}
	if (coinType == COIN_ETH) {
		compMode = SEARCH_UNCOMPRESSED;  // ETH uses uncompressed-style pubkey hashing
		useSSE = false;  // Disable SSE (different hash logic than BTC)
	}
	if (searchMode == (int)SEARCH_MODE_MX || searchMode == (int)SEARCH_MODE_SX) {
		useSSE = false;  // XPoint search doesn't benefit from SSE optimizations
	}

	// -----------------------------
	// Parse Operands (User Targets)
	// -----------------------------
	std::vector<std::string> ops = parser.getOperands();  // Non-option arguments

	if (ops.size() == 0) {
		// No operand → must use input file (-i)
		if (inputFile.size() == 0) {
			printf("Error: %s\n", "Missing arguments");
			usage();
			return -1;
		}
		if (searchMode != SEARCH_MODE_MA && searchMode != SEARCH_MODE_MX) {
			printf("Error: %s\n", "Wrong search mode provided for multiple addresses or xpoints");
			usage();
			return -1;
		}
	}
	else {
		// One operand expected (single target)
		if (ops.size() != 1) {
			printf("Error: %s\n", "Wrong args or more than one address or xpoint are provided, use inputFile for multiple addresses or xpoints");
			usage();
			return -1;
		}
		if (searchMode != SEARCH_MODE_SA && searchMode != SEARCH_MODE_SX) {
			printf("Error: %s\n", "Wrong search mode provided for single address or xpoint");
			usage();
			return -1;
		}

		switch (searchMode) {
		case (int)SEARCH_MODE_SA:
		{
			address = ops[0];  // Store target address
			if (coinType == COIN_BTC) {
				// Validate Bitcoin address
				if (address.length() < 30 || address[0] != '1') {
					printf("Error: %s\n", "Invalid address, must have Bitcoin P2PKH address or Ethereum address");
					usage();
					return -1;
				}
				else {
					// Decode Base58Check address
					if (DecodeBase58(address, hashORxpoint)) {
						// Remove version byte (first byte: 0x00)
						hashORxpoint.erase(hashORxpoint.begin());
						// Remove 4-byte checksum (last 4 bytes)
						hashORxpoint.erase(hashORxpoint.begin() + 20, hashORxpoint.begin() + 24);
						// Now hashORxpoint contains only 20-byte hash160
						assert(hashORxpoint.size() == 20);  // Must be exactly 20 bytes
					}
				}
			}
			else {
				// Handle Ethereum address: "0x..."
				if (address.length() != 42 || address[0] != '0' || address[1] != 'x') {
					printf("Error: %s\n", "Invalid Ethereum address");
					usage();
					return -1;
				}
				address.erase(0, 2);  // Remove "0x"
				// Convert hex string to 20-byte array
				for (int i = 0; i < 40; i += 2) {
					uint8_t c = 0;
					for (size_t j = 0; j < 2; j++) {
						uint32_t c0 = (uint32_t)address[i + j];  // Get hex char
						uint8_t c2 = (uint8_t)strtol((char*)&c0, NULL, 16);  // Convert to 4-bit value
						if (j == 0) c2 = c2 << 4;  // First digit → high nibble
						c |= c2;  // Combine into full byte
					}
					hashORxpoint.push_back(c);
				}
				assert(hashORxpoint.size() == 20);  // Must be 20 bytes
			}
		}
		break;
		case (int)SEARCH_MODE_SX:
		{
			unsigned char xpbytes[32];
			xpoint = ops[0];
			Int* xp = new Int();
			xp->SetBase16(xpoint.c_str());  // Parse hex string to big integer
			xp->Get32Bytes(xpbytes);        // Get 32-byte representation (big-endian)
			for (int i = 0; i < 32; i++)
				hashORxpoint.push_back(xpbytes[i]);  // Copy to hashORxpoint
			delete xp;
			if (hashORxpoint.size() != 32) {
				printf("Error: %s\n", "Invalid xpoint");
				usage();
				return -1;
			}
		}
		break;
		default:
			printf("Error: %s\n", "Invalid search mode for single address or xpoint");
			usage();
			return -1;
			break;
		}
	}

	// -----------------------------
	// Set Default GPU Grid Size
	// -----------------------------
	if (gridSize.size() == 0) {
		// If user didn't specify --gpux, set default: -1 (auto), 128 (block size)
		for (int i = 0; i < gpuId.size(); i++) {
			gridSize.push_back(-1);   // Auto grid X
			gridSize.push_back(128);  // Block size = 128
		}
	}
	if (gridSize.size() != gpuId.size() * 2) {
		// Each GPU needs two values: gridX and blockY
		printf("Error: %s\n", "Invalid gridSize or gpuId argument, must have coherent size\n");
		usage();
		return -1;
	}

	// -----------------------------
	// Validate Key Range
	// -----------------------------
	if (rangeStart.GetBitLength() <= 0) {
		printf("Error: %s\n", "Invalid start range, provide start range at least, end range would be: start range + 0xFFFFFFFFFFFFULL\n");
		usage();
		return -1;
	}

	// -----------------------------
	// Adjust CPU Threads if GPU Enabled
	// -----------------------------
	// Leave one CPU core free per GPU to prevent system freeze
	if (!tSpecified && nbCPUThread > 1 && gpuEnable)
		nbCPUThread -= (int)gpuId.size();
	if (nbCPUThread < 0)
		nbCPUThread = 0;

	// -----------------------------
	// Print Final Configuration
	// -----------------------------
	printf("\n");
	printf("CryptoHunt-Cuda v" RELEASE "\n");
	printf("\n");
	if (coinType == COIN_BTC)
		printf("COMP MODE    : %s\n", compMode == SEARCH_COMPRESSED ? "COMPRESSED" : (compMode == SEARCH_UNCOMPRESSED ? "UNCOMPRESSED" : "COMPRESSED & UNCOMPRESSED"));
	printf("COIN TYPE    : %s\n", coinType == COIN_BTC ? "BITCOIN" : "ETHEREUM");
	printf("SEARCH MODE  : %s\n",
		searchMode == (int)SEARCH_MODE_MA ? "Multi Address" :
		(searchMode == (int)SEARCH_MODE_SA ? "Single Address" :
			(searchMode == (int)SEARCH_MODE_MX ? "Multi X Points" : "Single X Point")));
	printf("DEVICE       : %s\n",
		(gpuEnable && nbCPUThread > 0) ? "CPU & GPU" :
		((!gpuEnable && nbCPUThread > 0) ? "CPU" : "GPU"));
	printf("CPU THREAD   : %d\n", nbCPUThread);
	if (gpuEnable) {
		printf("GPU IDS      : ");
		for (int i = 0; i < gpuId.size(); i++) {
			printf("%d", gpuId.at(i));
			if (i + 1 < gpuId.size()) printf(", ");
		}
		printf("\n");
		printf("GPU GRIDSIZE : ");
		for (int i = 0; i < gridSize.size(); i++) {
			printf("%d", gridSize.at(i));
			if (i + 1 < gridSize.size()) {
				if ((i + 1) % 2 != 0) printf("x");  // gridX x blockY
				else printf(", ");  // Next GPU
			}
		}
		if (gpuAutoGrid) printf(" (Auto grid size)\n");
		else printf("\n");
	}
	printf("SSE          : %s\n", useSSE ? "YES" : "NO");
	printf("RKEY         : %llu Mkeys\n", rKey);
	printf("MAX FOUND    : %d\n", maxFound);
	// Print target info
	if (coinType == COIN_BTC) {
		switch (searchMode) {
		case (int)SEARCH_MODE_MA: printf("BTC HASH160s : %s\n", inputFile.c_str()); break;
		case (int)SEARCH_MODE_SA: printf("BTC ADDRESS  : %s\n", address.c_str()); break;
		case (int)SEARCH_MODE_MX: printf("BTC XPOINTS  : %s\n", inputFile.c_str()); break;
		case (int)SEARCH_MODE_SX: printf("BTC XPOINT   : %s\n", xpoint.c_str()); break;
		}
	}
	else {
		switch (searchMode) {
		case (int)SEARCH_MODE_MA: printf("ETH ADDRESSES: %s\n", inputFile.c_str()); break;
		case (int)SEARCH_MODE_SA: printf("ETH ADDRESS  : 0x%s\n", address.c_str()); break;
		}
	}
	printf("OUTPUT FILE  : %s\n", outputFile.c_str());

	// -----------------------------
	// Set Up Ctrl+C Handler
	// -----------------------------
#ifdef WIN64
	if (SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
#else
	signal(SIGINT, CtrlHandler);  // Unix: handle Ctrl+C
#endif
	KeyHunt* v;  // Main search engine object

	// Create KeyHunt instance based on search mode
	switch (searchMode) {
	case (int)SEARCH_MODE_MA:
	case (int)SEARCH_MODE_MX:
		// Multi-target: use input file
		v = new KeyHunt(inputFile, compMode, searchMode, coinType, gpuEnable,
			outputFile, useSSE, maxFound, rKey,
			rangeStart.GetBase16(), rangeEnd.GetBase16(), should_exit);
		break;
	case (int)SEARCH_MODE_SA:
	case (int)SEARCH_MODE_SX:
		// Single target: use hashORxpoint
		v = new KeyHunt(hashORxpoint, compMode, searchMode, coinType, gpuEnable,
			outputFile, useSSE, maxFound, rKey,
			rangeStart.GetBase16(), rangeEnd.GetBase16(), should_exit);
		break;
	default:
		printf("\n\nNothing to do, exiting\n");
		return 0;
	}

	// Start the actual search
	v->Search(nbCPUThread, gpuId, gridSize, should_exit);

	delete v;  // Clean up
	printf("\n\nBYE\n");
	return 0;
#ifdef WIN64
	}
	else {
		printf("Error: could not set control-c handler\n");
		return -1;
		}
#endif
}