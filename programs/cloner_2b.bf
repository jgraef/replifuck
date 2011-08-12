; cloner_2.bf - Program that copies itself
; Author: Janosch Gräf
;
; Memory signature: '[JGRAEF*CLONER*2B]'
; Length:           121
;


;---- Header ------------------------------------------------------------------
 \x00 )>+<
;  |   |   |
;  |   |   `- push 4 times to get some scratch space (-1 til -4) & goto DP = 0
;  |   `- ')'+1 = '*'
;  `- 0 marker

; this is just a signature
[JGRAEF*CLONER*2B]
; second loop won't be executed since DP = 0 & *DP = 0

;---- Mainloop Begin ----------------------------------------------------------
#
; DP = 0, so add 1 and set it to 0 again in loop
+[[-]

;---- Copy Left ---------------------------------------------------------------
#
; go random distance to left (overwriting all passed words)
<[<,] >$

; goto end marker
[>]>[>]

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

;---- Copy Right --------------------------------------------------------------
#
; goto end marker
>[>]

; go random distance to right (overwriting all passed words)
>[>,] >$<

; goto end marker
<[<]

; push until start marker
^<[^<]^

; goto start marker of clone
>[>]>[>]
; decrement following word, since this must be ')' not '*'
>-<
; fork
Y
; goto own start marker again
<[<]<[<]

;---- Mainloop End ------------------------------------------------------------
#
; NOTE: ']' matches with '[' in line 26
; make sure *DP != 0 (DP = 1)
,]

; end of program
  * \x00
; |   |
; |   `- 0 marker
; `- terminate program

