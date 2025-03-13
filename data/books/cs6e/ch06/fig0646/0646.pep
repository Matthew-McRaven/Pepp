;File: fig0646.pep
;Computer Systems, Fifth edition
;Figure 6.46
;
         BR      main
first:   .EQUATE 0           ;struct field #1c
last:    .EQUATE 1           ;struct field #1c
age:     .EQUATE 2           ;struct field #2d
gender:  .EQUATE 4           ;struct field #1c
bill:    .BLOCK  5           ;globals #first #last #age #gender
;
;******* main()
main:    LDWX    first,i     ;scanf("%c%c%d %c", &bill.first,
         @CHARI  bill,x
         LDWX    last,i      ;&bill.last,
         @CHARI  bill,x
         LDWX    age,i       ;&bill.age,
         @DECI   bill,x
         LDWX    gender,i    ;&bill.gender)
         @CHARI  bill,x
         @STRO   msg0,d      ;printf("Initials: %c%c\n",
         LDWX    first,i     ;bill.first,
         @CHARO  bill,x
         LDWX    last,i      ;bill.last)
         @CHARO  bill,x
         @CHARO  '\n',i
         @STRO   msg1,d      ;printf("Age:  %d\n",
         LDWX    age,i       ;bill.age)
         @DECO   bill,x
         @CHARO  '\n',i
         @STRO   msg2,d      ;printf("Gender: ")
         LDWX    gender,i    ;if (bill.gender == 'm')
         LDBA    bill,x
         CPBA    'm',i
         BRNE    else
         @STRO   msg3,d      ;printf("male\n")
         BR      endIf
else:    @STRO   msg4,d      ;printf("female\n")
endIf:   RET
msg0:    .ASCII  "Initials: \0"
msg1:    .ASCII  "Age: \0"
msg2:    .ASCII  "Gender: \0"
msg3:    .ASCII  "male\n\0"
msg4:    .ASCII  "female\n\0"
