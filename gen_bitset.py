#!/usr/bin/env python3

import unicodedata

bits = 0

for codepoint in range(0x80):
    if unicodedata.category(chr(codepoint)).startswith("P"):
        bits |= 1 << codepoint

words = [(bits >> offset) & 0xFFFFFFFFFFFFFFFF for offset in (0, 64)]

print("static const uint64_t unicode_punctuation_ascii[2] = {")
for word in words:
    print(f"\tUINT64_C(0x{word:016X}),")
print("};")
