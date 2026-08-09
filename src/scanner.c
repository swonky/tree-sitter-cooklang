#include "tree_sitter/parser.h"
#include "unicode_tables.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum TokenType {
	METADATA_START,
	METADATA_CONTENT,
	METADATA_END,
	TEXT,
	IDENTIFIER,
	HIDDEN_MODIFIER,
	NEWLINE,
	EMPTY_LINE,
	COMMENT_LINE,
	COMMENT,
	HEADING_START,
	HEADING_TEXT,
	OPEN_PAREN,
	OPEN_BRACE,
	CLOSE_BRACE,
	CLOSE_BRACE_OPEN_PAREN,
	COLON,
	VERT_BAR,
};

enum {
	PREFIX_HEADING = '=',
	PREFIX_NOTE = '>',
};

typedef struct {
	bool in_metadata;
	bool first;
	bool saw_square;
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
static inline bool is_modifier(UnicodeChar c)
{
	return c == '@' || c == '?' || c == '&' || c == '-' || c == '+';
}
static inline bool is_alnum(UnicodeChar c)
{
	return is_num(c) || is_upper(c) || is_lower(c);
}
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

static bool scan_hyphen_token(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	bool is_line_start = lexer->get_column(lexer) == 0;

	(void)skip_while(lexer, is_ws_horiz);

	if (lexer->lookahead != '-')
		return false;

	advance(lexer);

	if (lexer->lookahead != '-') {
		if (valid_symbols[HIDDEN_MODIFIER]) {
			mark_end(lexer);
			lexer->result_symbol = HIDDEN_MODIFIER;
			return true;
		}
		return false;
	}

	advance(lexer);

	if (lexer->lookahead == '-' && is_line_start) {
		if (!scanner->in_metadata && scanner->first &&
		    valid_symbols[METADATA_START]) {
			advance(lexer);
			(void)advance_while(lexer, is_ws_horiz);
			(void)advance_while(lexer, is_ws_vert);
			mark_end(lexer);
			scanner->in_metadata = true;
			lexer->result_symbol = METADATA_START;
			return true;
		}
		if (scanner->in_metadata && valid_symbols[METADATA_END]) {
			advance(lexer);
			mark_end(lexer);
			scanner->in_metadata = false;
			lexer->result_symbol = METADATA_END;
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

static bool scan_text(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	bool seen = false;
	bool escape = false;

	if (eof(lexer))
		return false;

	switch (lexer->lookahead) {
	case '{':
		if (valid_symbols[OPEN_BRACE]) {
			lexer->result_symbol = OPEN_BRACE;
			advance(lexer);
			mark_end(lexer);
			return true;
		}
		break;
	case '(':
		if (valid_symbols[OPEN_PAREN]) {
			lexer->result_symbol = OPEN_PAREN;
			advance(lexer);
			mark_end(lexer);
			return true;
		}
		break;
	case '}':
		if (valid_symbols[CLOSE_BRACE]) {
			advance(lexer);
			if (valid_symbols[CLOSE_BRACE_OPEN_PAREN] &&
			    lexer->lookahead == '(') {
				advance(lexer);
				lexer->result_symbol = CLOSE_BRACE_OPEN_PAREN;
			} else
				lexer->result_symbol = CLOSE_BRACE;
			mark_end(lexer);
			return true;
		}
		break;
	case ':':
		if (valid_symbols[COLON]) {
			lexer->result_symbol = COLON;
			advance(lexer);
			mark_end(lexer);
			return true;
		}
		break;
	case '|':
		if (valid_symbols[VERT_BAR]) {
			lexer->result_symbol = VERT_BAR;
			advance(lexer);
			mark_end(lexer);
			return true;
		}
		break;
		// case ')':
	}

	if (!valid_symbols[TEXT])
		return false;

	lexer->result_symbol = TEXT;

	while (!eof(lexer)) {
		scanner->saw_square = lexer->lookahead == '[';
		if (escape) {
			advance(lexer);
			mark_end(lexer);
			seen = true;
			escape = false;
			continue;
		}

		if (is_posnum(lexer->lookahead)) {
			mark_end(lexer);
			while (is_num(lexer->lookahead))
				advance(lexer);
			if (scan_temperature_suffix(lexer))
				return seen;
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

		if (is_def_prefix(c)) {
			advance(lexer);
			bool no_name = lexer->lookahead == '{';
			if (c == '~') {
				if (no_name)
					return seen;
				while (!eof(lexer)) {
					UnicodeChar nc = lexer->lookahead;
					if (nc == '{')
						return seen;
					if (is_ws_horiz(nc) || is_ws_vert(nc) ||
					    is_punc(nc))
						break;
					advance(lexer);
				}
			} else if (!is_ws_horiz(lexer->lookahead) && !no_name)
				return seen;
		}

		if (get_column(lexer) == 0) {
			switch (c) {
			case PREFIX_HEADING:
			case PREFIX_NOTE:
				mark_end(lexer);
				return seen;
			}
		}

		switch (c) {
		case '[':
		case '-':
			mark_end(lexer);
			advance(lexer);
			if (lexer->lookahead == '-') {
				return seen;
			}
			break;
		default:
			advance(lexer);
		}

		mark_end(lexer);
		seen = true;
	}

	return seen;
}

static bool scan_metadata_content(TSLexer *lexer)
{
	if (eof(lexer))
		return false;
	while (!eof(lexer)) {
		if (lexer->get_column(lexer) == 0 && lexer->lookahead == '-')
			break;
		advance(lexer);
		mark_end(lexer);
	}
	return true;
}

static bool scan_newline(TSLexer *lexer, const bool *valid_symbols)
{
	if (valid_symbols[EMPTY_LINE] && lexer->get_column(lexer) == 0) {
		while (!eof(lexer) && is_ws_horiz(lexer->lookahead))
			advance(lexer);
		if (!eof(lexer) && is_ws_vert(lexer->lookahead)) {
			mark_end(lexer);
			lexer->result_symbol = EMPTY_LINE;
			return true;
		}
	}
	if (!valid_symbols[NEWLINE]) {
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
	lexer->result_symbol = NEWLINE;
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
		if (first_char && is_modifier(c)) {
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

void *tree_sitter_cooklang_external_scanner_create(void)
{
	Scanner *scanner = calloc(1, sizeof(Scanner));
	scanner->in_metadata = false;
	scanner->first = true;
	scanner->saw_square = false;
	return scanner;
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

	// Intentionally not serialised: saw_square is transient.
	(void)scanner->saw_square;

	return 2;
}

void tree_sitter_cooklang_external_scanner_deserialize(
    void *payload, const char *buffer, unsigned length)
{
	Scanner *scanner = payload;

	// Intentionally not de-serialised: saw_square is transient.
	scanner->saw_square = false;

	if (length < 2) {
		scanner->in_metadata = false;
		scanner->first = true;
		return;
	}

	scanner->in_metadata = (bool)buffer[0];
	scanner->first = (bool)buffer[1];
}

static bool dispatch(
    TSLexer *lexer, Scanner *scanner, const bool *valid_symbols)
{
	if ((valid_symbols[NEWLINE] || valid_symbols[EMPTY_LINE]) &&
	    scan_newline(lexer, valid_symbols)) {
		return true;
	}
	if ((valid_symbols[METADATA_START] || valid_symbols[METADATA_END] ||
		valid_symbols[COMMENT] || valid_symbols[HIDDEN_MODIFIER]) &&
	    scan_hyphen_token(lexer, scanner, valid_symbols)) {
		return true;
	}
	if (valid_symbols[METADATA_CONTENT] && scanner->in_metadata &&
	    scan_metadata_content(lexer)) {
		lexer->result_symbol = METADATA_CONTENT;
		return true;
	}
	if (valid_symbols[HEADING_START] && scan_heading(lexer)) {
		lexer->result_symbol = HEADING_START;
		return true;
	}
	if (valid_symbols[HEADING_TEXT] && scan_heading_text(lexer, scanner)) {
		lexer->result_symbol = HEADING_TEXT;
		return true;
	}
	if ((valid_symbols[TEXT] || valid_symbols[OPEN_BRACE] ||
		valid_symbols[OPEN_PAREN] || valid_symbols[CLOSE_BRACE] ||
		valid_symbols[CLOSE_BRACE_OPEN_PAREN] || valid_symbols[COLON] ||
		valid_symbols[VERT_BAR]) &&
	    scan_text(lexer, scanner, valid_symbols)) {
		return true;
	}
	if (valid_symbols[IDENTIFIER] && scan_identifier(lexer)) {
		lexer->result_symbol = IDENTIFIER;
		return true;
	}
	return ((valid_symbols[COMMENT] || valid_symbols[COMMENT_LINE]) &&
		scan_block(lexer, scanner, valid_symbols));
}

bool tree_sitter_cooklang_external_scanner_scan(
    void *payload, TSLexer *lexer, const bool *valid_symbols)
{
	Scanner *scanner = payload;
	if (dispatch(lexer, scanner, valid_symbols)) {
		if (scanner->first && lexer->result_symbol != NEWLINE)
			scanner->first = false;
		return true;
	}
	return false;
}
