# extract_hash160.py
# Extract 20-byte hash160 from:
#   - P2PKH: 1...
#   - P2WPKH: bc1q... (Bech32, witness v0, 20-byte)
# Skip: 3..., bc1p..., P2WSH, invalid
# Output: binary file of 20-byte hash160 values for CryptoHunt-Cuda

import sys
import base58

# Try to import bech32 (pip install bech32)
try:
    from bech32 import bech32_decode, convertbits
except ImportError:
    print("Error: 'bech32' module not found.")
    print("Install it with: pip install bech32")
    print("Or: python -m pip install bech32")
    sys.exit(1)


def decode_address(addr):
    """
    Decode a Bitcoin address and return (hash160, reason)
    Returns (None, reason) if invalid or unsupported.
    """
    addr = addr.strip()
    if not addr or addr.startswith('#'):
        return None, "empty or comment"

    # -----------------------------
    # 1. P2PKH: 1...
    # -----------------------------
    if addr.startswith('1'):
        try:
            decoded = base58.b58decode_check(addr)
            if len(decoded) != 21:
                return None, f"invalid length ({len(decoded)} bytes)"
            if decoded[0] != 0x00:
                return None, f"version 0x{decoded[0]:02x}, expected 0x00"
            return decoded[1:21], "p2pkh"
        except Exception as e:
            return None, f"base58 decode failed: {str(e)}"

    # -----------------------------
    # 2. P2SH: 3...
    # -----------------------------
    elif addr.startswith('3'):
        return None, "P2SH (3...) not supported"

    # -----------------------------
    # 3. Bech32: bc1...
    # -----------------------------
    elif addr.lower().startswith('bc1'):
        # Decode Bech32
        try:
            hrp, data = bech32_decode(addr)
        except Exception:
            return None, "bech32 decode error"

        if hrp is None or data is None:
            return None, "invalid bech32 format"
        if hrp != 'bc':
            return None, f"hrp='{hrp}', expected 'bc'"
        if len(data) == 0:
            return None, "empty data"

        witver = data[0]
        witprog_5bit = data[1:]

        # Convert 5-bit → 8-bit
        try:
            witprog = convertbits(witprog_5bit, 5, 8, False)
        except Exception:
            return None, "5→8 bit conversion failed"

        if witprog is None:
            return None, "conversion returned None"
        if len(witprog) != 20:
            return None, f"witness program len={len(witprog)}, expected 20"

        if witver == 0:
            return bytes(witprog), "bech32_p2wpkh"
        elif witver == 1:
            return None, "Taproot (P2TR / bc1p) not supported"
        else:
            return None, f"unsupported witness version: {witver}"

    # -----------------------------
    # 4. Unknown format
    # -----------------------------
    else:
        return None, "unknown address format"


def main():
    if len(sys.argv) != 3:
        print("Usage: python extract_hash160.py <input.txt> <output.bin>")
        print("Example:")
        print("  python extract_hash160.py addresses.txt hash160.bin")
        print("\nSupported:")
        print("  - P2PKH: 1...")
        print("  - P2WPKH: bc1q... (v0, 20-byte)")
        print("Skipped:")
        print("  - P2SH: 3...")
        print("  - P2WSH: bc1... (32-byte)")
        print("  - Taproot: bc1p...")
        return 1

    infile = sys.argv[1]
    outfile = sys.argv[2]

    count = 0
    skipped = 0

    try:
        with open(infile, 'r', encoding='utf-8') as f, open(outfile, 'wb') as out:
            for line_num, line in enumerate(f, 1):
                addr = line.strip()
                hash160, reason = decode_address(addr)

                if hash160 is not None:
                    out.write(hash160)
                    print(f"✅ Line {line_num}: {reason.upper()} → {addr}")
                    count += 1
                else:
                    # Truncate long addresses in output
                    display_addr = addr if len(addr) <= 40 else addr[:37] + "..."
                    print(f"❌ Line {line_num}: Skipped → {display_addr} ({reason})")
                    skipped += 1

        print(f"\n{'='*60}")
        print(f"✅ SUCCESS: Extracted {count} hash160 values")
        print(f"❌ Skipped : {skipped} addresses")
        print(f"📁 Output  : {outfile}")
        print(f"💡 Use with: -i {outfile}")
        print(f"💡 Your tool must support both P2PKH and P2WPKH")
        print(f"💡 Install bech32: pip install bech32")
        print(f"{'='*60}")

        if count == 0:
            print("⚠️  Warning: No valid addresses extracted. Check input file.")
            return 1

    except FileNotFoundError:
        print(f"❌ Error: Input file not found: {infile}")
        return 1
    except PermissionError:
        print(f"❌ Error: Cannot write to output file: {outfile}")
        return 1
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())