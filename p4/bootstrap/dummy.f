: IF ' 0BRANCH , HERE@ 0 , ; [LATEST] IMMEDIATE

: THEN
    DUP
    HERE@ SWAP -
    SWAP !
; [LATEST] IMMEDIATE
: TEST IF 3 . THEN ;
