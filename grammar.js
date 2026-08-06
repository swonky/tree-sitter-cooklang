/**
 * @file A tree-sitter grammar for cooklang
 * @author Tom Spencer
 * @license MIT License
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const INTEGER = /[1-9][0-9]*/;
const DECIMAL = /(?:0\.[0-9]+|[1-9][0-9]*\.[0-9]+)/;
const DIGIT = /[0-9]/;

const WS_HORIZ = /[\t\p{Zs}]+/u;
const TEXT_COMMENT = /[^\u000A-\u000D\u0085\u2028\u2029]+/;
const STRING =
	/[^1-9\.-\/\u000A-\u000D\u0085\u2028\u2029%}][^\u000A-\u000D\u0085\u2028\u2029%}]+/;

const DEGREE = /(?:[°º˚]|[Dd]eg(?:ree)?s?)/;
const CELSIUS = /(?:[Cc℃](?:elsius)?)/;
const FAHRENHEIT = /(?:[Ff℉](?:ahrenheit)?)/;

const NOTE = { prefix: /[>][^>]/ };
const PREFIX = { ingredient: /[@]/, cookware: '#', timer: '~' };
const MODIFIERS = { hidden: '-', optional: '?', reference: '&', new: '+', recipe: '@' };
const DELIMITERS = { fractional: '/', range: '-', alias: '|' };
const PAREN = { open: '(', close: ')', content: token(/[^\r\n)]+/) };
const BRACE = { open: '{', close: '}', content: token(/[^\r\n}]+/) };

module.exports = grammar({
	name: 'cooklang',
	supertypes: $ => [$.block, $.definition, $.number, $.modifiers],

	externals: $ => [
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
	],

	extras: $ => [],
	rules: {
		recipe: $ =>
			seq(
				optional(seq($.metadata, $._newline)),
				optional($._padding),
				optional($._body),
				repeat($.section),
			),
		metadata: $ =>
			seq($.metadata_start, field('content', $.metadata_content), $.metadata_end),
		section: $ => seq($.heading, $._padding, optional($._body)),

		heading: $ =>
			seq(
				$._heading_start,
				optional($._ws_horiz),
				optional(
					seq(
						field('name', alias($._heading_text, $.text)),
						optional($._ws_horiz),
						optional($._permissive_text),
					),
				),
				optional($.comment),
			),
		_body: $ =>
			seq(
				seq($.block, repeat(seq($._block_seperator, $.block))),
				optional($._block_seperator),
			),
		_padding: $ => repeat1(choice($._newline, $.comment_line, $.mode)),
		modifiers: $ => choice(...Object.values(MODIFIERS)),

		block: $ => choice($.note, $.step),
		note: $ =>
			repeat1(
				prec.left(
					seq(
						alias(NOTE.prefix, '>'),
						optional($._note_line),
						repeat(seq($._newline, $._note_line)),
						$._newline,
					),
				),
			),
		_note_line: $ =>
			prec.left(repeat1(choice(alias($._permissive_text, $.text), $.comment))),
		step: $ =>
			prec.left(seq($._step_line, repeat(seq($._newline, $._step_line)), $._newline)),
		_step_line: $ =>
			seq(
				$._step_content,
				repeat(choice($._step_content, $.comment)),
				// optional(WS_HORIZ),
			),
		_step_content: $ => choice($.definition, $.text, $.temperature, $._ws_horiz),
		_ws_horiz: $ => token(WS_HORIZ),
		_block_seperator: $ =>
			choice(
				seq($._empty_line, repeat($._newline)),
				seq($.comment_line, repeat($._newline)),
				seq($.mode, repeat($._newline)),
			),
		definition: $ => choice($.ingredient, $.cookware, $.timer),
		ingredient: $ =>
			seq($._ingredient_prefix, repeat($.modifiers), $._ingredient_attributes),
		_ingredient_prefix: $ => token(PREFIX.ingredient),
		_ingredient_attributes: $ =>
			seq(
				field('name', $.identifier),
				optional($._alias),
				optional(
					seq(
						alias($._open_brace, BRACE.open),
						optional($._amount_inner),
						choice(
							alias($._close_brace, BRACE.close),
							seq(
								alias($._close_brace_open_paren, '}('),
								field('preparation', alias(PAREN.content, $.string)),
								PAREN.close,
							),
						),
					),
				),
			),
		cookware: $ =>
			seq(
				$._cookware_prefix,
				field('name', $.identifier),
				optional($._alias),
				optional($._amount_standalone),
			),
		_cookware_prefix: $ => token(PREFIX.cookware),
		_alias: $ => seq(DELIMITERS.alias, field('alias', $.identifier)),
		timer: $ =>
			seq($._timer_prefix, optional(field('name', $.identifier)), $._amount_standalone),
		_timer_prefix: $ => token(PREFIX.timer),

		_amount_standalone: $ =>
			seq(
				alias($._open_brace, BRACE.open),
				optional($._amount_inner),
				alias($._close_brace, BRACE.close),
			),
		_amount_inner: $ => prec(10, choice($._numeric_amount, $._string_amount, WS_HORIZ)),
		_numeric_amount: $ =>
			seq(
				field(
					'quantity',
					seq(optional(WS_HORIZ), prec.left(choice($.number, $.fractional, $.range))),
				),
				optional(
					seq(
						optional(WS_HORIZ),
						optional('%'),
						optional(WS_HORIZ),
						field('unit', $.unit),
					),
				),
			),
		_string_amount: $ =>
			seq(
				field('quantity', $.string),
				optional(seq('%', optional(WS_HORIZ), field('unit', $.unit))),
			),
		unit: $ => token(seq(/[^\r\n}%\-/ ]/, optional(BRACE.content))),

		// TEMPERATURE
		temperature: $ =>
			prec.right(
				seq(
					field('quantity', $.integer),
					optional(WS_HORIZ),
					optional(DEGREE),
					optional(WS_HORIZ),
					optional(field('scale', $.scale)),
				),
			),
		scale: $ => token(choice(CELSIUS, FAHRENHEIT)),

		// STRUCTURED QUANTITY
		range: $ =>
			seq(
				field('left', $.number),
				optional(WS_HORIZ),
				DELIMITERS.range,
				optional(WS_HORIZ),
				field('right', $.number),
			),
		fractional: $ =>
			seq(
				field('left', $.number),
				optional(WS_HORIZ),
				DELIMITERS.fractional,
				optional(WS_HORIZ),
				field('right', $.number),
			),

		// MODE
		mode: $ =>
			prec.left(
				seq(
					'>>',
					choice(
						seq(
							optional($._ws_horiz),
							'[',
							optional(field('key', alias($._mode_key, $.identifier))),
							optional(
								seq(
									']',
									optional(':'),
									optional(field('value', alias($._mode_value, $.string))),
								),
							),
						),
						field('text', seq(alias($._mode_fallback, $.string))),
					),
					$._newline,
				),
			),
		_mode_key: $ => repeat1(/[^\]\r\n\u000B\u000C\u0085\u2028\u2029]+/),
		_mode_value: $ =>
			seq(
				optional($._ws_horiz),
				/[^ \:\r\n\u000B\u000C\u0085\u2028\u2029]/,
				$._permissive_text_inline,
			),
		_mode_fallback: $ =>
			seq(
				optional($._ws_horiz),
				/[^ \[\r\n\u000B\u000C\u0085\u2028\u2029]/,
				$._permissive_text_inline,
			),

		// PRIMITIVES
		number: $ => prec.left(choice($.integer, $.decimal)),
		decimal: $ => token(DECIMAL),
		integer: $ => token(INTEGER),
		string: $ => STRING,
		text: $ => $._text,
		identifier: $ => $._identifier,

		// use for text that can contain definition prefix characters
		_permissive_text: $ => prec.right(repeat1(choice($._text, '@', '#', '~'))),
		_permissive_text_inline: $ =>
			seq(
				alias($._permissive_text, $.text),
				repeat(choice(alias($._permissive_text, $.text), $.comment)),
			),
	},
});
