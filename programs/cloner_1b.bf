; cloner_1b.bf - Program that copies itself
; Author: Janosch Gräf
; Copies itself downwards memory - It repeats this - with overwriting old
; clones - until a random value is 0.
; NOTE: This is a minor changed version of Cloner 1A
;
; Memory signature: '[JGRAEF-CLONER-1B]'
; Length:           73
;


;---- Header ------------------------------------------------------------------
 \x00 )>+<
;  |   |   |
;  |   |   `- push 4 times to get some scratch space (-1 til -4) & goto DP = 0
;  |   `- ')'+1 = '*'
;  `- 0 marker

; this is just a signature
[JGRAEF-CLONER-1B]
; second loop won't be executed since DP = 0 & *DP = 0

;---- Mainloop Begin ----------------------------------------------------------
; reset [0] = 0 (since after iteration of mainloop this is a random value)
; set DP = 1, so add 1
+[[-]

;---- Copy --------------------------------------------------------------------

; goto to next lower 0 and set byte after it as stack base
; NOTE: At this code position DP differs between 0 and -1 depending on whether
;       it is executed the first time or not respectively
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
; NOTE: ']' matches with '[' in line 26
; make sure *DP != 0 (DP = 1)
,]

; end of program
  * \x00
; |   |
; |   `- 0 marker
; `- terminate program

