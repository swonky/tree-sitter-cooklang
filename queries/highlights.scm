(note
	">" @punctuation.special
	@string
)

(comment) @comment @nospell
(comment_line) @comment @nospell @markup.italic

[
	"{"
	"}"
	"}("
	")"
] @punctuation.bracket

["%" "|"] @punctuation.delimiter

(temperature) @number
(unit) @constant

(ingredient) @markup.link
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

; ((unit) @constant.builtin
; 	(#any-of? @constant.builtin
; 		"s" "h" "min" "d"
; 		"second" "seconds"
; 		"minute" "minutes"
; 		"hour" "hours"
; 		"day" "days"))

[(integer)
 (fractional 
	 "/" @punctuation.delimiter)
 ]@number

(range 
	"-" @punctuation.delimiter)

(decimal) @number.float
(string) @string @nospell
(text) @spell

(heading) @markup.heading

(note 
	(text) @markup.quote
)

([
	(metadata_start)
	(metadata_end)
] @keyword.directive
	(#set! priority 90))

(ERROR) @error-node

