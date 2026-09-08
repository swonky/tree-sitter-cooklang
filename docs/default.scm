([
	(metadata_start)
	(metadata_end)
] @metadata
	(#set! priority 90))

(mode key: (identifier) @property)
(metadata key: (identifier) @property)

((ingredient) @ingredient (#set! priority 90))
(ingredient unit: (_) @ingredient)
(ingredient preparation: (_) @ingredient)

(ingredient 
	["@" "?" "+" "&" "-"] @section
)

(cookware) @cookware

(timer) @timer
(timer
	unit: (unit) @timer
	(#any-of? @timer
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


["[" "]"] @punctuation
[ "|" ":" "/" "-"] @section

(ingredient ["{" "}" "}(" "(" ")" "%"] @ingredient_punc)
(cookware ["#" "{" "}" "%"] @cookware_punc)
(timer ["~" "{" "}" "%"] @timer_punc)
[ ">"] @timer
(directive ">>" @ingredient)
