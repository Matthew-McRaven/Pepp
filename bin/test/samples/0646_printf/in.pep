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
main:    LDWX    first,i     ;scanf("%c%c%d %s", &bill.first,
         @CHARI  bill,x
         LDWX    last,i      ;&bill.last,
         @CHARI  bill,x
         LDWX    age,i       ;&bill.age,
         @DECI   bill,x
         LDWX    gender,i    ;&bill.gender)
         @CHARI  bill,x
         LDWX    first,i
         LDBA    bill,x
         STBA    -6,s
         LDWX    last,i
         LDBA    bill,x
         STBA    -5,s
         LDWX    age,i
         LDWA    bill,x
         STWA    -4, s
         LDWX    gender,i
         LDBA    bill,x
         CPBA    'm',i
         BRNE    else
         LDWA    mMale,i
         BR      print
else:    LDWA    mFemale,i
print:   STWA    -2,s
         LDWA    msg,i
         STWA    -8,s
         SUBSP   8,i
         CALL    printf
         ADDSP   8,i
endIf:   RET
mFemale: .ASCII  "fe"
mMale:   .ASCII  "male\x00"
msg:     .ASCII  "Initials: %c%c\nAge: %d\nGender: %s\n\x00"
         @LIBC
