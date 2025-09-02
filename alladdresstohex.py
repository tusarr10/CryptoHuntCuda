import sys
import codecs
import base58
import binascii

CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l"

def bech32_hrp_expand(hrp):
    return [ord(x) >> 5 for x in hrp] + [0] + [ord(x) & 31 for x in hrp]

def bech32_polymod(values):
    GEN = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3]
    chk = 1
    for v in values:
        b = chk >> 25
        chk = (chk & 0x1ffffff) << 5 ^ v
        for i in range(5):
            chk ^= GEN[i] if ((b >> i) & 1) else 0
    return chk

def bech32_verify_checksum(hrp, data):
    return bech32_polymod(bech32_hrp_expand(hrp) + data) == 1

def bech32_decode(bech):
    if bech.lower() != bech and bech.upper() != bech:
        return (None, None)
    bech = bech.lower()
    pos = bech.rfind('1')
    if pos < 1 or pos + 7 > len(bech):
        return (None, None)
    hrp = bech[:pos]
    data = []
    for c in bech[pos+1:]:
        if c not in CHARSET:
            return (None, None)
        data.append(CHARSET.find(c))
    if not bech32_verify_checksum(hrp, data):
        return (None, None)
    return hrp, data[:-6]

def convertbits(data, frombits, tobits, pad=True):
    acc = 0
    bits = 0
    ret = []
    maxv = (1 << tobits) - 1
    for value in data:
        if value < 0 or value >> frombits:
            return None
        acc = (acc << frombits) | value
        bits += frombits
        while bits >= tobits:
            bits -= tobits
            ret.append((acc >> bits) & maxv)
    if pad and bits:
        ret.append((acc << (tobits - bits)) & maxv)
    elif bits >= frombits or ((acc << (tobits - bits)) & maxv):
        return None
    return ret

def extract_hash160(address):
    try:
        if address.startswith("1") or address.startswith("3"):
            decoded = base58.b58decode_check(address)
            return decoded[1:]  # Strip version byte
        elif address.startswith("bc1"):
            hrp, data = bech32_decode(address)
            if hrp != 'bc':
                return None
            decoded = convertbits(data[1:], 5, 8, False)
            return bytes(decoded)
        else:
            return None
    except Exception as e:
        return None

def addresses_to_hash160(filein, fileout):
    with open(filein) as inf, open(fileout, 'wb') as outf:
        count = 0
        skip = 0
        for line in inf:
            address = line.strip()
            hash160 = extract_hash160(address)
            if hash160:
                outf.write(hash160)
                print(f"Processed: {address} → {hash160.hex()}")
                count += 1
            else:
                print(f"Skipped: {address}")
                skip += 1

        print(f"\nDone.\nProcessed: {count} address(es)\nSkipped: {skip} invalid address(es)")

# Command-line interface
if len(sys.argv) != 3:
    print('Usage:')
    print(f'\tpython3 {sys.argv[0]} addresses_in.txt hash160_out.bin')
else:
    addresses_to_hash160(sys.argv[1], sys.argv[2])
