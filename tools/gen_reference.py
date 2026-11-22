"""Reference LZW encoder for the expected values in test/codec.cpp and
test/compile_time.cpp.

The format: byte alphabet (codes 0..255 are literals), new codes from 256,
code width starting at 9 bits and growing once the next code to assign no
longer fits, up to max_width bits; when the dictionary is full it is frozen.
No CLEAR or EOI codes; the last byte is zero-padded.

Written independently of the C++ implementation (string-keyed dictionary,
bit-string packing) so that the tests do not compare it against itself.

usage: python3 tools/gen_reference.py   (prints the C++ constants)
"""

FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3


def lzw_codes(data: bytes, max_width: int) -> list[tuple[int, int]]:
    """Returns (code, width) pairs."""
    max_codes = 1 << max_width
    table = {bytes([i]): i for i in range(256)}
    out: list[tuple[int, int]] = []
    if not data:
        return out
    w = data[:1]
    for b in data[1:]:
        wc = w + bytes([b])
        if wc in table:
            w = wc
            continue
        out.append((table[w], min(max_width, len(table).bit_length())))
        if len(table) < max_codes:
            table[wc] = len(table)
        w = bytes([b])
    out.append((table[w], min(max_width, len(table).bit_length())))
    return out


def pack(codes: list[tuple[int, int]], msb_first: bool) -> bytes:
    bits = ""
    for code, width in codes:
        s = format(code, f"0{width}b")
        bits += s if msb_first else s[::-1]
    bits += "0" * (-len(bits) % 8)
    octets = [bits[i : i + 8] for i in range(0, len(bits), 8)]
    return bytes(int(o if msb_first else o[::-1], 2) for o in octets)


def fnv1a(data: bytes) -> int:
    h = FNV_OFFSET
    for b in data:
        h = ((h ^ b) * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return h


def growth_input() -> bytes:
    # mirrors growth_input() in test/codec.cpp: almost every pair is new, so
    # about one code per byte, enough to pass code 512
    return bytes((i * 7 + (i // 256) * 13) % 256 for i in range(600))


def main() -> None:
    for name, data in [
        ("TOBEORNOTTOBEORTOBEORNOT", b"TOBEORNOTTOBEORTOBEORNOT"),
        ("AAAAAAAAAA", b"AAAAAAAAAA"),
    ]:
        codes = lzw_codes(data, 12)
        print(f"// {name}: codes {[c for c, _ in codes]}")
        for order, msb in [("lsb_first", False), ("msb_first", True)]:
            print(f"//   {order}: {', '.join(f'0x{b:02X}' for b in pack(codes, msb))}")
    data = growth_input()
    for max_width in [12, 9]:
        codes = lzw_codes(data, max_width)
        widths = sorted({w for _, w in codes})
        print(
            f"// growth_input, max_width {max_width}: {len(codes)} codes, widths {widths}"
        )
        for order, msb in [("lsb_first", False), ("msb_first", True)]:
            out = pack(codes, msb)
            print(f"//   {order}: size {len(out)}, fnv1a 0x{fnv1a(out):016X}")


if __name__ == "__main__":
    main()
