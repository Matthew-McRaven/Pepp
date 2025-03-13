;File: fig0640.pep
;Computer Systems, Fifth edition
;Figure 6.40
;
         BR      main
;
;******* main()
guess:   .EQUATE 0           ;local variable #2d
main:    SUBSP   2,i         ;push #guess
         @STRO   msgIn,d     ;printf("Pick a number 0..3: ")
         @DECI   guess,s     ;scanf("%d", &guess)
         LDWX    guess,s     ;switch (guess)
         ASLX                ;two bytes per address
         BR      guessJT,x
guessJT: .WORD case0
         .WORD case1
         .WORD case2
         .WORD case3
case0:   @STRO   msg0,d      ;printf("Not close\n")
         BR      endCase     ;break
case1:   @STRO   msg1,d      ;printf("Close\n")
         BR      endCase     ;break
case2:   @STRO   msg2,d      ;printf("Right on\n")
         BR      endCase     ;break
case3:   @STRO   msg3,d      ;printf("Too high\n")
endCase: ADDSP   2,i         ;pop #guess
         RET
msgIn:   .ASCII  "Pick a number 0..3: \0"
msg0:    .ASCII  "Not close\n\0"
msg1:    .ASCII  "Close\n\0"
msg2:    .ASCII  "Right on\n\0"
msg3:    .ASCII  "Too high\n\0"
