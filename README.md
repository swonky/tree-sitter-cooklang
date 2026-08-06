# tree-sitter-cooklang

A [cooklang](https://cooklang.org) grammar for the  [tree-sitter](https://github.com/tree-sitter/tree-sitter) parser generator.


> [tree-sitter-yaml](https://github.com/tree-sitter-grammars/tree-sitter-yaml) is also required to enable YAML frontmatter metadata parsing.

## Features
- Implements the current Cooklang language base specification [^2].
- Supports the documented Cooklang extensions [^3] listed below.
- Unicode-aware parsing.
- Query files for syntax highlighting, folding, and YAML frontmatter injection.
- Structured numeric quantity tokens (`integer`, `decimal`, `fractional`, `range`).
- Comprehensive test corpus covering canonical recipes and extensions.

## Supported extensions 
A limited number of extensions [^3] are currently supported.

- [x] Modifiers
- [x] Component alias
- [x] Advanced units
- [x] Range values
- [x] Timer requires time (otherwise rendered as plain text)
- [x] Temperature (unicode-aware and some plain English expressions)
- [ ] Intermediate preparations
- [ ] Modes

Where possible, numeric amounts are classified as `integer` or `decimal` tokens, or as structured `fractional` or `range` nodes. 
Non-conforming or ambiguous values fall back to representation as a `string` rather than being represented as untyped text.

## Deviations from specification
This grammar is intentionally more permissive than the cooklang-rs reference parser [^3] and performs limited semantic validation. Where practical, syntactically recoverable constructs are parsed instead of rejected, allowing editor features such as syntax highlighting and incremental parsing to continue operating on incomplete or non-conforming documents.

- __Inline block comments are recognised only where plain text is valid.__ Because tree-sitter performs incremental parsing, comments cannot simply be stripped from the input before lexing. Supporting block comments within structured constructs such as ingredients, cookware, and timers therefore requires substantially more complex grammar and scanner logic. As an implementation trade-off, this parser recognises block comments only where plain text is valid.

## Testing [^4]
The repository contains a several test suites located within `./test/corpus`.

> Running the tests requires [tree-sitter-cli](https://github.com/tree-sitter/tree-sitter/blob/master/crates/cli/README.md).

```bash
# runs all tests in the corpus
tree-sitter test

# fuzzing must exclude cst-based tests
tree-sitter fuzz --exclude "\b\w+_cst\b"
```

| File | Content |
| ---- | ------- |
| canonical.txt     | adapted from the cooklang-rs `canonical.yml` test file [^5]. |
| canonical_cst.txt | same inputs as above, but shows captured content as a concrete syntax tree. |
| additional.txt    | additional tests covering notes, section headings, multiline steps, and more complex syntax combinations. |
| extensions.txt    | tests covering extended language features [^3]. |

## Example

![Syntax highlighting example](docs/example.svg)

### Concrete syntax tree
```yaml
recipe:
  metadata:
    metadata_start:
    metadata_content:
      - "title: Stuffed Peppers"
      - "tags: [dinner, vegetarian]"
      - "servings: 4"
      - "prep time: 20 minutes"
      - "cook time: 35 minutes"
    metadata_end: "---"
  note:
    text: "These freeze well. Double the batch and freeze half for a quick weeknight dinner."
  section:
    heading:
      name: !text "Filling"
    step:
      text: "Cook "
      ingredient:
        name: !identifier "rice"
        quantity: !integer "200"
        unit: !unit "g"
      text: "according to "
      comment: "[- todo: !maybe add flavoured rice recipe -]"
      text: "package directions."
    step:
      text: "Sauté "
      ingredient:
        name: !identifier "onion"
        quantity: !fractional
          left: !integer "1"
          right: !integer "2"
        preparation: !string "diced"
      text: "and "
      ingredient:
        name: !identifier "garlic"
        quantity: !range
          left: !integer "2"
          right: !integer "3"
        unit: !unit "cloves"
        preparation: !string "minced"
      text: "in "
      ingredient:
        name: !identifier "olive oil"
        quantity: !integer "2"
        unit: !unit "tbsp"
      text: "in a "
      cookware:
        name: !identifier "large skillet"
      text: "until softened, about "
      timer:
        name: !identifier "fry"
        quantity: !integer "5"
        unit: !unit "minutes"
      text: "."
    step:
      text: "Add "
      ingredient:
        name: !identifier "canned tomatoes"
        quantity: !integer "400"
        unit: !unit "g"
      text: ", "
      ingredient:
        name: !identifier "black beans"
        quantity: !integer "240"
        unit: !unit "g"
        preparation: !string "drained"
      text: ","
      ingredient:
        name: !identifier "cumin"
        quantity: !integer "1"
        unit: !unit "tsp"
      text: ", and "
      ingredient:
        name: !identifier "smoked paprika"
        quantity: !integer "1"
        unit: !unit "tsp"
      text: ". Stir in the cooked "
      ingredient:
        name: !identifier "rice"
      text: "."
  section:
    heading:
      name: !text "Assembly"
    comment_line: "-- perhaps add sauce section here later?"
    step:
      text: "Cut the tops off "
      ingredient:
        name: !identifier "peppers"
        alias: !identifier "capsicum"
        quantity: !integer "4"
      text: "and remove seeds."
      text: "Stuff with the filling and place in a "
      cookware:
        name: !identifier "baking dish"
      text: "."
    note:
      text: "An aged cheddar is recommended."
    step:
      text: "Top each pepper with "
      ingredient:
        name: !identifier "cheese"
        alias: !identifier "cheddar"
        quantity: !integer "100"
        unit: !unit "g"
        preparation: !string "grated"
      text: "and optionally "
      ingredient:
        name: !identifier "pecorino"
        quantity: !integer "50"
        unit: !unit "g"
        preparation: !string "shaved"
      text: "."
    step:
      text: "Bake in a preheated "
      cookware:
        name: !identifier "oven"
      text: "at "
      temperature:
        quantity: !integer "190"
        scale: !scale "C"
      text: "for "
      timer:
        quantity: !integer "30"
        unit: !unit "minutes"
      text: "until "
      ingredient:
        name: !identifier "peppers"
      text: "are tender and "
      ingredient:
        name: !identifier "cheese"
      text: "is bubbling."
    step:
      text: "Serve with "
      ingredient:
        name: !identifier "./Sauces/Salsa Verde"
        quantity: !range
          left: !integer "1"
          right: !decimal "1.5"
        unit: !unit "servings"
      text: ". "
      comment: "-- todo"
```

## References
[^2]: [Cooklang specification](https://github.com/cooklang/spec)
[^3]: [cooklang-rs Extensions](https://github.com/cooklang/cooklang-rs/blob/main/extensions.md)
[^4]: [tree-sitter: Writing tests](https://tree-sitter.github.io/tree-sitter/creating-parsers/5-writing-tests.html)
[^5]: [cooklang-rs Tests](https://github.com/cooklang/cooklang-rs/blob/main/tests/canonical.yaml)
