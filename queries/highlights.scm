(comment) @nospell @comment
(comment_line) @nospell @comment

[
	"{" "}"
	"}(" 
	"(" ")"
	"[" "]"
] @punctuation.bracket

["%" "|" ":"] @punctuation.delimiter

(temperature) @number
(unit) @constant

((ingredient) @markup.link
	(#set! priority 90))
(ingredient	
  ["@" "?" "+" "&" "-"] @keyword.modifier)  

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

[(integer)
 (fractional 
	 "/" @punctuation.delimiter)
 ]@number

(range 
	"-" @punctuation.delimiter)

(decimal) @number.float
(string) @nospell @string 
(text) @spell

(heading) @markup.heading

(note
	">" @punctuation.special
) @markup.quote

(mode
	">>" @keyword.directive
) @macro.constant

(mode
	key: (identifier) @keyword.directive
)

([
	(metadata_start)
	(metadata_end)
] @keyword.directive
	(#set! priority 90))

(ERROR) @error-node

