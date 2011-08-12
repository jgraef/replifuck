; cloner_2.bf - Program that copies itself
; Author: Janosch Gräf
; Copies itself downwards memory and upwards memory. Tries to not overwrite
; other clones. Cloning is done until a random value becomes 0. This cloner
; also destroys all code it encounters on its way to a copy destination.
; NOTE: This is the sucessor of Cloner 1A and 1B
; NOTE: Since this cloner uses poor propablities to avoid being overwritten by
;       a clone of itself, this is very copetitive.
; TODO: Could be a better idea to clone to right side first, since this takes
;       longer
; TODO: I could put a generation word into the signature, that is incremented
;       each time a new program spawns.
;
;
; Memory signature: '[JGRAEF-CLONER-2]'
; Length:           121
;


;---- Header ------------------------------------------------------------------
 \x00 )>+<
;  |   |   |
;  |   |   `- push 4 times to get some scratch space (-1 til -4) & goto DP = 0
;  |   `- ')'+1 = '*'
;  `- 0 marker

; this is just a signature
[JGRAEF-CLONER-2]
; second loop won't be executed since DP = 0 & *DP = 0

;---- Mainloop Begin ----------------------------------------------------------
; DP = 0, so add 1 and set it to 0 again in loop
+[[-]

;---- Copy Left ---------------------------------------------------------------

; goto second next lower 1, set it to 0 and set stack base to word after it
<[<-] >$

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

; goto end marker
>[>]

; goto second next higher 1, set it to 0 and set stack base to word after it
; we choose the second next, to be sure (not really sure) to be distant enough, since
; stack is pushed in our direction. but we shouldn't take too much time, since our
; left clone will copy to its right soon.
>[>-]+ >[>-] >$<

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
; NOTE: ']' matches with '[' in line 26
; make sure *DP != 0 (DP = 1)
,]

; end of program
  * \x00
; |   |
; |   `- 0 marker
; `- terminate program

