/**
 * @file A tree-sitter grammar for cooklang
 * @author Tom Spencer
 * @license MIT License
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const WS_VERT = String.raw`\u000A-\u000D\u0085\u2028\u2029`;
// const WS_HORIZ = /[\t\p{Zs}]+/u;

const escapeChar = (c) =>
	`\\x${c.codePointAt(0).toString(16).padStart(2, "0")}`;

const charClassExclude = (...chars) =>
	String.raw`[^${chars
		.map((c) => `\\x${c.codePointAt(0).toString(16).padStart(2, "0")}`)
		.join("")}${WS_VERT}]`;

const charExclude = (...chars) => new RegExp(charClassExclude(...chars));

const strExclude = (...chars) => new RegExp(`${charClassExclude(...chars)}+`);

const DEGREE = /(?:[°º˚]|[Dd]eg(?:ree)?s?)/;
const CELSIUS = /(?:[Cc℃](?:elsius)?)/;
const FAHRENHEIT = /(?:[Ff℉](?:ahrenheit)?)/;

const SQUARE = {
	open: "[",
	close: "]",
	content: strExclude("]"),
};

const METADATA_KEY = token(seq(charExclude(" ", "["), strExclude(":")));

const REF_MODS = { relative: "~", section: "=" };

module.exports = grammar({
	name: "cooklang",
	supertypes: ($) => [$.block, $.definition, $.number, $.modifiers],

	externals: ($) => [
		$._unspecified, // never requested
		// frontmatter
		$._fmat_prefix,
		$.frontmatter,
		$._fmat_suffix,
		// plain text content
		$._text,
		$._identifier,
		$._newline,
		$._empty_line,
		$.comment_line,
		$.comment,
		// headings
		// $._heading_start,
		// $._heading_text,
		// symbolic operators
		$._ext_sym_paren_open,
		$._ext_sym_paren_close,
		$._ext_sym_brace_open,
		$._ext_sym_brace_close,
		$._ext_sym_colon,
		$._ext_sym_vbar,
		$._ext_sym_percent,
		$._ext_sym_solidus,
		$._ext_sym_hyphen,
		$._ext_sym_commat,
		$._ext_sym_num,
		$._ext_sym_tilde,
		$._ext_sym_ampersand,
		$._ext_sym_plus,
		$._ext_sym_question,
		$._ext_sym_equal,
		$._ext_sym_gt,
		// composite operators
		$._close_brace_open_paren,
		$._ext_sym_gt_gt,
		// numbers
		$._ext_lit_integer,
		$._ext_lit_decimal,
		// whitespace
		$._ws,
	],

	// must be defined empty to override default whitespace handling.
	extras: ($) => [],
	rules: {
		recipe: ($) =>
			seq(
				optional(seq($._frontmatter, $._newline)),
				optional($._padding),
				optional($._body),
				repeat($.section),
			),
		_frontmatter: ($) =>
			seq($._sym_fmat_open, $.frontmatter, $._sym_fmat_close),

		// SECTIONS
		section: ($) => seq($.heading, $._padding, optional($._body)),

		heading: ($) =>
			seq(
				$._heading_start,
				optional($._ws),
				optional(
					seq(
						field("name", $.text),
						optional($.text),
						// optional($._ws),
						// optional($._permissive_text),
					),
				),
				optional($.comment),
			),
		_heading_start: ($) => repeat1($._sym_equal),
		_body: ($) =>
			seq(
				seq($.block, repeat(seq($._block_seperator, $.block))),
				optional($._block_seperator),
			),

		_padding: ($) => repeat1(choice($._newline, $.comment_line, $.directive)),
		_interblock_lines: ($) => choice($.comment_line, $.directive),
		_block_seperator: ($) =>
			prec.left(
				seq(
					choice(
						seq(choice($._interblock_lines, $._empty_line), repeat($._newline)),
					),
					repeat(seq($._interblock_lines, repeat($._newline))),
				),
			),

		block: ($) => choice($.note, $.step),

		// NOTE
		note: ($) => prec.left(repeat1($._note_line)),

		_note_line: ($) =>
			seq(
				$._sym_gt,
				optional($._newline),
				repeat1(seq($._inline_text, $._newline)),
			),
		_inline_text: ($) => repeat1(choice($.text, $.comment)),

		// STEP
		step: ($) =>
			prec.left(
				seq($._step_line, repeat(seq($._newline, $._step_line)), $._newline),
			),
		_step_line: ($) =>
			seq($._step_content, repeat(choice($._step_content, $.comment))),

		_step_content: ($) => choice($.definition, $.text, $.temperature), // $._ws),

		definition: ($) => choice($.ingredient, $.cookware, $.timer),

		// INGREDIENT
		ingredient: ($) =>
			seq(
				$._sym_commat,
				repeat($.modifiers),
				$._ingredient_prefix,
				optional($._alias),
				optional($._ingredient_attr),
			),

		_ingredient_prefix: ($) =>
			choice(
				seq(
					$._sym_ampersand,
					choice(
						seq($._reference, repeat($.modifiers), field("name", $.identifier)),
						seq(repeat($.modifiers), field("target", $.identifier)),
					),
				),
				field("name", $.identifier),
			),

		_ingredient_attr: ($) =>
			seq(
				$._sym_brace_open,
				optional($._typed_amount),
				choice(
					$._sym_brace_close,
					seq(
						alias($._close_brace_open_paren, "}("),
						optional(field("preparation", $._string)),
						$._sym_paren_close,
					),
				),
			),

		// TIMER
		timer: ($) =>
			seq(
				$._sym_tilde,
				optional(field("name", $.identifier)),
				$._amount_standalone,
			),

		// COOKWARE
		cookware: ($) =>
			seq(
				$._sym_num,
				field("name", $.identifier),
				optional($._alias),
				optional($._amount_standalone),
			),

		// INGREDIENT AMOUNTS
		_amount_standalone: ($) =>
			seq($._sym_brace_open, optional($._typed_amount), $._sym_brace_close),

		_typed_amount: ($) =>
			seq(
				field("quantity", $._quantity),
				optional(choice($._ws, $._sym_percent)),
				optional(field("unit", $._string)),
			),

		// QUANTITY
		_literal: ($) => choice($.number, $._string),
		_structured: ($) => choice($.fractional, $.range),
		_quantity: ($) => choice($._literal, $._structured),

		fractional: ($) =>
			prec.right(
				seq(optional($._expr_left), $._sym_solidus, optional($._expr_right)),
			),
		range: ($) =>
			prec.right(
				seq(optional($._expr_left), $._sym_hyphen, optional($._expr_right)),
			),

		_expr_left: ($) => field("left", $._literal),
		_expr_right: ($) => field("right", $._literal),

		// TEMPERATURE
		temperature: ($) =>
			prec.right(
				seq(
					field("quantity", $.integer),
					optional($._ws),
					optional(DEGREE),
					optional($._ws),
					optional(field("scale", $.scale)),
				),
			),
		scale: ($) => token(choice(CELSIUS, FAHRENHEIT)),

		// DIRECTIVE (METADATA/MODE)
		directive: ($) =>
			prec.left(
				seq(
					$._sym_gt_gt,
					optional($._ws),
					choice($.metadata, $.mode),
					$._newline,
				),
			),

		_directive_value: ($) => repeat1(choice($._field_value, $.comment)),
		_field_value: ($) => field("value", choice($.number, $._string)),

		metadata: ($) =>
			prec.right(
				seq(
					field("key", alias(METADATA_KEY, $.identifier)),
					optional(
						seq($._sym_colon, optional($._ws), optional($._directive_value)),
					),
				),
			),

		mode: ($) =>
			prec.right(
				seq(
					SQUARE.open,
					optional(field("key", alias(SQUARE.content, $.identifier))),
					optional(
						seq(
							SQUARE.close,
							optional($._sym_colon),
							optional($._ws),
							optional($._directive_value),
						),
					),
				),
			),

		// MODIFIERS
		modifiers: ($) =>
			choice($._sym_commat, $._sym_plus, $._sym_hyphen, $._sym_question),

		// INTERMEDIATE PREPARATIONS
		_reference: ($) =>
			seq(
				$._sym_paren_open,
				optional($._ws),
				optional(choice($.step_reference, $.section_reference)),
				$._sym_paren_close,
			),

		step_reference: ($) =>
			seq(choice($.relative_reference, $.absolute_reference)),
		section_reference: ($) =>
			seq(
				$._sym_equal,
				// optional($._ws),
				choice($.relative_reference, $.absolute_reference),
			),
		relative_reference: ($) =>
			seq(
				$._sym_tilde,
				// optional($._ws),
				field("target", $.integer),
				// optional($._ws),
			),
		absolute_reference: ($) => seq(field("target", $.integer), optional($._ws)),

		// COMPONENT ALIAS
		_alias: ($) => seq($._sym_vbar, field("alias", $.identifier)),

		// PRIMITIVE
		number: ($) => choice($.integer, $.decimal),
		decimal: ($) => $._ext_lit_decimal,
		integer: ($) => $._ext_lit_integer,
		_string: ($) => alias($._text, $.string),
		text: ($) => $._text,
		identifier: ($) => $._identifier,

		// SYMBOLIC
		_sym_colon: ($) => alias($._ext_sym_colon, ":"),
		_sym_percent: ($) => alias($._ext_sym_percent, "%"),
		_sym_brace_open: ($) => alias($._ext_sym_brace_open, "{"),
		_sym_brace_close: ($) => alias($._ext_sym_brace_close, "}"),
		_sym_paren_open: ($) => alias($._ext_sym_paren_open, "("),
		_sym_paren_close: ($) => alias($._ext_sym_paren_close, ")"),
		_sym_vbar: ($) => alias($._ext_sym_vbar, "|"),
		_sym_solidus: ($) => alias($._ext_sym_solidus, "/"),
		_sym_hyphen: ($) => alias($._ext_sym_hyphen, "-"),
		_sym_fmat_open: ($) => alias($._fmat_prefix, "---"),
		_sym_fmat_close: ($) => alias($._fmat_suffix, "---"),

		_sym_tilde: ($) => alias($._ext_sym_tilde, "~"),
		_sym_num: ($) => alias($._ext_sym_num, "#"),
		_sym_commat: ($) => alias($._ext_sym_commat, "@"),
		_sym_ampersand: ($) => alias($._ext_sym_ampersand, "&"),
		_sym_plus: ($) => alias($._ext_sym_plus, "+"),
		_sym_question: ($) => alias($._ext_sym_question, "?"),

		_sym_equal: ($) => alias($._ext_sym_equal, "="),
		_sym_gt: ($) => alias($._ext_sym_gt, ">"),
		_sym_gt_gt: ($) => alias(seq($._sym_gt, $._sym_gt), ">>"),

		// _ws: ($) => token(WS_HORIZ),
		_permissive_text: ($) =>
			prec.right(
				repeat1(choice($._text, $._sym_tilde, $._sym_num, $._sym_commat)),
			),
		_permissive_numeric_text: ($) =>
			prec.left(
				choice($._permissive_text, seq(/[1-9]+/, optional($._permissive_text))),
			),
		_permissive_text_inline: ($) =>
			seq(
				repeat1(
					seq(
						choice(alias($._permissive_numeric_text, $.string), $.comment),
						optional($._ws),
					),
				),
			),
	},
});
