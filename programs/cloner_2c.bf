; cloner_2c.bf - Program that copies itself
; Author: Janosch Gräf
;
; Length:           75
;


;---- Header ------------------------------------------------------------------
 \x00 )>+
;  |   |
;  |   |
;  |   `- ')'+1 = '*'
;  `- 0 marker

; second loop won't be executed since DP = 0 & *DP = 0


;---- Mainloop Begin ----------------------------------------------------------
; DP = SOP+1
[

;---- Copy Right ---------------------------------------------------------------
; goto EOP
[>]

; go random distance to right (overwriting all passed words)
>[>,]
;;>[>]

; pop code
V>V[>V]

; goto SOP
<[<]

; decrement following word, since this must be ')' not '*'; and fork
>-< Y

; goto SOP and reset stackbase
<[<] <[<] $


;---- Copy Left --------------------------------------------------------------
; go random distance to left (overwriting all passed words)
<[<,]
;;<[<]

; pop code
V>V[>V]

; goto SOP
<[<]

; decrement following word, since this must be ')' not '*'; and fork
>-< Y

; goto EOP reset stackbase
>[>] >[>] $

;---- Mainloop End ------------------------------------------------------------
; NOTE: ']' matches with '[' in line 24
; set DP to SOP+1
>]

;---- Footer ------------------------------------------------------------------
; end of program
;
  * JG\x01\x2C \x00
; |     |      |
; |     |      `- 0 marker
; |     `- signature
; `- terminate program
;
; signature:
; JG: initials of author
; \x01: program ID (0x01=cloner)
; \x2C: version (0xF0=major; 0x0F=minor)



