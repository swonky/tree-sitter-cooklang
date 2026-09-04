#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t colour;
  bool bold;
  bool italic;
  bool underline;
  bool strikethrough;
  bool reverse;
  bool undercurl;
} Highlight;

typedef struct {
  const char *capture_group;
  Highlight hl;
} Mapping;

static const Mapping mappings[] = {
    {"attribute", {0x0F, false, false, false, false, false, false}},
    {"attribute.builtin", {0x0F, false, false, false, false, false, false}},

    {"boolean", {0x09, false, false, false, false, false, false}},

    {"character", {0x0B, false, false, false, false, false, false}},
    {"character.special", {0x0C, false, false, false, false, false, false}},

    {"comment", {0x03, false, false, false, false, false, false}},
    {"comment.documentation", {0x03, false, false, false, false, false, false}},
    {"comment.error", {0x08, false, false, false, false, false, false}},
    {"comment.note", {0x0C, false, false, false, false, false, false}},
    {"comment.todo", {0x0F, false, false, false, false, false, false}},
    {"comment.warning", {0x09, false, false, false, false, false, false}},

    {"constant", {0x09, false, false, false, false, false, false}},
    {"constant.builtin", {0x09, false, false, false, false, false, false}},
    {"constant.macro", {0x09, false, false, false, false, false, false}},

    {"constructor", {0x0A, false, false, false, false, false, false}},

    {"diff.delta", {0x0A, false, false, false, false, false, false}},
    {"diff.minus", {0x08, false, false, false, false, false, false}},
    {"diff.plus", {0x0B, false, false, false, false, false, false}},

    {"function", {0x0D, false, false, false, false, false, false}},
    {"function.builtin", {0x0D, false, false, false, false, false, false}},
    {"function.call", {0x0D, false, false, false, false, false, false}},
    {"function.macro", {0x0D, false, false, false, false, false, false}},
    {"function.method", {0x0D, false, false, false, false, false, false}},
    {"function.method.call", {0x0D, false, false, false, false, false, false}},

    {"keyword", {0x0E, false, false, false, false, false, false}},
    {"keyword.conditional", {0x0E, false, false, false, false, false, false}},
    {"keyword.conditional.ternary",
     {0x0E, false, false, false, false, false, false}},
    {"keyword.coroutine", {0x0E, false, false, false, false, false, false}},
    {"keyword.debug", {0x0E, false, false, false, false, false, false}},
    {"keyword.directive", {0x0E, false, false, false, false, false, false}},
    {"keyword.directive.define",
     {0x0E, false, false, false, false, false, false}},
    {"keyword.exception", {0x08, false, false, false, false, false, false}},
    {"keyword.function", {0x0E, false, false, false, false, false, false}},
    {"keyword.import", {0x0E, false, false, false, false, false, false}},
    {"keyword.modifier", {0x0E, false, false, false, false, false, false}},
    {"keyword.operator", {0x0E, false, false, false, false, false, false}},
    {"keyword.repeat", {0x0E, false, false, false, false, false, false}},
    {"keyword.return", {0x0E, false, false, false, false, false, false}},
    {"keyword.type", {0x0E, false, false, false, false, false, false}},

    {"label", {0x0E, false, false, false, false, false, false}},

    {"markup", {0x0F, false, false, false, false, false, false}},
    {"markup.heading", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.1", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.2", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.3", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.4", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.5", {0x0F, false, false, false, false, false, false}},
    {"markup.heading.6", {0x0F, false, false, false, false, false, false}},
    {"markup.italic", {0x0F, false, true, false, false, false, false}},
    {"markup.link", {0x0F, false, false, true, false, false, false}},
    {"markup.link.label", {0x0F, false, false, true, false, false, false}},
    {"markup.link.url", {0x0D, false, false, true, false, false, false}},
    {"markup.list", {0x0F, false, false, false, false, false, false}},
    {"markup.list.checked", {0x0B, false, false, false, false, false, false}},
    {"markup.list.unchecked", {0x0B, false, false, false, false, false, false}},
    {"markup.math", {0x0C, false, false, false, false, false, false}},
    {"markup.quote", {0x0F, false, false, false, false, false, false}},
    {"markup.raw", {0x0F, false, false, false, false, false, false}},
    {"markup.raw.block", {0x0F, false, false, false, false, false, false}},
    {"markup.strikethrough", {0x0F, false, false, false, true, false, false}},
    {"markup.strong", {0x0F, true, false, false, false, false, false}},
    {"markup.underline", {0x0F, false, false, true, false, false, false}},

    {"module", {0x0A, false, false, false, false, false, false}},
    {"module.builtin", {0x0A, false, false, false, false, false, false}},

    {"number", {0x09, false, false, false, false, false, false}},
    {"number.float", {0x09, false, false, false, false, false, false}},

    {"operator", {0x05, false, false, false, false, false, false}},

    {"property", {0x05, false, false, false, false, false, false}},

    {"punctuation.bracket", {0x05, false, false, false, false, false, false}},
    {"punctuation.delimiter", {0x05, false, false, false, false, false, false}},
    {"punctuation.special", {0x0C, false, false, false, false, false, false}},

    {"string", {0x0B, false, false, false, false, false, false}},
    {"string.documentation", {0x0B, false, false, false, false, false, false}},
    {"string.escape", {0x0C, false, false, false, false, false, false}},
    {"string.regexp", {0x0C, false, false, false, false, false, false}},
    {"string.special", {0x0C, false, false, false, false, false, false}},
    {"string.special.path", {0x0C, false, false, false, false, false, false}},
    {"string.special.symbol", {0x0C, false, false, false, false, false, false}},
    {"string.special.url", {0x0D, false, false, true, false, false, false}},

    {"tag", {0x0A, false, false, false, false, false, false}},
    {"tag.attribute", {0x05, false, false, false, false, false, false}},
    {"tag.builtin", {0x0C, false, false, false, false, false, false}},
    {"tag.delimiter", {0x05, false, false, false, false, false, false}},

    {"type", {0x0A, false, false, false, false, false, false}},
    {"type.builtin", {0x0A, false, false, false, false, false, false}},
    {"type.definition", {0x0A, false, false, false, false, false, false}},

    {"variable", {0x05, false, false, false, false, false, false}},
    {"variable.builtin", {0x05, false, false, false, false, false, false}},
    {"variable.member", {0x05, false, false, false, false, false, false}},
    {"variable.parameter", {0x05, false, false, false, false, false, false}},
    {"variable.parameter.builtin",
     {0x05, false, false, false, false, false, false}},
};

#define MAPPING_COUNT (sizeof(mappings) / sizeof(mappings[0]))

static Highlight get_colour(const char *capture_group) {
  size_t low = 0;
  size_t high = MAPPING_COUNT;

  while (low < high) {
    size_t mid = low + (high - low) / 2;
    int cmp = strcmp(capture_group, mappings[mid].capture_group);

    if (cmp == 0)
      return mappings[mid].hl;

    if (cmp < 0)
      high = mid;
    else
      low = mid + 1;
  }

  return (Highlight){
      .colour = 0x05,
  };
}
