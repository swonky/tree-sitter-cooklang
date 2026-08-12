#include "tree_sitter/parser.h"
#include "unicode_tables.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum TokenType {
	_UNSPECIFIED,
	// frontmatter
	FMAT_PREFIX,
	FMAT_CONTENT,
	FMAT_SUFFIX,
	// plain text content
	TXT_PLAIN,
	TXT_IDENT,
	EOL,
	LINE_BLANK,
	COMMENT_LINE,
	COMMENT,
	// headings
	// SYM_HEADING, // [=]+
	// TXT_HEADING,
	// symbolic operators
	SYM_PAREN_O, // (
	SYM_PAREN_C, // )
	SYM_BRACE_O, // {
	SYM_BRACE_C, // }
	SYM_COLON,   // :
	SYM_VBAR,    // |
	SYM_PERCENT, // %
	SYM_SOLIDUS, // /
	SYM_HYPHEN,  // -
	SYM_COMMAT,  // @
	SYM_NUM,     // #
	SYM_TILDE,   // ~
	SYM_AMPERSAND,
	SYM_PLUS,
	SYM_QUESTION,
	SYM_EQUAL,
	SYM_GT,
	// composite operators
	SYM_BRACE_PAREN, // }(
	SYM_GT_GT,
	// integer
	LIT_INTEGER,
	LIT_DECIMAL,
	WS,
};

static const UnicodeChar sym_map[128] = {
    ['('] = SYM_PAREN_O,
    [')'] = SYM_PAREN_C,
    ['{'] = SYM_BRACE_O,
    ['}'] = SYM_BRACE_C,
    [':'] = SYM_COLON,
    ['|'] = SYM_VBAR,
    ['%'] = SYM_PERCENT,
    ['/'] = SYM_SOLIDUS,
    ['-'] = SYM_HYPHEN,
    ['@'] = SYM_COMMAT,
    ['#'] = SYM_NUM,
    ['~'] = SYM_TILDE,
    ['&'] = SYM_AMPERSAND,
    ['+'] = SYM_PLUS,
    ['?'] = SYM_QUESTION,
    ['='] = SYM_EQUAL,
    ['>'] = SYM_GT,
};

static inline enum TokenType get_token(UnicodeChar c)
{
	if (c >= 128)
		return _UNSPECIFIED;
	return sym_map[c];
}

// enum {
// 	PREFIX_HEADING = '=',
// 	PREFIX_NOTE = '>',
// };

typedef struct {
	/* persistent fields */
	bool in_metadata;
	bool first;
	/* transient fields */
	bool saw_square;
	bool saw_integer;
	bool has_advanced;
} Scanner;

typedef bool (*Asserter)(UnicodeChar);

static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }
static inline void skip(TSLexer *lexer) { lexer->advance(lexer, true); }
static inline void mark_end(TSLexer *lexer) { lexer->mark_end(lexer); }
static inline unsigned int get_column(TSLexer *lexer)
{
	return lexer->get_column(lexer);
}

static inline bool eof(TSLexer *lexer) { return lexer->eof(lexer); }
static inline bool is_num(UnicodeChar c) { return (c >= '0' && c <= '9'); }
static inline bool is_posnum(UnicodeChar c) { return (c >= '1' && c <= '9'); }
static inline bool is_upper(UnicodeChar c) { return (c >= 'A' && c <= 'Z'); }
static inline bool is_lower(UnicodeChar c) { return (c >= 'a' && c <= 'z'); }

static inline bool is_def_prefix(UnicodeChar c)
{
	return c == '@' || c == '#' || c == '~';
}

static inline bool is_alpha(UnicodeChar c)
{
	return is_upper(c) || is_lower(c);
}

static inline bool is_alnum(UnicodeChar c) { return is_num(c) || is_alpha(c); }

static inline int skip_while(TSLexer *lexer, Asserter fn)
{
	int count = 0;
	while (!eof(lexer) && fn(lexer->lookahead)) {
		skip(lexer);
		count++;
	}
	return count;
}

static inline int advance_while(TSLexer *lexer, Asserter fn)
{
	int count = 0;
	while (!eof(lexer) && fn(lexer->lookahead)) {
		advance(lexer);
		count++;
	}
	return count;
}

// consume the rest of the line (incl. newline characters)
static inline int advance_rol(TSLexer *lexer)
{
	int count = 0;
	while (!eof(lexer) && get_column(lexer) != 0) {
		advance(lexer);
	}
	return count;
}

static bool scan_hyphen_token(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	bool is_line_start = lexer->get_column(lexer) == 0;

	if (advance_while(lexer, is_ws_horiz) > 0)
		scanner->has_advanced = true;

	if (lexer->lookahead != '-')
		return false;

	advance(lexer);

	if (lexer->lookahead != '-') {
		if (valid_symbols[SYM_HYPHEN]) {
			mark_end(lexer);
			lexer->result_symbol = SYM_HYPHEN;
			return true;
		}
		return false;
	}

	advance(lexer);

	if (lexer->lookahead == '-' && is_line_start) {
		if (!scanner->in_metadata && scanner->first &&
		    valid_symbols[FMAT_PREFIX]) {
			(void)advance_rol(lexer);
			mark_end(lexer);
			scanner->in_metadata = true;
			lexer->result_symbol = FMAT_PREFIX;
			return true;
		}
		if (scanner->in_metadata && valid_symbols[FMAT_SUFFIX]) {
			advance(lexer);
			mark_end(lexer);
			scanner->in_metadata = false;
			lexer->result_symbol = FMAT_SUFFIX;
			return true;
		}
	}

	if (is_line_start && valid_symbols[COMMENT_LINE]) {
		while (!eof(lexer) && !is_ws_vert(lexer->lookahead))
			advance(lexer);
		mark_end(lexer);
		lexer->result_symbol = COMMENT_LINE;
		return true;
	}
	if (valid_symbols[COMMENT]) {
		while (!eof(lexer) && !is_ws_vert(lexer->lookahead))
			advance(lexer);
		mark_end(lexer);
		lexer->result_symbol = COMMENT;
		return true;
	}
	return false;
}

static bool match_word(TSLexer *lexer, const char *s)
{
	while (*s) {
		if (eof(lexer))
			return false;
		UnicodeChar c = lexer->lookahead;
		char expected = *s++;
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		if (c != expected)
			return false;
		advance(lexer);
	}
	return true;
}

static inline bool is_temp_delimiter(TSLexer *lexer)
{
	return eof(lexer) || !is_alnum(lexer->lookahead);
}

static inline bool is_degree_delimiter(TSLexer *lexer)
{
	if (eof(lexer) || !is_alnum(lexer->lookahead))
		return true;
	if (lexer->lookahead == 'C' || lexer->lookahead == 'F') {
		advance(lexer);
		return eof(lexer) || !is_alnum(lexer->lookahead);
	}
	return false;
}

static bool scan_temperature_suffix(TSLexer *lexer)
{
	(void)advance_while(lexer, is_ws_horiz);

	return !is_ws_vert(lexer->lookahead) &&
	       (is_degree_symbol(lexer->lookahead) ||
		   (match_word(lexer, "c") &&
		       (is_temp_delimiter(lexer) ||
			   (match_word(lexer, "elsius") &&
			       is_temp_delimiter(lexer)))) ||
		   (match_word(lexer, "f") &&
		       (is_temp_delimiter(lexer) ||
			   (match_word(lexer, "ahrenheit") &&
			       is_temp_delimiter(lexer)))) ||
		   (match_word(lexer, "deg") &&
		       (is_degree_delimiter(lexer) ||
			   (match_word(lexer, "ree") &&
			       (is_degree_delimiter(lexer) ||
				   (match_word(lexer, "s") &&
				       is_degree_delimiter(lexer)))))));
}

static bool scan_heading_text(TSLexer *lexer, Scanner *scanner)
{
	(void)skip_while(lexer, is_ws_horiz);
	bool seen = false;
	while (!eof(lexer)) {
		switch (lexer->lookahead) {
		case '\n':
		case '\r':
		case '=':
			mark_end(lexer);
			return seen;
		case '[':
			scanner->saw_square = true;
			/* fall through */
		case '-':
			mark_end(lexer);
			advance(lexer);
			if (lexer->lookahead == '-') {
				return seen;
			}
			/* fall through */
		default:
			advance(lexer);
		}
		seen = true;
	}
	mark_end(lexer);
	return seen;
}

static inline void set_advanced(Scanner *s) { s->has_advanced = true; }

static bool scan_ws(TSLexer *lex, Scanner *s, const bool *vs)
{
	if (get_column(lex) == 0 && vs[TXT_PLAIN])
		return false;

	if (advance_while(lex, is_ws_horiz) != 0)
		set_advanced(s);

	if (!s->has_advanced || !vs[WS]) {
		return false;
	}
	lex->result_symbol = WS;
	mark_end(lex);
	return true;
}

static bool scan_numbers(TSLexer *lex, Scanner *s, const bool *vs)
{
	if (eof(lex) || !(vs[LIT_INTEGER] || vs[LIT_DECIMAL]))
		return false;

	// .digits
	if (lex->lookahead == '.') {
		if (!vs[LIT_DECIMAL])
			return false;

		advance(lex);
		set_advanced(s);

		if (!is_num(lex->lookahead))
			return false;

		while (is_num(lex->lookahead))
			advance(lex);

		if (is_alpha(lex->lookahead))
			return false;

		lex->result_symbol = LIT_DECIMAL;
		mark_end(lex);
		return true;
	}

	if (!is_num(lex->lookahead))
		return false;

	// Integer portion.
	s->saw_integer = true;

	if (lex->lookahead == '0') {
		advance(lex);
		set_advanced(s);

		if (is_num(lex->lookahead))
			return false;
	} else {
		while (is_num(lex->lookahead)) {
			advance(lex);
			set_advanced(s);
		}
	}

	// Integer.
	if (lex->lookahead != '.' && !is_alpha(lex->lookahead)) {
		if (!vs[LIT_INTEGER])
			return false;
		lex->result_symbol = LIT_INTEGER;
		mark_end(lex);
		return true;
	}

	// Decimal unavailable: return the integer, leaving '.' untouched.
	if (!vs[LIT_DECIMAL]) {
		if (!vs[LIT_INTEGER])
			return false;

		lex->result_symbol = LIT_INTEGER;
		mark_end(lex);
		return true;
	}

	// Decimal.
	advance(lex);
	set_advanced(s);

	while (is_num(lex->lookahead))
		advance(lex);

	if (is_alpha(lex->lookahead))
		return false;

	lex->result_symbol = LIT_DECIMAL;
	mark_end(lex);
	return true;
}

static bool scan_sym(TSLexer *lexer, const bool *valid_symbols)
{
	if (eof(lexer))
		return false;

	if (lexer->lookahead == '}') {
		if (!(valid_symbols[SYM_BRACE_C] ||
			valid_symbols[SYM_BRACE_PAREN]))
			return false;

		advance(lexer);
		if (!valid_symbols[SYM_BRACE_PAREN] ||
		    (!eof(lexer) && lexer->lookahead != '(')) {
			lexer->result_symbol = SYM_BRACE_C;
			mark_end(lexer);
			return true;
		}

		advance(lexer);
		lexer->result_symbol = SYM_BRACE_PAREN;
		mark_end(lexer);
		return true;
	}

	enum TokenType token = get_token(lexer->lookahead);
	if (token != _UNSPECIFIED) {
		if (!valid_symbols[token])
			return false;
		lexer->result_symbol = token;
		advance(lexer);
		mark_end(lexer);
		return true;
	}

	return false;
	// switch (lexer->lookahead) {
	//
	// case '@':
	// 	return submit(lexer, valid_symbols, SYM_COLON);
	// case '#':
	// 	return submit(lexer, valid_symbols, SYM_COLON);
	// case '~':
	// 	return submit(lexer, valid_symbols, SYM_COLON);
	// case ':':
	// 	return submit(lexer, valid_symbols, SYM_COLON);
	// case '|':
	// 	return submit(lexer, valid_symbols, SYM_VBAR);
	// case '%':
	// 	return submit(lexer, valid_symbols, SYM_PERCENT);
	// case '(':
	// 	return submit(lexer, valid_symbols, SYM_PAREN_O);
	// case ')':
	// 	return submit(lexer, valid_symbols, SYM_PAREN_C);
	// case '{':
	// 	return submit(lexer, valid_symbols, SYM_BRACE_O);
	// case '}':
	// 	if (!(valid_symbols[SYM_BRACE_C] ||
	// 		valid_symbols[SYM_BRACE_PAREN]))
	// 		return false;
	//
	// 	advance(lexer);
	// 	if (!valid_symbols[SYM_BRACE_PAREN] ||
	// 	    (!eof(lexer) && lexer->lookahead != '(')) {
	// 		lexer->result_symbol = SYM_BRACE_C;
	// 		mark_end(lexer);
	// 		return true;
	// 	}
	//
	// 	advance(lexer);
	// 	lexer->result_symbol = SYM_BRACE_PAREN;
	// 	mark_end(lexer);
	// 	return true;
	//
	// default:
	// 	if (valid_symbols[SYM_SOLIDUS] &&
	// 	    is_solidus(lexer->lookahead)) {
	// 		advance(lexer);
	// 		mark_end(lexer);
	// 		lexer->result_symbol = SYM_SOLIDUS;
	// 		return true;
	// 	}
	// 	return false;
	// }
}

static bool scan_text(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	bool seen = scanner->has_advanced;
	bool escape = false;

	if (seen)
		mark_end(lexer);

	while (!eof(lexer)) {
		scanner->saw_square = lexer->lookahead == '[';

		/* skip logic upon ESCAPE char */
		if (escape) {
			advance(lexer);
			mark_end(lexer);
			seen = true;
			escape = false;
			continue;
		}

		/* terminate if temperature found */
		if (scanner->saw_integer || is_posnum(lexer->lookahead)) {
			mark_end(lexer);
			while (is_num(lexer->lookahead))
				advance(lexer);
			if (scan_temperature_suffix(lexer))
				return seen;
			scanner->saw_integer = false;
		}

		mark_end(lexer);

		if (!seen) {
			unsigned int pos = get_column(lexer);
			if (scan_temperature_suffix(lexer))
				return false;
			if (pos != get_column(lexer)) {
				mark_end(lexer);
				seen = true;
			}
		}

		UnicodeChar c = lexer->lookahead;

		if (c == '\\') {
			advance(lexer);
			escape = true;
			continue;
		}

		if (is_ws_vert(c))
			return seen;

		// if (contains(c, sym_tags)) {
		// 	advance(lexer);
		// 	bool no_name = lexer->lookahead == '{';
		// 	if (c == '~') {
		// 		if (no_name)
		// 			return seen;
		// 		while (!eof(lexer)) {
		// 			UnicodeChar nc = lexer->lookahead;
		// 			if (nc == '{')
		// 				return seen;
		// 			if (is_ws_horiz(nc) || is_ws_vert(nc) ||
		// 			    is_punc(nc))
		// 				break;
		// 			advance(lexer);
		// 		}
		// 	} else if (!is_ws_horiz(lexer->lookahead) && !no_name)
		// 		return seen;
		// }

		// if (get_column(lexer) == 0) {
		// 	switch (c) {
		// 	case PREFIX_HEADING:
		// 	case PREFIX_NOTE:
		// 		mark_end(lexer);
		// 		return seen;
		// 	}
		// }

		if (valid_symbols[WS] && is_ws_horiz(lexer->lookahead)) {
			mark_end(lexer);
			return seen;
		}

		if (c == '-' || c == '[') {
			mark_end(lexer);
			if (valid_symbols[SYM_HYPHEN])
				return seen;
			advance(lexer);
			if (lexer->lookahead == '-')
				return seen;
			advance(lexer);
			mark_end(lexer);
			seen = true;
			continue;
		}

		enum TokenType token = get_token(lexer->lookahead);
		if (token != _UNSPECIFIED && valid_symbols[token]) {
			mark_end(lexer);
			return seen;
		}

		advance(lexer);
		mark_end(lexer);
		seen = true;

		// switch (c) {
		// case '[':
		// case '-':
		// 	mark_end(lexer);
		// 	if (valid_symbols[sym_hyphen])
		// 		return seen;
		// 	advance(lexer);
		// 	if (lexer->lookahead == '-')
		// 		return seen;
		// 	advance(lexer);
		// 	break;
		// case '%':
		// 	if (valid_symbols[sym_percent]) {
		// 		mark_end(lexer);
		// 		return seen;
		// 	}
		// 	advance(lexer);
		// 	break;
		//
		// case '}':
		// 	if (valid_symbols[sym_brace_c] ||
		// 	    valid_symbols[sym_brace_paren]) {
		// 		mark_end(lexer);
		// 		return seen;
		// 	}
		// 	advance(lexer);
		// 	break;
		// case ')':
		// 	if (valid_symbols[sym_paren_c]) {
		// 		mark_end(lexer);
		// 		return seen;
		// 	}
		// 	advance(lexer);
		// 	break;
		// default:
		//
		//
		// 	if (valid_symbols[sym_solidus] &&
		// 	    is_solidus(lexer->lookahead)) {
		// 		mark_end(lexer);
		// 		return seen;
		// 	}
		// 	advance(lexer);
		// }
	}

	return seen;
}

static bool scan_metadata_content(TSLexer *lexer)
{
	if (eof(lexer))
		return false;
	int nchar = 0;
	while (!eof(lexer)) {
		if (lexer->get_column(lexer) == 0 && lexer->lookahead == '-')
			break;
		advance(lexer);
		nchar++;
	}
	mark_end(lexer);
	return nchar > 0;
}

static bool scan_newline(TSLexer *lexer, const bool *valid_symbols)
{
	if (valid_symbols[LINE_BLANK] && lexer->get_column(lexer) == 0) {
		while (!eof(lexer) && is_ws_horiz(lexer->lookahead))
			advance(lexer);
		if (!eof(lexer) && is_ws_vert(lexer->lookahead)) {
			mark_end(lexer);
			lexer->result_symbol = LINE_BLANK;
			return true;
		}
	}
	if (!valid_symbols[EOL]) {
		return false;
	}
	if (!is_ws_vert(lexer->lookahead))
		return false;
	if (lexer->lookahead == '\r') {
		advance(lexer);
		if (!eof(lexer) && lexer->lookahead == '\n')
			advance(lexer);
	} else {
		advance(lexer);
	}
	mark_end(lexer);
	lexer->result_symbol = EOL;
	return true;
}

static bool scan_heading(TSLexer *lexer)
{
	if (lexer->get_column(lexer) != 0 || lexer->lookahead != '=')
		return false;
	while (!eof(lexer) && lexer->lookahead == '=')
		advance(lexer);
	mark_end(lexer);
	return true;
}

static bool scan_block_start(TSLexer *lexer, Scanner *scanner)
{
	if (scanner->saw_square && lexer->lookahead == '-')
		return true;
	if (lexer->lookahead == '[') {
		advance(lexer);
		return lexer->lookahead == '-';
	}
	return false;
}

static bool scan_block(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	if (!scanner->saw_square) {
		(void)skip_while(lexer, is_ws_horiz);
		(void)skip_while(lexer, is_ws_vert);
	}

	bool is_line_start =
	    (scanner->saw_square && lexer->get_column(lexer) == 1) ||
	    (!scanner->saw_square && lexer->get_column(lexer) == 0);

	if (!scan_block_start(lexer, scanner))
		return false;

	advance(lexer);

	while (!eof(lexer)) {
		if (lexer->lookahead != '-') {
			advance(lexer);
			continue;
		}
		advance(lexer);
		if (lexer->lookahead == ']') {
			advance(lexer);
			mark_end(lexer);
			(void)advance_while(lexer, is_ws_horiz);
			if (is_line_start &&
			    (eof(lexer) || is_ws_vert(lexer->lookahead))) {
				if (valid_symbols[COMMENT_LINE]) {
					lexer->result_symbol = COMMENT_LINE;
					return true;
				}
			}
			if (valid_symbols[COMMENT]) {
				lexer->result_symbol = COMMENT;
				return true;
			}
			return false;
		}
	}
	mark_end(lexer);
	return true;
}

static bool scan_identifier(TSLexer *lexer)
{
	bool seen = false;
	bool first_char = true;
	bool first_word_done = false;
	bool escape = false;

	while (!eof(lexer)) {
		if (escape) {
			advance(lexer);
			seen = true;
			escape = false;
			continue;
		}

		UnicodeChar c = lexer->lookahead;
		if (first_char && contains(c, sym_modifiers)) {
			return false;
		}
		first_char = false;

		if (c == '{' || c == '|') {
			mark_end(lexer);
			return seen;
		}
		if (is_ws_vert(c) || c == '@' || c == '#' || c == '~')
			break;
		if (c == '\\') {
			advance(lexer);
			escape = true;
			continue;
		}
		if (is_ws_horiz(c) || is_punc(c)) {
			if (!first_word_done) {
				mark_end(lexer);
				first_word_done = true;
			}
			advance(lexer);
			continue;
		}
		advance(lexer);
		seen = true;
	}
	if (!first_word_done) {
		mark_end(lexer);
	}
	return seen;
}

/*
 * Initialises persistent field values.
 * Modifications to these values persist across scanner instances.
 */
static inline void init_persistent_fields(Scanner *s)
{
	s->in_metadata = false;
	s->first = true;
}

/*
 * Sets transient field values.
 * These fields do not persist across scanner instances.
 */
static inline void init_transient_fields(Scanner *s)
{
	s->saw_square = false;
	s->saw_integer = false;
	s->has_advanced = false;
}

void *tree_sitter_cooklang_external_scanner_create(void)
{
	Scanner *s = calloc(1, sizeof(Scanner));
	init_persistent_fields(s);
	init_transient_fields(s);
	return s;
}

void tree_sitter_cooklang_external_scanner_destroy(void *payload)
{
	free(payload);
}

unsigned tree_sitter_cooklang_external_scanner_serialize(
    void *payload, char *buffer)
{
	Scanner *scanner = payload;
	buffer[0] = (char)scanner->in_metadata;
	buffer[1] = (char)scanner->first;

	(void)scanner->saw_square;
	(void)scanner->saw_integer;
	(void)scanner->has_advanced;

	return 2;
}

void tree_sitter_cooklang_external_scanner_deserialize(
    void *payload, const char *buffer, unsigned length)
{
	Scanner *scanner = payload;

	init_transient_fields(scanner);

	if (length < 2) {
		scanner->in_metadata = false;
		scanner->first = true;
		return;
	}

	scanner->in_metadata = (bool)buffer[0];
	scanner->first = (bool)buffer[1];
}

static bool dispatch(TSLexer *lexer, Scanner *scanner, const bool *vs)
{
	if ((vs[EOL] || vs[LINE_BLANK]) && scan_newline(lexer, vs)) {
		return true;
	}
	if ((vs[FMAT_PREFIX] || vs[FMAT_SUFFIX] || vs[COMMENT] ||
		vs[SYM_HYPHEN]) &&
	    scan_hyphen_token(lexer, scanner, vs)) {
		return true;
	}
	if (vs[FMAT_CONTENT] && scanner->in_metadata &&
	    scan_metadata_content(lexer)) {
		lexer->result_symbol = FMAT_CONTENT;
		return true;
	}
	// if (vs[SYM_HEADING] && scan_heading(lexer)) {
	// 	lexer->result_symbol = SYM_HEADING;
	// 	return true;
	// }
	// if (vs[TXT_HEADING] && scan_heading_text(lexer, scanner)) {
	// 	lexer->result_symbol = TXT_HEADING;
	// 	return true;
	// }
	//
	if (scan_ws(lexer, scanner, vs))
		return true;

	if (scan_sym(lexer, vs))
		return true;

	if ((vs[LIT_INTEGER] || vs[LIT_DECIMAL]) &&
	    scan_numbers(lexer, scanner, vs))
		return true;

	if (vs[TXT_PLAIN] && scan_text(lexer, scanner, vs)) {
		lexer->result_symbol = TXT_PLAIN;
		return true;
	}

	if (vs[TXT_IDENT] && scan_identifier(lexer)) {
		lexer->result_symbol = TXT_IDENT;
		return true;
	}
	return ((vs[COMMENT] || vs[COMMENT_LINE]) &&
		scan_block(lexer, scanner, vs));
}

bool tree_sitter_cooklang_external_scanner_scan(
    void *payload, TSLexer *lexer, const bool *valid_symbols)
{
	Scanner *scanner = payload;
	if (dispatch(lexer, scanner, valid_symbols)) {
		if (scanner->first && lexer->result_symbol != EOL)
			scanner->first = false;
		return true;
	}
	return false;
}
