/*
 * This file is part of the VanitySearch distribution (https://github.com/JeanLucPons/VanitySearch).
 * Copyright (c) 2019 Jean Luc PONS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SECP256K1H
#define SECP256K1H

#include "Point.h"           // Defines Point (x,y,z) and Int (big integer)
#include <string>            // std::string
#include <vector>            // std::vector

 // ============================================================================
 // Secp256k1 Class
 // ============================================================================
 //
 // Purpose:
 //   - Implements Bitcoin/Ethereum elliptic curve cryptography (secp256k1)
 //   - Converts private keys → public keys → addresses
 //   - Supports fast batch operations (SSE) and GPU offloading
 //
 // Used in tools like:
 //   - Vanity address generators
 //   - Private key recovery tools (e.g., CryptoHunt-Cuda)
 //   - Wallet inspection
 //
 // Curve: y² = x³ + 7 over finite field (prime P)
 // Generator: G (fixed base point)
 // Order: n (number of points on curve)
 //
 // ============================================================================

class Secp256K1
{
public:

    // -----------------------------
    // Constructor / Destructor
    // -----------------------------
    Secp256K1();
    ~Secp256K1();

    // -----------------------------
    // Initialize Curve Parameters
    // -----------------------------
    // Must be called before any crypto operation.
    // Sets up:
    //   - Prime field (P)
    //   - Generator point G
    //   - Curve order (n)
    //   - Precomputed table (GTable) for fast scalar multiplication
    void Init();

    // -----------------------------
    // Compute Public Key from Private Key
    // -----------------------------
    // Input: 32-byte private key (as Int*)
    // Output: Public key (Point) on secp256k1 curve
    //
    // Uses precomputed GTable for speed (windowed method)
    // Equivalent to: pubKey = privKey * G
    Point ComputePublicKey(Int* privKey);

    // -----------------------------
    // Increment Public Key by G
    // -----------------------------
    // Input: Current public key (Point&)
    // Output: nextKey = currentKey + G
    //
    // Optimized for sequential key scanning.
    // Assumes input is reduced.
    Point NextKey(Point& key);

    // -----------------------------
    // Self-Test: Validate Implementation
    // -----------------------------
    // Runs internal checks:
    //   - GTable points lie on curve
    //   - Add/Double work correctly
    //   - Known key → known address
    //   - Big integer math
    // Called with `-c` flag.
    void Check();

    // -----------------------------
    // Check if Point Lies on Curve
    // -----------------------------
    // Verifies: y² ≡ x³ + 7 (mod P)
    // Used to validate public keys.
    bool EC(Point& p);

    // -----------------------------
    // Batch Hash160 (4 keys at once, SSE-optimized)
    // -----------------------------
    // Input: 4 compressed/uncompressed public keys (k0-k3)
    // Output: 4 hash160s (RIPEMD-160(SHA-256(pubkey)))
    // Uses SIMD (SSE) for 4x speed.
    void GetHash160(bool compressed,
        Point& k0, Point& k1, Point& k2, Point& k3,
        uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3);

    // -----------------------------
    // Single Hash160 (Bitcoin)
    // -----------------------------
    // Input: One public key
    // Output: 20-byte hash160 (used in P2PKH/P2SH)
    void GetHash160(bool compressed, Point& pubKey, unsigned char* hash);

    // -----------------------------
    // Ethereum Hash (keccak-160)
    // -----------------------------
    // Input: Public key
    // Output: 20-byte keccak-160(pubkey) → Ethereum address
    // Ethereum does NOT use compression.
    void GetHashETH(Point& pubKey, unsigned char* hash);

    // -----------------------------
    // Get Raw Public Key Bytes
    // -----------------------------
    // Output: Serializes public key to bytes
    //   - Uncompressed: 0x04 || X || Y (65 bytes)
    //   - Compressed: 0x02/0x03 || X (33 bytes)
    void GetPubKeyBytes(bool compressed, Point& pubKey, unsigned char* publicKeyBytes);

    // -----------------------------
    // Get X-Coordinate Bytes (for XPoint search)
    // -----------------------------
    // Output: Only X (32 bytes), optionally Y
    // Used in "xpoint" collision attacks.
    void GetXBytes(bool compressed, Point& pubKey, unsigned char* publicKeyBytes);

    // -----------------------------
    // Address Generation Functions
    // -----------------------------

    // From public key → Bitcoin address (P2PKH)
    std::string GetAddress(bool compressed, Point& pubKey);

    // From public key → Ethereum address (0x...)
    std::string GetAddressETH(Point& pubKey);

	// From hash160 → Bitcoin address (Base58Check) Delete latter
    std::string GetAddress(bool compressed, unsigned char* hash160);
    // From hash160 → Bitcoin address (Base58Check)
    std::vector<std::string> GetAllAddress(bool compresses, unsigned char* hash160);

    // Add to Secp256K1 class
    std::vector<std::string> GetAllAddresses(bool compressed, Point& pubKey);

    // From hash160 → Ethereum address (0x + hex)
    std::string GetAddressETH(unsigned char* hash);

    // Batch: 4 hash160s → 4 addresses (SSE-optimized)
    std::vector<std::string> GetAddress(bool compressed,
        unsigned char* h1, unsigned char* h2,
        unsigned char* h3, unsigned char* h4);

    // -----------------------------
    // Private Key → WIF (Wallet Import Format)
    // -----------------------------
    // Input: Private key (Int), compressed flag
    // Output: WIF string (e.g., "Kx...")
    // Format:
    //   - Uncompressed: Base58(0x80 || key)
    //   - Compressed: Base58(0x80 || key || 0x01)
    std::string GetPrivAddress(bool compressed, Int& privKey);

    // Placeholder for ETH WIF (not used — ETH uses raw keys)
    // std::string GetPrivAddressETH(Int& privKey);

    // -----------------------------
    // Public Key → Hex String
    // -----------------------------
    // Output: Hex representation
    //   - Uncompressed: "04" + X + Y (130 chars)
    //   - Compressed: "02/03" + X (66 chars)
    std::string GetPublicKeyHex(bool compressed, Point& pubKey);

    // Ethereum: Full 64-byte pubkey as hex (no prefix)
    std::string GetPublicKeyHexETH(Point& pubKey);

    // -----------------------------
    // Parse Public Key from Hex
    // -----------------------------
    // Input: Hex string (e.g., "02abc...")
    // Output: Point on curve
    // Sets `isCompressed` flag.
    // Supports:
    //   - 02: compressed, Y even
    //   - 03: compressed, Y odd
    //   - 04: uncompressed
    Point ParsePublicKeyHex(std::string str, bool& isCompressed);

    // -----------------------------
    // Validate Bitcoin Address Checksum
    // -----------------------------
    // Checks Base58Check integrity of a P2PKH address
    // Returns true if checksum is valid
    bool CheckPudAddress(std::string address);

    // -----------------------------
    // Decode WIF Private Key
    // -----------------------------
    // Input: WIF string (e.g., "5H...", "Kx...")
    // Output: 32-byte private key (Int)
    // Sets `compressed` flag.
    // Validates prefix and checksum.
    static Int DecodePrivateKey(char* key, bool* compressed);

    // -----------------------------
    // Elliptic Curve Group Operations
    // -----------------------------
    // Low-level math for point addition/doubling
    // Used in ComputePublicKey and NextKey

    Point Add(Point& p1, Point& p2);           // p1 + p2 (projective coords)
    Point Add2(Point& p1, Point& p2);          // Optimized add (p2.z = 1)
    Point AddDirect(Point& p1, Point& p2);     // Fast add (assumes reduced inputs)
    Point Double(Point& p);                    // 2*p
    Point DoubleDirect(Point& p);              // Fast double

    // -----------------------------
    // Public Curve Constants
    // -----------------------------
    Point G;           // Generator point: base of the group
    Int   order;       // Order of the group: #G = n

private:

    // -----------------------------
    // Helper: Parse Hex Digit Pair
    // -----------------------------
    // Input: Hex string, index
    // Output: Byte value at 2*index
    // Example: GetByte("AABBCC", 1) → 0xBB
    uint8_t GetByte(std::string& str, int idx);

    // -----------------------------
    // Recover Y from X (for compressed keys)
    // -----------------------------
    // Solves: y = sqrt(x³ + 7) mod P
    // `isEven`: true → choose even Y, false → odd Y
    Int GetY(Int x, bool isEven);

    // -----------------------------
    // Precomputed Generator Table
    // -----------------------------
    // GTable[i] = (i+1) * G for i in [0..8191]
    // Used for fast scalar multiplication in ComputePublicKey
    // Speeds up key generation by ~10x
    Point GTable[256 * 32];  // 8192 entries

};

#endif // SECP256K1H