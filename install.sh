#!/usr/bin/env sh

OUTPUT_PATH="build/cooklang.$(date '+%Y%m%d-%H%M%S').so"

tree-sitter generate
tree-sitter build -o $OUTPUT_PATH
install $OUTPUT_PATH ~/.local/share/nvim/site/parser/cooklang.so
cp ./queries/* ~/.config/nvim/queries/cooklang/
