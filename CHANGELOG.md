## [0.2.1] - 2026-08-08

### Bug Fixes

- Fixed tokenisation failure for trailing horizontal whitespace after frontmatter '---' token.
- Big typedef oversight in unicode_tables header causing unsafe conversion to unsigned int.
- Fixed some more implicit type conversions and constant folding issues.
- Removed blank field in tree-sitter.json.

### Refactor

- `grammar.js` Tidied up rule definitions.
- `scanner.c` Added explicit fallthrough comments for switch statements as per draconian compiler expectations.
