if exists('b:current_syntax')
  finish
endif

syn keyword graceKeywords class if else for in by while return end import this print println skipwhite
syn keyword graceBooleans true false skipwhite
syn keyword graceVariable var final skipwhite
syn keyword graceFunction func skipwhite
syn keyword graceOperator and or skipwhite

syn keyword graceTodo TODO FIXME NOTE NOTES XXX contained
syn match graceComment "//.*$" contains=graceTodo

syn match graceNumber "\v<\d+>"
syn match graceNumber "\v<\d+\.\d+>"
syn region graceString start='"' end='"'
syn region graceChar start="'" end="'"

hi def link graceTodo           Todo
hi def link graceComment        Comment
hi def link graceString         String
hi def link graceChar           String
hi def link graceNumber         Number
hi def link graceBooleans       Boolean
hi def link graceKeywords       Keyword
hi def link graceVariable       Type
hi def link graceFunction       Function
hi def link graceOperator       Operator

let b:current_syntax = "grace"