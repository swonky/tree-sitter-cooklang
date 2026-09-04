#include <assert.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bindings/c/tree_sitter/tree-sitter-cooklang.h"
#include "highlight-map.h"

#include <base16.h>
#include <cairo-svg.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <tree_sitter/api.h>

typedef struct {
	uint32_t start;
	uint32_t end;
	uint32_t capture_index;
} Capture;

typedef uint32_t Codepoint;

typedef struct {
	Capture capture;
	Highlight hl;
	rgb_t color;
} Token;

typedef struct {
	Token *items;
	size_t len;
	size_t capacity;
} TokenVec;

static bool token_vec_push(TokenVec *vec, Token tok)
{
	if (vec->len == vec->capacity) {
		size_t new_capacity = vec->capacity ? vec->capacity * 2 : 64;

		Token *new_items =
		    realloc(vec->items, new_capacity * sizeof(*new_items));

		if (!new_items)
			return false;

		vec->items = new_items;
		vec->capacity = new_capacity;
	}

	vec->items[vec->len++] = tok;
	return true;
}

static char *read_file(const char *path, size_t *length)
{
	FILE *file = fopen(path, "rb");
	if (!file) {
		perror(path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	if (size < 0) {
		fclose(file);
		return NULL;
	}

	char *contents = malloc((size_t)size + 1);
	if (!contents) {
		fclose(file);
		return NULL;
	}

	size_t read = fread(contents, 1, (size_t)size, file);
	fclose(file);

	if (read != (size_t)size) {
		free(contents);
		return NULL;
	}

	contents[size] = '\0';

	if (length)
		*length = (size_t)size;

	return contents;
}

static bool render(const char *source, size_t source_length,
    const TokenVec *tokens, const char *png_path, const char *svg_path,
    base16_scheme scheme)
{
	(void)source_length;

	/*
	 * Calculate the total output length.
	 *
	 * If tokens cover the source exactly once, this will equal
	 * source_length, but don't rely on that.
	 */
	size_t text_length = 0;

	for (size_t i = 0; i < tokens->len; i++) {
		const Token *token = &tokens->items[i];
		text_length += token->capture.end - token->capture.start;
	}

	char *text = malloc(text_length + 1);
	if (!text)
		return false;

	/*
	 * Concatenate token contents.
	 */
	size_t output_position = 0;

	for (size_t i = 0; i < tokens->len; i++) {
		const Token *token = &tokens->items[i];

		size_t length = token->capture.end - token->capture.start;

		memcpy(text + output_position, source + token->capture.start,
		    length);

		output_position += length;
	}

	text[output_position] = '\0';

	/*
	 * Pango layout.
	 */
	PangoContext *context =
	    pango_cairo_font_map_get_default()
		? pango_font_map_create_context(
		      PANGO_FONT_MAP(pango_cairo_font_map_get_default()))
		: NULL;

	if (!context) {
		free(text);
		return false;
	}

	PangoLayout *layout = pango_layout_new(context);

	pango_layout_set_text(layout, text, (int)output_position);

	/*
	 * Attributes.
	 */
	PangoAttrList *attrs = pango_attr_list_new();

	output_position = 0;

	for (size_t i = 0; i < tokens->len; i++) {
		const Token *token = &tokens->items[i];

		size_t length = token->capture.end - token->capture.start;

		guint start = (guint)output_position;
		guint end = (guint)(output_position + length);

		/*
		 * Foreground colour.
		 *
		 * Pango uses 16-bit colour components.
		 */
		PangoAttribute *attr =
		    pango_attr_foreground_new(token->color.r * 257,
			token->color.g * 257, token->color.b * 257);

		attr->start_index = start;
		attr->end_index = end;

		pango_attr_list_insert(attrs, attr);

		if (token->hl.bold) {
			attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
			attr->start_index = start;
			attr->end_index = end;
			pango_attr_list_insert(attrs, attr);
		}

		if (token->hl.italic) {
			attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
			attr->start_index = start;
			attr->end_index = end;
			pango_attr_list_insert(attrs, attr);
		}

		if (token->hl.underline) {
			attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);

			attr->start_index = start;
			attr->end_index = end;
			pango_attr_list_insert(attrs, attr);
		}

		if (token->hl.strikethrough) {
			attr = pango_attr_strikethrough_new(TRUE);
			attr->start_index = start;
			attr->end_index = end;
			pango_attr_list_insert(attrs, attr);
		}

		output_position += length;
	}

	pango_layout_set_attributes(layout, attrs);
	pango_attr_list_unref(attrs);

	/*
	 * Font.
	 */
	PangoFontDescription *font =
	    pango_font_description_from_string("monospace 14");

	pango_layout_set_font_description(layout, font);
	pango_font_description_free(font);

	/*
	 * Determine required surface dimensions.
	 */
	int width;
	int height;

	pango_layout_get_pixel_size(layout, &width, &height);

	/*
	 * Add some padding.
	 */
	const int padding = 20;

	width += padding * 2;
	height += padding * 2;

	/*
	 * PNG.
	 */
	cairo_surface_t *png_surface =
	    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

	cairo_t *cr = cairo_create(png_surface);

	rgb_t background = scheme[0x00];

	/*
	 * SVG.
	 */
	cairo_surface_t *svg_surface =
	    cairo_svg_surface_create(svg_path, width, height);

	cr = cairo_create(svg_surface);

	cairo_set_source_rgb(cr, background.r / 255.0, background.g / 255.0,
	    background.b / 255.0);
	cairo_paint(cr);

	cairo_move_to(cr, padding, padding);

	pango_cairo_update_layout(cr, layout);
	pango_cairo_show_layout(cr, layout);

	cairo_destroy(cr);
	cairo_surface_destroy(svg_surface);

	g_object_unref(layout);
	g_object_unref(context);
	free(text);

	return true;
}

enum error {
	OK = 0,
	ERR_THEME_INVALID_PATH,
	ERR_THEME_NOT_FOUND,
};

static inline bool is_alnum(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	       (c >= '0' && c <= '9');
}

static char *pathjoin(const char *prefix, const char *suffix)
{
	assert(prefix != NULL || suffix != NULL);

	size_t prefix_len = strlen(prefix);
	size_t suffix_len = strlen(prefix);

	bool need_slash = prefix_len > 0 && prefix[prefix_len - 1] != '/';

	size_t len = prefix_len + need_slash + suffix_len + strlen(".yaml") + 1;

	char *path = malloc(len);
	if (!path)
		return NULL;

	snprintf(
	    path, len, "%s%s%s.yaml", prefix, need_slash ? "/" : "", suffix);

	return path;
}

typedef const TSLanguage *(*LanguageFn)(void);
static const TSLanguage *load_language(const char *path, void **handle)
{
	*handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

	if (!*handle) {
		fprintf(stderr, "%s\n", dlerror());
		return NULL;
	}

	const char *filename = strrchr(path, '/');
	filename = filename ? filename + 1 : path;

	size_t len = strlen(filename);

	if (len <= 3 || strcmp(filename + len - 3, ".so") != 0) {
		fprintf(stderr, "invalid language library: %s\n", filename);
		dlclose(*handle);
		*handle = NULL;
		return NULL;
	}

	len -= 3; /* strip ".so" */

	char symbol[256];

	int n = snprintf(
	    symbol, sizeof(symbol), "tree_sitter_%.*s", (int)len, filename);

	if (n < 0 || (size_t)n >= sizeof(symbol)) {
		fprintf(stderr, "language name too long\n");
		dlclose(*handle);
		*handle = NULL;
		return NULL;
	}

	typedef const TSLanguage *(*LanguageFn)(void);

	dlerror();

	LanguageFn language_fn = (LanguageFn)dlsym(*handle, symbol);

	const char *error = dlerror();

	if (error) {
		fprintf(stderr, "failed to find %s: %s\n", symbol, error);
		dlclose(*handle);
		*handle = NULL;
		return NULL;
	}

	return language_fn();
}

int main(int argc, char **argv)
{
	char illegal_char;
	char *path_language;
	char *path_input;

	char *theme_id = "tokyonight/tokyonight";
	char *path_highlights = "queries/highlights.scm";

	switch (argc) {
	case 1:
	case 2:
	case 3:
		fprintf(stderr, "usage: %s <example-file>\n", argv[0]);
		return EXIT_FAILURE;
	case 4:
		path_language = argv[2];
		path_input = argv[3];
		break;
	case 5:
		path_language = argv[2];
		path_highlights = argv[3];
		path_input = argv[4];
		break;
	case 6:
		path_language = argv[2];
		path_highlights = argv[3];
		theme_id = argv[4];
		path_input = argv[5];
		break;
	}

	const char *theme_dir = getenv("BASE16_THEME_PATH");
	if (theme_dir == NULL) {
		fprintf(stderr, "BASE16_THEME_PATH is not set.\n");
		return EXIT_FAILURE;
	}

	const char *theme_path = pathjoin(theme_dir, theme_id);
	if (theme_path == NULL) {
		fprintf(stderr, "Failed to resolve theme path.\n");
		return EXIT_FAILURE;
	}

	size_t source_length;
	char *source = read_file(path_input, &source_length);

	if (!source)
		return EXIT_FAILURE;

	base16_scheme colorscheme;
	int err = load_base16(theme_path, colorscheme);

	if (err != 0) {
		fprintf(stderr, "Failed to load theme: %s.\n", theme_path);
		return EXIT_FAILURE;
	}

	size_t query_length;
	char *query_source = read_file(path_highlights, &query_length);

	if (!query_source) {
		free(source);
		return EXIT_FAILURE;
	}

	void *language_handle;

	TSParser *parser = ts_parser_new();

	const TSLanguage *language = load_language(argv[1], &language_handle);

	if (!language)
		goto cleanup_parser;

	if (!ts_parser_set_language(parser, language)) {
		fprintf(stderr, "failed to set language\n");
		dlclose(language_handle);
		goto cleanup_parser;
	}

	TSTree *tree = ts_parser_parse_string(
	    parser, NULL, source, (uint32_t)source_length);

	if (!tree) {
		fprintf(stderr, "failed to parse %s\n", argv[1]);
		goto cleanup_parser;
	}

	uint32_t error_offset;
	TSQueryError error_type;
	TSQuery *query = ts_query_new(language, query_source,
	    (uint32_t)query_length, &error_offset, &error_type);

	if (!query) {
		fprintf(stderr,
		    "failed to compile highlights.scm at byte %u (error %d)\n",
		    error_offset, error_type);
		ts_tree_delete(tree);
		goto cleanup_parser;
	}

	/*
	 * Collect every highlight capture.
	 */
	Capture *highlights = NULL;
	size_t highlight_count = 0;
	size_t highlight_capacity = 0;

	TSQueryCursor *cursor = ts_query_cursor_new();

	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

	TSQueryMatch match;
	uint32_t capture_index;

	while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
		TSQueryCapture capture = match.captures[capture_index];

		uint32_t start = ts_node_start_byte(capture.node);
		uint32_t end = ts_node_end_byte(capture.node);

		if (start == end)
			continue;

		if (highlight_count == highlight_capacity) {
			size_t new_capacity = highlight_capacity == 0
						  ? 64
						  : highlight_capacity * 2;

			Capture *new_highlights = realloc(
			    highlights, new_capacity * sizeof(*highlights));

			if (!new_highlights) {
				fprintf(stderr, "out of memory\n");
				free(highlights);
				ts_query_cursor_delete(cursor);
				ts_query_delete(query);
				ts_tree_delete(tree);
				goto cleanup_parser;
			}

			highlights = new_highlights;
			highlight_capacity = new_capacity;
		}

		highlights[highlight_count++] = (Capture){
		    .start = start,
		    .end = end,
		    .capture_index = capture.index,
		};
	}

	ts_query_cursor_delete(cursor);

	/*
	 * Walk the ORIGINAL SOURCE.
	 *
	 * Every byte belongs either to a highlight capture or to an
	 * unhighlighted region. This means whitespace, tabs, newlines,
	 * and anything else omitted by highlights.scm are retained.
	 *
	 * Adjacent bytes with the same highlight group are emitted as
	 * one row.
	 */

	uint32_t position = 0;

	TokenVec tokens = {0};

	while (position < source_length) {
		/*
		 * Find a capture covering this byte.
		 *
		 * For overlapping captures, the last matching capture is
		 * used. This is deliberately simple; if exact Tree-sitter
		 * highlight precedence is required, that should be handled
		 * separately.
		 */
		const Capture *active = NULL;

		for (size_t i = 0; i < highlight_count; i++) {
			if (highlights[i].start <= position &&
			    position < highlights[i].end) {
				active = &highlights[i];
			}
		}

		uint32_t end = position + 1;
		Token t = {};
		/*
		 * Extend the run while the same capture continues.
		 */
		while (end < source_length) {
			const Capture *next = NULL;

			for (size_t i = 0; i < highlight_count; i++) {
				if (highlights[i].start <= end &&
				    end < highlights[i].end) {
					next = &highlights[i];
				}
			}

			if (next != active)
				break;

			end++;
		}

		if (active) {
			uint32_t name_length;
			const char *name = ts_query_capture_name_for_id(
			    query, active->capture_index, &name_length);

			Highlight hl = get_colour(name);
			rgb_t color = colorscheme[hl.colour];

			t.capture = (Capture){.start = position, .end = end};
			t.hl = hl;
			t.color = colorscheme[hl.colour];

		} else {
			Capture cap = {
			    .start = position,
			    .end = end,
			};
			t.capture = cap;
			t.color = colorscheme[5];
		}

		token_vec_push(&tokens, t);
		position = end;
	}

	if (!render(source, source_length, &tokens, "output.png", "output.svg",
		colorscheme)) {
		fprintf(stderr, "failed to render\n");
	}
	free(tokens.items);
	free(highlights);
	ts_query_delete(query);
	ts_tree_delete(tree);

cleanup_parser:
	ts_parser_delete(parser);
	dlclose(language_handle);
	free(query_source);
	free(source);

	return EXIT_SUCCESS;
}
