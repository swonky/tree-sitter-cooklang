# tree-sitter-cooklang

Unofficial [tree-Sitter](https://github.com/tree-sitter/tree-sitter) grammar for [Cooklang](https://cooklang.org/).

This grammar is intentionally permissive compared with the reference parser. 
Where practical, syntactically recoverable constructs are parsed instead of rejected, allowing editor features such as highlighting, navigation, and incremental parsing to continue operating on incomplete or non-conforming documents.

>> **YAML frontmatter metadata is captured, not parsed.** This parser supports injected syntax trees for `metadata_content` token. 
To enable full document parsing, the [tree-sitter-yaml](https://github.com/tree-sitter-grammars/tree-sitter-yaml) grammar should also be installed.

## Deviations from specification

**Inline block comments are recognised only where plain text is valid.** They are not recognised inside ingredients, cookware, timers, or other structured tokens, as Tree-sitter cannot preprocess the document before parsing like the reference implementation.

**Bare timers (`~rest`) are treated as plain text rather than timer nodes.** The cooklang specification permits timers without durations, however the reference parser does not appear to honour this. Considering that timers without durations are semantically incomplete, this parser intentionally follows the behaviour of the reference parser, and recognizes only timers with explicit quantities (e.g. `~{10%minutes}` or `~boil{10%minutes}`), treating bare timers as ordinary text.

## Supported extensions 
A limited number of extensions are currently supported.

### Modifiers
Ingredient identifiers can be prefixed with any number of modifier characters (`@`, `+`, `-`, `&`, `?`). The parser does not validate against repeated or incompabible modifier combinations. Modifiertokens are expressed in the syntax tree as anonymous nodes.
```cooklang
@@+-&?chicken
```
```query
(ingredient
    "@"
    "+"
    "-"
    "&"
    "?"
    name: (identifier) ; `chicken`
)

```
### Aliases
Aliases are supported for ingredients and cookware.
```cooklang
@white wine|wine{}
```
```query
(ingredient
    name: (identifier)      ; `white wine`
    alias: (identifier))))  ; `wine`
```

### 'Range' and 'Advanced units'
Where possible, numeric amounts are classified as `integer` or `decimal` leaf tokens, or as structured `fractional` or `range` nodes. Non-conforming or ambiguous values fallback to representation as a `string` rather than being represented as untyped text.

The `range` node can contain mixed `integer` and `decimal` components.
```cooklang
@water{1.5-2%l}
```
```query
(ingredient
    name: (identifier)      ; `water`
    quantity: (range
        left: (decimal)     ; `1.5`
        right: (integer))   ; `2`
    unit: (unit))))         ; `l`
```

The 'advanced units' extension allows typed tokenisation without a `%` delimiter.
```cooklang
@water{1 L} is the same as @water{1%L}
```
```query
(ingredient
    name: (identifier)   ; `water`
    quantity: (integer)  ; `1`
    unit: (unit))))      ; `L`
```

### Temperature
Temperature expressions in free text are parsed as structured temperature nodes to enable editor tooling and semantic analysis. The parser recognises common unicode degree symbols (`U+00B0`, `U+00BA`, and `U+02DA`), and some plain English expressions.
```cooklang
Preheat the oven to 100C, or 100 deg C, 100 degrees Celsius, or 100 C, or 100 degC, or 100 ºC,  or 100ºF, and so on...
```
```query
(temperature
    quantity: (integer)
    scale: (scale))
```


## References
* [Official tree-sitter-cooklang](https://github.com/addcninblue/tree-sitter-cooklang)
* [Cooklang specification](https://github.com/cooklang/spec)
* [cooklang-rs Extensions](https://github.com/cooklang/cooklang-rs/blob/main/extensions.md)
* [cooklang-rs Playground](https://cooklang.github.io/cooklang-rs/)
