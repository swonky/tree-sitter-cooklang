## [0.2.2] - 2026-08-09

### Features

- Frontmatter can now begin following vertical whitespace.
- Slightly improved highlight query groups.

### Bug Fixes

- Added missing `℃` (U+2103) and `℉` (U+2109) degree symbols to `unicode_tables.h` header.
- `TEXT` tokens can now begin with `)` and `|` when appropriate.

### Refactor

- `scanner.c`
    - Removed vestigial field from Scanner struct.
    - Simplified `metadata` Scanner struct field to bool.
    - Removed redundant valid_symbol check.
    - Documented transient `in_bracket` Scanner field.

### Testing

- Added a Rust-based criterion benchmarking suite based on the implementation in _cooklang-rs_.
- Updated expected test outputs to accommodate improved temperature parsing
