; cloner_1a.bf - Program that copies itself
; Author: Janosch Gräf
; Copies itself downwards memory - Always overwrites last copy of itself.
;
; Memory signature: '[JGRAEF-CLONER-1A]'
; Length:           71
;


;---- Header ------------------------------------------------------------------
 \x00 )>+<
;  |   |   |
;  |   |   `- push 4 times to get some scratch space (-1 til -4) & goto DP = 0
;  |   `- ')'+1 = '*'
;  `- 0 marker

; this is just a signature
[JGRAEF-CLONER-1A]
; second loop won't be executed since DP = 0 & *DP = 0

;---- Mainloop Begin ----------------------------------------------------------
; set DP = 1, so add 1
>[<

;---- Copy --------------------------------------------------------------------

; goto to next lower 0 and set byte after it as stack base
<[<]>$

; goto start marker
>[>]
; goto end marker
>[>]

; push until start marker
^<[^<]^

; goto start marker of clone
<[<]<[<]
; decrement following word, since this must be ')' not '*'
>-<
; fork
Y
; goto own start marker again
>[>]>[>]

;---- Mainloop End ------------------------------------------------------------
; NOTE: ']' matches with '[' in line 23
; make sure *DP != 0 (DP = 1)
>]

; end of program
  * \x00
; |   |
; |   `- 0 marker
; `- terminate program

