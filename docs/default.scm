([
	(metadata_start)
	(metadata_end)
] @metadata
	(#set! priority 90))

(directive ">>" @punctuation)
(mode key: (identifier) @mode)
(metadata key: (identifier) @metadata)

((ingredient) @ingredient (#set! priority 90))

(ingredient 
	["@" "?" "+" "&" "-"] @modifier
)

(cookware) @cookware

(timer) @timer
(timer
	unit: (unit) @property
	(#any-of? @property
		"s" "h" "min" "d"
		"second" "seconds"
		"minute" "minutes"
		"hour" "hours"
		"day" "days"))

(heading) @section
(note) @note

(comment) @comment
(comment_line) @comment

(unit) @constant
(temperature) @number
(integer) @number
(decimal) @number
(string) @property 

["{" "}" "}(" "(" ")" "[" "]"] @punctuation
["%" "|" ":" "/" "-"] @punctuation
[">"] @punctuation
