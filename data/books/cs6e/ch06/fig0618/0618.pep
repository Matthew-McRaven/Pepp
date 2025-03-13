;File: fig0618.pep
;Computer Systems, Fifth edition
;Figure 6.18
;
         BR      main
;
;******* void printTri()
printTri:@STRO   msg1,d      ;printf("*\n")
         @STRO   msg2,d      ;printf("**\n")
         @STRO   msg3,d      ;printf("***\n")
         RET
msg1:    .ASCII  "*\n\0"
msg2:    .ASCII  "**\n\0"
msg3:    .ASCII  "***\n\0"
;
;******* int main()
main:    CALL    printTri    ;printTri()
         CALL    printTri    ;printTri()
         CALL    printTri    ;printTri()
         RET
