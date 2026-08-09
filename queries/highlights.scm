([
	(metadata_start)
	(metadata_end)
] @keyword.directive
	(#set! priority 90))

(directive
	">>" @keyword.directive.define
)

(metadata
	key: (identifier) @property
)

(mode
	key: (identifier) @keyword.directive.define
)

((ingredient) @attribute (#set! priority 90))

(ingredient 
	["@" "?" "+" "&" "-"] @keyword.modifier
)

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
[">"] @punctuation.special

(ERROR) @error-node
