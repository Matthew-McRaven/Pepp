;File: fig0610.pep
;Computer Systems, Fifth edition
;Figure 6.10
;
         BR      main        
letter:  .BLOCK  1           ;global variable #1c
;
main:    @CHARI  letter,d    ;scanf("%c", &letter)
while:   LDBA    letter,d    ;while (letter != '*')
         CPBA    '*',i       
         BREQ    endWh       
if:      CPBA    ' ',i       ;if (letter == ' ')
         BRNE    else        
         @CHARO  '\n',i      ;printf("\n")
         BR      endIf       
else:    @CHARO  letter,d    ;printf("%c", letter)
endIf:   @CHARI  letter,d    ;scanf("%c", &letter)
         BR      while       
endWh:   RET                
         .END                  
