/**
 * @file A tree-sitter grammar for cooklang
 * @author Tom Spencer
 * @license MIT License
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const WS_VERT = String.raw`\u000A-\u000D\u0085\u2028\u2029`;
const WS_HORIZ = /[\t\p{Zs}]+/u;

const escapeChar = (c) =>
	`\\x${c.codePointAt(0).toString(16).padStart(2, "0")}`;

const charClassExclude = (...chars) =>
	String.raw`[^${chars
		.map((c) => `\\x${c.codePointAt(0).toString(16).padStart(2, "0")}`)
		.join("")}${WS_VERT}]`;

const charExclude = (...chars) => new RegExp(charClassExclude(...chars));

const strExclude = (...chars) => new RegExp(`${charClassExclude(...chars)}+`);

const ws_suffix = (token) => seq(token, optional(WS_HORIZ));

const INTEGER = /[1-9][0-9]*/;
const DECIMAL = /(?:0\.[0-9]+|[1-9][0-9]*\.[0-9]+)/;
const DIGIT = /[0-9]/;

const TEXT_COMMENT = new RegExp(`[^${WS_VERT}]+`);
const STRING = new RegExp(`[^1-9.\\-/${WS_VERT}%}][^${WS_VERT}%}]+`);

const DEGREE = /(?:[°º˚]|[Dd]eg(?:ree)?s?)/;
const CELSIUS = /(?:[Cc℃](?:elsius)?)/;
const FAHRENHEIT = /(?:[Ff℉](?:ahrenheit)?)/;

const NOTE = { prefix: /[>][^>]/ };

const PREFIX = {
	ingredient: "@",
	cookware: "#",
	timer: "~",
	directive: ">>",
	note: ">",
};
const MODIFIERS = {
	hidden: "-",
	optional: "?",
	reference: "&",
	new: "+",
	recipe: "@",
};
const DELIMITERS = {
	amount: "%",
	fractional: "/",
	range: "-",
	alias: "|",
	directive: ":",
};
const PAREN = { open: "(", close: ")", content: strExclude(")") };
const BRACE = { open: "{", close: "}", content: strExclude("}") };
const SQUARE = {
	open: "[",
	close: "]",
	content: strExclude("]"),
};

const METADATA_KEY = token(seq(charExclude(" ", "["), strExclude(":")));

const REF_MODS = { relative: "~", section: "=" };

const UNIT = /[^\r\n}%\-/ ]/;

module.exports = grammar({
	name: "cooklang",
	supertypes: ($) => [$.block, $.definition, $.number, $.modifiers],

	externals: ($) => [
		$.metadata_start,
		$.metadata_content,
		$.metadata_end,
		$._text,
		$._identifier,
		$._hidden_modifier,
		$._newline,
		$._empty_line,
		$.comment_line,
		$.comment,
		$._heading_start,
		$._heading_text,
		$._open_paren,
		$._open_brace,
		$._close_brace,
		$._close_brace_open_paren,
		$._colon,
		$._vert_bar,
	],

	// must be defined empty to override default whitespace handling.
	extras: ($) => [],
	rules: {
		recipe: ($) =>
			seq(
				optional(seq($.frontmatter, $._newline)),
				optional($._padding),
				optional($._body),
				repeat($.section),
			),
		frontmatter: ($) =>
			seq(
				$.metadata_start,
				field("content", $.metadata_content),
				$.metadata_end,
			),

		// SECTIONS
		section: ($) => seq($.heading, $._padding, optional($._body)),

		heading: ($) =>
			seq(
				$._heading_start,
				optional($._ws_horiz),
				optional(
					seq(
						field("name", alias($._heading_text, $.text)),
						optional($._ws_horiz),
						optional($._permissive_text),
					),
				),
				optional($.comment),
			),
		_body: ($) =>
			seq(
				seq($.block, repeat(seq($._block_seperator, $.block))),
				optional($._block_seperator),
			),

		_padding: ($) => repeat1(choice($._newline, $.comment_line, $.directive)),
		_interblock_lines: ($) => choice($.comment_line, $.directive),
		_block_seperator: ($) =>
			seq(
				choice(
					seq(choice($._interblock_lines, $._empty_line), repeat($._newline)),
				),
				repeat(seq($._interblock_lines, repeat($._newline))),
			),

		block: ($) => choice($.note, $.step),

		// NOTE
		note: ($) =>
			repeat1(
				prec.left(
					seq(
						alias(NOTE.prefix, PREFIX.note),
						optional($._note_line),
						repeat(seq($._newline, $._note_line)),
						$._newline,
					),
				),
			),
		_note_line: ($) =>
			prec.left(repeat1(choice(alias($._permissive_text, $.text), $.comment))),

		// STEP
		step: ($) =>
			prec.left(
				seq($._step_line, repeat(seq($._newline, $._step_line)), $._newline),
			),
		_step_line: ($) =>
			seq($._step_content, repeat(choice($._step_content, $.comment))),
		_step_content: ($) =>
			choice($.definition, $.text, $.temperature, $._ws_horiz),

		definition: ($) => choice($.ingredient, $.cookware, $.timer),

		// INGREDIENT
		ingredient: ($) =>
			seq(/[@]/, repeat($.modifiers), $._ingredient_attributes),

		_ingredient_attributes: ($) =>
			seq(
				choice(
					seq(
						MODIFIERS.reference,
						choice(
							seq(
								$._reference,
								repeat($.modifiers),
								field("name", $.identifier),
							),
							seq(repeat($.modifiers), field("target", $.identifier)),
						),
					),
					field("name", $.identifier),
				),
				optional($._alias),
				optional(
					seq(
						alias($._open_brace, BRACE.open),
						optional($._amount_inner),
						choice(
							alias($._close_brace, BRACE.close),
							seq(
								alias($._close_brace_open_paren, "}("),
								field("preparation", alias(PAREN.content, $.string)),
								PAREN.close,
							),
						),
					),
				),
			),

		// TIMER
		timer: ($) =>
			seq(
				$._timer_prefix,
				optional(field("name", $.identifier)),
				$._amount_standalone,
			),
		_timer_prefix: ($) => token(PREFIX.timer),

		// COOKWARE
		cookware: ($) =>
			seq(
				$._cookware_prefix,
				field("name", $.identifier),
				optional($._alias),
				optional($._amount_standalone),
			),
		_cookware_prefix: ($) => token(PREFIX.cookware),

		// INGREDIENT AMOUNTS
		_amount_standalone: ($) =>
			seq(
				alias($._open_brace, BRACE.open),
				optional($._amount_inner),
				alias($._close_brace, BRACE.close),
			),
		_amount_inner: ($) =>
			prec(10, choice($._numeric_amount, $._string_amount, $._ws_horiz)),
		_numeric_amount: ($) =>
			seq(
				field(
					"quantity",
					seq(
						optional($._ws_horiz),
						prec.left(choice($.number, $.fractional, $.range)),
					),
				),
				optional(
					seq(
						optional($._ws_horiz),
						optional(DELIMITERS.amount),
						optional($._ws_horiz),
						field("unit", $.unit),
					),
				),
			),
		_string_amount: ($) =>
			seq(
				field("quantity", $.string),
				optional(
					seq(DELIMITERS.amount, optional($._ws_horiz), field("unit", $.unit)),
				),
			),
		unit: ($) => token(seq(UNIT, optional(BRACE.content))),

		// TEMPERATURE
		temperature: ($) =>
			prec.right(
				seq(
					field("quantity", $.integer),
					optional($._ws_horiz),
					optional(DEGREE),
					optional($._ws_horiz),
					optional(field("scale", $.scale)),
				),
			),
		scale: ($) => token(choice(CELSIUS, FAHRENHEIT)),

		// STRUCTURED QUANTITY
		range: ($) =>
			seq(
				field("left", $.number),
				optional($._ws_horiz),
				DELIMITERS.range,
				optional($._ws_horiz),
				field("right", $.number),
			),
		fractional: ($) =>
			seq(
				field("left", $.number),
				optional($._ws_horiz),
				DELIMITERS.fractional,
				optional($._ws_horiz),
				field("right", $.number),
			),

		// DIRECTIVE (METADATA/MODE)
		directive: ($) =>
			prec.left(
				seq(
					PREFIX.directive,
					optional($._ws_horiz),
					choice($.metadata, $.mode),
					$._newline,
				),
			),

		_directive_delimiter: ($) => alias($._colon, DELIMITERS.directive),
		_directive_value: ($) => field("value", $._permissive_text_inline),

		// METADATA DIRECTIVE
		metadata: ($) =>
			prec.right(
				seq(
					field("key", alias(METADATA_KEY, $.identifier)),
					optional(
						seq(
							$._directive_delimiter,
							optional($._ws_horiz),
							optional($._directive_value),
						),
					),
				),
			),

		// MODE DIRECTIVE
		mode: ($) =>
			prec.right(
				seq(
					SQUARE.open,
					optional(field("key", alias(SQUARE.content, $.identifier))),
					optional(
						seq(
							SQUARE.close,
							optional($._directive_delimiter),
							optional($._ws_horiz),
							optional($._directive_value),
						),
					),
				),
			),

		// MODIFIERS
		modifiers: ($) =>
			choice(
				MODIFIERS.recipe,
				MODIFIERS.new,
				MODIFIERS.optional,
				MODIFIERS.hidden,
			),

		// INTERMEDIATE PREPARATIONS
		_reference: ($) =>
			seq(
				alias($._open_paren, PAREN.open),
				optional($._ws_horiz),
				optional(choice($.step_reference, $.section_reference)),
				PAREN.close,
			),

		step_reference: ($) =>
			seq(choice($.relative_reference, $.absolute_reference)),
		section_reference: ($) =>
			seq(
				REF_MODS.section,
				optional($._ws_horiz),
				choice($.relative_reference, $.absolute_reference),
			),
		relative_reference: ($) =>
			seq(
				REF_MODS.relative,
				optional($._ws_horiz),
				field("target", $.integer),
				optional($._ws_horiz),
			),
		absolute_reference: ($) =>
			seq(field("target", $.integer), optional($._ws_horiz)),

		// COMPONENT ALIAS
		_alias: ($) =>
			seq(alias($._vert_bar, DELIMITERS.alias), field("alias", $.identifier)),

		// PRIMITIVES
		number: ($) => prec.left(choice($.integer, $.decimal)),
		decimal: ($) => token(DECIMAL),
		integer: ($) => token(INTEGER),
		string: ($) => STRING,
		text: ($) => $._text,
		identifier: ($) => $._identifier,

		_ws_horiz: ($) => token(WS_HORIZ),
		_permissive_text: ($) =>
			prec.right(
				repeat1(
					choice($._text, PREFIX.ingredient, PREFIX.cookware, PREFIX.timer),
				),
			),
		_permissive_numeric_text: ($) =>
			alias(
				prec.left(
					choice(
						$._permissive_text,
						seq(/[1-9]+/, optional($._permissive_text)),
					),
				),
				$.string,
			),
		_permissive_text_inline: ($) =>
			seq(
				repeat1(
					seq(
						choice($._permissive_numeric_text, $.comment),
						optional($._ws_horiz),
					),
				),
			),
	},
});
