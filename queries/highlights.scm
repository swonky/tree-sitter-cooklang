([
	(metadata_start)
	(metadata_end)
] @keyword.directive
	(#set! priority 90))

((ingredient) @markup.link (#set! priority 90))
(ingredient ["@" "?" "+" "&" "-"] @keyword.modifier)  

(cookware) @type

(timer) @property
(timer
	unit: (unit) @constant.builtin
	(#any-of? @constant.builtin
		"s" "h" "min" "d"
		"second" "seconds"
		"minute" "minutes"
		"hour" "hours"
		"day" "days"))

(heading) @markup.heading
(note) @markup.quote
(directive) @macro.constant

(metadata
	key: (identifier) @parameter.member
)

(mode
	key: (identifier) @keyword.directive.define
)

(comment) @nospell @comment
(comment_line) @nospell @comment

(unit) @constant
(temperature) @number
(integer) @number
(decimal) @number.float
(string) @nospell @string 
(text) @spell

["{" "}" "}(" "(" ")" "[" "]"] @punctuation.bracket
["%" "|" ":" "/" "-"] @punctuation.delimiter
[">>"] @keyword.directive
[">"] @punctuation.special

(ERROR) @error-node
