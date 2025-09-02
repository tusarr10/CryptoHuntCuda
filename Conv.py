# extract_hash160.py
# Extracts hash160 from Bitcoin addresses (1..., 3..., bc1...)
# Outputs raw 20-byte hash160 values to binary file for tools like CryptoHunt-Cuda

import sys
import base58
from bech32 import bech32_decode, convertbits

def decode_address(addr):
    """
    Decode a Bitcoin address and return the underlying pubkey hash160 (20 bytes)
    Returns None if invalid or unsupported.
    """
    addr = addr.strip()
    
    # Skip empty lines and comments
    if not addr or addr.startswith('#'):
        return None

    # ----------------------------------------
    # 1. P2PKH: 1...
    # ----------------------------------------
    if addr.startswith('1'):
        try:
            decoded = base58.b58decode_check(addr)
            if len(decoded) != 21:
                print(f"Invalid length for P2PKH: {len(decoded)} bytes")
                return None
            if decoded[0] != 0x00:
                print(f"Invalid version byte for P2PKH: {decoded[0]:02x}")
                return None
            return decoded[1:21]  # hash160
        except Exception as e:
            print(f"Base58Check decode failed for {addr}: {e}")
            return None

    # ----------------------------------------
    # 2. P2SH: 3...
    # ----------------------------------------
    elif addr.startswith('3'):
        try:
            decoded = base58.b58decode_check(addr)
            if len(decoded) != 21:
                print(f"Invalid length for P2SH: {len(decoded)} bytes")
                return None
            if decoded[0] != 0x05:
                print(f"Invalid version byte for P2SH: {decoded[0]:02x}")
                return None
            # For P2SH-P2WPKH, the 20 bytes IS the pubkey hash160
            return decoded[1:21]  # pubkey hash160
        except Exception as e:
            print(f"Base58Check decode failed for {addr}: {e}")
            return None

    # ----------------------------------------
    # 3. Bech32: bc1...
    # ----------------------------------------
    elif addr.lower().startswith('bc1'):
        hrp, data = bech32_decode(addr)
        if hrp is None or data is None:
            print(f"Bech32 decode failed (invalid format): {addr}")
            return None
        if hrp != 'bc':
            print(f"Wrong HRP (not 'bc'): {hrp}")
            return None
        if not data:
            print(f"No data in Bech32: {addr}")
            return None

        witver = data[0]  # Witness version
        witprog_5bit = data[1:]  # Witness program (5-bit)

        # Convert 5-bit → 8-bit
        witprog = convertbits(witprog_5bit, 5, 8, False)
        if witprog is None:
            print(f"Bech32 5-to-8 bit conversion failed: {addr}")
            return None

        # Support only witness v0 (P2WPKH, P2WSH)
        if witver == 0:
            if len(witprog) == 20:
                return bytes(witprog)  # P2WPKH
            elif len(witprog) == 32:
                return bytes(witprog)  # P2WSH
            else:
                print(f"Unsupported P2WPKH/P2WSH length: {len(witprog)} bytes")
                return None
        else:
            print(f"Unsupported witness version: {witver}")
            return None

    else:
        print(f"Unknown address type (not 1, 3, or bc1): {addr}")
        return None

    return None  # Should not reach here


def main():
    if len(sys.argv) != 3:
        print("Usage: python extract_hash160.py <input.txt> <output.bin>")
        print("Example:")
        print("  python extract_hash160.py addresses.txt hash160.bin")
        print("\nInput file format:")
        print("  One address per line (1..., 3..., bc1...)")
        print("  Comments (starting with #) and empty lines are ignored")
        return -1

    infile = sys.argv[1]
    outfile = sys.argv[2]

    count = 0
    skipped = 0

    try:
        with open(infile, 'r') as f, open(outfile, 'wb') as out:
            for line_num, line in enumerate(f, 1):
                addr = line.strip()
                if not addr or addr.startswith('#'):
                    continue

                hash160 = decode_address(addr)
                if hash160:
                    out.write(hash160)
                    count += 1
                else:
                    skipped += 1
                    print(f"Skipped line {line_num}: {addr}")

        print(f"\n✅ Success!")
        print(f"Extracted: {count} hash160 values")
        print(f"Skipped  : {skipped} addresses")
        print(f"Output   : {outfile}")

    except Exception as e:
        print(f"Error processing files: {e}")
        return -1

    return 0


if __name__ == '__main__':
    sys.exit(main())