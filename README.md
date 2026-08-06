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

## Highlighting example

<div>
<table>
<tr><td class=line-number>1</td><td class=line><span style='font-weight: bold;color: #cc241d'>---</span>
</td></tr>
<tr><td class=line-number>2</td><td class=line><span style='font-weight: bold;color: #cc241d'></span><span style='font-weight: bold;color: #7dfdfe'>title</span><span style='font-weight: bold;color: #ffffff'>:</span> <span style='color: #39963e'>Stuffed Peppers</span>
</td></tr>
<tr><td class=line-number>3</td><td class=line><span style='font-weight: bold;color: #7dfdfe'>tags</span><span style='font-weight: bold;color: #ffffff'>:</span> <span style='font-weight: bold;color: #ffffff'>[</span><span style='color: #39963e'>dinner</span><span style='font-weight: bold;color: #ffffff'>,</span> <span style='color: #39963e'>vegetarian</span><span style='font-weight: bold;color: #ffffff'>]</span>
</td></tr>
<tr><td class=line-number>4</td><td class=line><span style='font-weight: bold;color: #7dfdfe'>servings</span><span style='font-weight: bold;color: #ffffff'>:</span> <span style='font-weight: bold;color: #39963e'>4</span>
</td></tr>
<tr><td class=line-number>5</td><td class=line><span style='font-weight: bold;color: #7dfdfe'>prep time</span><span style='font-weight: bold;color: #ffffff'>:</span> <span style='color: #39963e'>20 minutes</span>
</td></tr>
<tr><td class=line-number>6</td><td class=line><span style='font-weight: bold;color: #7dfdfe'>cook time</span><span style='font-weight: bold;color: #ffffff'>:</span> <span style='color: #39963e'>35 minutes</span>
</td></tr>
<tr><td class=line-number>7</td><td class=line><span style='font-weight: bold;color: #cc241d'>---</span>
</td></tr>
<tr><td class=line-number>8</td><td class=line>
</td></tr>
<tr><td class=line-number>9</td><td class=line><span style='color: #39963e'>&gt;</span> <span style='font-style: italic;color: #39963e'>These freeze well. Double the batch and freeze half for a quick weeknight dinner.</span>
</td></tr>
<tr><td class=line-number>10</td><td class=line>
</td></tr>
<tr><td class=line-number>11</td><td class=line><span style='text-decoration: underline;font-weight: bold;color: #ffffff'>= Filling</span>
</td></tr>
<tr><td class=line-number>12</td><td class=line>
</td></tr>
<tr><td class=line-number>13</td><td class=line>Cook <span style='color: #f93d5d'>@rice<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>200</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>g</span><span style='font-weight: bold;color: #ffffff'>}</span></span> according to <span style='font-style: italic;color: #928374'>[- todo: maybe add flavoured rice recipe -]</span> package directions.
</td></tr>
<tr><td class=line-number>14</td><td class=line>
</td></tr>
<tr><td class=line-number>15</td><td class=line>Sauté <span style='color: #f93d5d'>@onion<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'><span style='font-weight: bold;color: #39963e'>1</span><span style='font-weight: bold;color: #ffffff'>/</span><span style='font-weight: bold;color: #39963e'>2</span></span><span style='font-weight: bold;color: #ffffff'>}(</span><span style='color: #39963e'>diced</span><span style='font-weight: bold;color: #ffffff'>)</span></span> and <span style='color: #f93d5d'>@garlic<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>2</span><span style='font-weight: bold;color: #ffffff'>-</span><span style='font-weight: bold;color: #39963e'>3</span> <span style='color: #d79921'>cloves</span><span style='font-weight: bold;color: #ffffff'>}(</span><span style='color: #39963e'>minced</span><span style='font-weight: bold;color: #ffffff'>)</span></span> in <span style='color: #f93d5d'>@olive oil<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>2</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>tbsp</span><span style='font-weight: bold;color: #ffffff'>}</span></span>
</td></tr>
<tr><td class=line-number>16</td><td class=line>in a <span style='color: #ddb700'>#large skillet<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #ffffff'>}</span></span> until softened, about <span style='font-weight: bold;color: #7dfdfe'>~fry<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>5</span> <span style='color: #d65d0e'>minutes</span><span style='font-weight: bold;color: #ffffff'>}</span></span>.
</td></tr>
<tr><td class=line-number>17</td><td class=line>
</td></tr>
<tr><td class=line-number>18</td><td class=line>Add <span style='color: #f93d5d'>@canned tomatoes<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>400</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>g</span><span style='font-weight: bold;color: #ffffff'>}</span></span>, <span style='color: #f93d5d'>@black beans<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>240</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>g</span><span style='font-weight: bold;color: #ffffff'>}(</span><span style='color: #39963e'>drained</span><span style='font-weight: bold;color: #ffffff'>)</span></span>,
</td></tr>
<tr><td class=line-number>19</td><td class=line><span style='color: #f93d5d'>@cumin<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>1</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>tsp</span><span style='font-weight: bold;color: #ffffff'>}</span></span>, and <span style='color: #f93d5d'>@smoked paprika<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>1</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>tsp</span><span style='font-weight: bold;color: #ffffff'>}</span></span>. Stir in the cooked <span style='color: #f93d5d'>@<span style='font-weight: bold;color: #d79921'>&amp;</span>rice</span>.
</td></tr>
<tr><td class=line-number>20</td><td class=line>
</td></tr>
<tr><td class=line-number>21</td><td class=line><span style='text-decoration: underline;font-weight: bold;color: #ffffff'>= Assembly</span>
</td></tr>
<tr><td class=line-number>22</td><td class=line>
</td></tr>
<tr><td class=line-number>23</td><td class=line><span style='font-style: italic;color: #928374'>-- perhaps add sauce section here later?</span>
</td></tr>
<tr><td class=line-number>24</td><td class=line>Cut the tops off <span style='color: #f93d5d'>@peppers<span style='font-weight: bold;color: #ffffff'>|</span>capsicum<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>4</span><span style='font-weight: bold;color: #ffffff'>}</span></span> and remove seeds.
</td></tr>
<tr><td class=line-number>25</td><td class=line>Stuff with the filling and place in a <span style='color: #ddb700'>#baking dish<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #ffffff'>}</span></span>.
</td></tr>
<tr><td class=line-number>26</td><td class=line>
</td></tr>
<tr><td class=line-number>27</td><td class=line><span style='color: #39963e'>&gt;</span> <span style='font-style: italic;color: #39963e'>An aged cheddar is recommended.</span>
</td></tr>
<tr><td class=line-number>28</td><td class=line>
</td></tr>
<tr><td class=line-number>29</td><td class=line>Top each pepper with <span style='color: #f93d5d'>@cheese<span style='font-weight: bold;color: #ffffff'>|</span>cheddar<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>100</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>g</span><span style='font-weight: bold;color: #ffffff'>}(</span><span style='color: #39963e'>grated</span><span style='font-weight: bold;color: #ffffff'>)</span></span> and optionally <span style='color: #f93d5d'>@<span style='font-weight: bold;color: #d79921'>?</span>pecorino<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>50</span><span style='font-weight: bold;color: #ffffff'>%</span><span style='color: #d79921'>g</span><span style='font-weight: bold;color: #ffffff'>}(</span><span style='color: #39963e'>shaved</span><span style='font-weight: bold;color: #ffffff'>)</span></span>.
</td></tr>
<tr><td class=line-number>30</td><td class=line>
</td></tr>
<tr><td class=line-number>31</td><td class=line>Bake in a preheated <span style='color: #ddb700'>#oven</span> at <span style='font-weight: bold;color: #39963e'><span style='font-weight: bold;color: #39963e'>190</span>°C</span> for <span style='font-weight: bold;color: #7dfdfe'>~<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>30</span> <span style='color: #d65d0e'>minutes</span><span style='font-weight: bold;color: #ffffff'>}</span></span> until <span style='color: #f93d5d'>@<span style='font-weight: bold;color: #d79921'>&amp;</span>peppers</span>
</td></tr>
<tr><td class=line-number>32</td><td class=line>are tender and <span style='color: #f93d5d'>@<span style='font-weight: bold;color: #d79921'>&amp;</span>cheese</span> is bubbling.
</td></tr>
<tr><td class=line-number>33</td><td class=line>
</td></tr>
<tr><td class=line-number>34</td><td class=line>Serve with <span style='color: #f93d5d'>@<span style='font-weight: bold;color: #d79921'>@</span>./Sauces/Salsa Verde<span style='font-weight: bold;color: #ffffff'>{</span><span style='font-weight: bold;color: #39963e'>1</span><span style='font-weight: bold;color: #ffffff'>-</span><span style='color: #dc8b3f'>1.5</span> <span style='color: #d79921'>servings</span><span style='font-weight: bold;color: #ffffff'>}</span></span>. <span style='font-style: italic;color: #928374'>-- todo</span>
</td></tr>
</table>
</div>

## References
[^2]: [Cooklang specification](https://github.com/cooklang/spec)
[^3]: [cooklang-rs Extensions](https://github.com/cooklang/cooklang-rs/blob/main/extensions.md)
[^4]: [tree-sitter: Writing tests](https://tree-sitter.github.io/tree-sitter/creating-parsers/5-writing-tests.html)
[^5]: [cooklang-rs Tests](https://github.com/cooklang/cooklang-rs/blob/main/tests/canonical.yaml)

