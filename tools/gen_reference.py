"""Reference LZW encoder for the expected values in test/codec.cpp and
test/compile_time.cpp.

The format: an alphabet of 2^alphabet_width literals (codes 0 up), optionally
followed by a CLEAR and an EOI code, then new codes. The code width starts at
alphabet_width + 1 bits and grows up to max_width bits: by default once the next
code to assign no longer fits, or with growth="last" (GIF) once the last code
assigned no longer fits. When the dictionary is full it is frozen, or with
on_full="clear" a CLEAR is written and it starts over. With control codes the
stream starts with CLEAR and ends with EOI. The last byte is zero-padded.

Written independently of the C++ implementation (string-keyed dictionary,
bit-string packing) so that the tests do not compare it against itself.

usage: python3 tools/gen_reference.py   (prints the C++ constants)
"""

FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3


def lzw_codes(
    data: bytes,
    max_width: int,
    *,
    alphabet_width: int = 8,
    control: bool = False,
    growth: str = "next",
    on_full: str = "freeze",
) -> list[tuple[int, int]]:
    """Returns (code, width) pairs."""
    roots = 1 << alphabet_width
    assert all(b < roots for b in data)
    clear, eoi = roots, roots + 1
    first_free = roots + 2 if control else roots
    max_codes = 1 << max_width

    def width(next_code: int) -> int:
        fitting = next_code if growth == "next" else next_code - 1
        return min(max_width, max(alphabet_width + 1, fitting.bit_length()))

    def fresh() -> dict[bytes, int]:
        return {bytes([i]): i for i in range(roots)}

    table = fresh()
    next_code = first_free
    out: list[tuple[int, int]] = []
    if control:
        out.append((clear, width(next_code)))
    if not data:
        if control:
            out.append((eoi, width(next_code)))
        return out
    w = data[:1]
    for b in data[1:]:
        wc = w + bytes([b])
        if wc in table:
            w = wc
            continue
        out.append((table[w], width(next_code)))
        if next_code < max_codes:
            table[wc] = next_code
            next_code += 1
        elif on_full == "clear":
            out.append((clear, width(next_code)))
            table = fresh()
            next_code = first_free
        w = bytes([b])
    out.append((table[w], width(next_code)))
    if control:
        # the decoder adds an entry for the last code before reading EOI
        out.append((eoi, width(min(next_code + 1, max_codes))))
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


def quaternary_input(size: int) -> bytes:
    # mirrors quaternary_input() in test/codec.cpp: four symbols, for a 2-bit alphabet
    return bytes((i * i + i // 3) % 4 for i in range(size))


def hex_list(data: bytes) -> str:
    return ", ".join(f"0x{b:02X}" for b in data)


def print_summary(name: str, codes: list[tuple[int, int]], clear: int | None) -> None:
    widths = sorted({w for _, w in codes})
    clears = sum(1 for c, _ in codes if c == clear)
    print(f"// {name}: {len(codes)} codes, widths {widths}, {clears} clear codes")
    for order, msb in [("lsb_first", False), ("msb_first", True)]:
        out = pack(codes, msb)
        print(f"//   {order}: size {len(out)}, fnv1a 0x{fnv1a(out):016X}")


def main() -> None:
    for name, data in [
        ("TOBEORNOTTOBEORTOBEORNOT", b"TOBEORNOTTOBEORTOBEORNOT"),
        ("AAAAAAAAAA", b"AAAAAAAAAA"),
    ]:
        codes = lzw_codes(data, 12)
        print(f"// {name}: codes {[c for c, _ in codes]}")
        for order, msb in [("lsb_first", False), ("msb_first", True)]:
            print(f"//   {order}: {hex_list(pack(codes, msb))}")
    data = growth_input()
    for max_width in [12, 9]:
        print_summary(
            f"growth_input, max_width {max_width}", lzw_codes(data, max_width), None
        )

    gif = {"control": True, "growth": "last"}
    for name, data in [
        ("GIF, empty", b""),
        ("GIF, quaternary_input(64)", quaternary_input(64)),
    ]:
        codes = lzw_codes(data, 12, alphabet_width=2, **gif)
        print(f"// {name}, alphabet_width 2: codes {[c for c, _ in codes]}")
        print(f"//   widths {[w for _, w in codes]}")
        for order, msb in [("lsb_first", False), ("msb_first", True)]:
            print(f"//   {order}: {hex_list(pack(codes, msb))}")
    print_summary(
        "GIF, growth_input, alphabet_width 8", lzw_codes(growth_input(), 12, **gif), 256
    )
    for on_full in ["freeze", "clear"]:
        codes = lzw_codes(
            quaternary_input(300), 4, alphabet_width=2, on_full=on_full, **gif
        )
        print_summary(
            f"GIF, quaternary_input(300), alphabet_width 2, max_width 4, on_full {on_full}",
            codes,
            4,
        )


if __name__ == "__main__":
    main()
