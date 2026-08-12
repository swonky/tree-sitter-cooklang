#!/usr/bin/env python3

import unicodedata

UNICODE_LIMIT = 0x110000
BLOCK_SIZE = 0x100
BLOCK_COUNT = UNICODE_LIMIT // BLOCK_SIZE


def unicode_ranges(category_prefix):
    ranges = []

    start = None

    for codepoint in range(UNICODE_LIMIT):
        category = unicodedata.category(chr(codepoint))
        matches = category.startswith(category_prefix)

        if matches and start is None:
            start = codepoint
        elif not matches and start is not None:
            ranges.append((start, codepoint - 1))
            start = None

    if start is not None:
        ranges.append((start, UNICODE_LIMIT - 1))

    return ranges


def build_index(ranges):
    first = [0] * BLOCK_COUNT
    count = [0] * BLOCK_COUNT

    for i, (lo, hi) in enumerate(ranges):
        first_block = lo // BLOCK_SIZE
        last_block = hi // BLOCK_SIZE

        for block in range(first_block, last_block + 1):
            if count[block] == 0:
                first[block] = i

            count[block] += 1

    return first, count


def emit_array(name, values, c_type, columns=8):
    print(f"static const {c_type} {name}[0x{len(values):X}] = {{")

    for i in range(0, len(values), columns):
        row = values[i : i + columns]
        print("\t" + ", ".join(f"0x{x:X}" for x in row) + ",")

    print("};")
    print()


ranges = unicode_ranges("P")
first, count = build_index(ranges)

print("#include <stdint.h>")
print()
print(f"#define UNICODE_PUNCTUATION_COUNT {len(ranges)}")
print()

print("static const UnicodeRange unicode_punctuation[] = {")
for lo, hi in ranges:
    print(f"\t{{0x{lo:06X}, 0x{hi:06X}}},")
print("};")
print()

emit_array(
    "unicode_punctuation_first",
    first,
    "uint16_t",
)

emit_array(
    "unicode_punctuation_count",
    count,
    "uint8_t",
)
