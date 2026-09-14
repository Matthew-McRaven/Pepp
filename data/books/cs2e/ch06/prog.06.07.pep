;Program 6.7
         BR      Main
lineFeed:.EQUATE h#000A
;
;------- void PrintTri()
PrintTri:CHARO   c#/*/,i    ;cout << "*" << endl
         CHARO   lineFeed,i
         CHARO   c#/*/,i    ;cout << "**" << endl
         CHARO   c#/*/,i
         CHARO   lineFeed,i
         CHARO   c#/*/,i    ;cout << "***" << endl
         CHARO   c#/*/,i
         CHARO   c#/*/,i
         CHARO   lineFeed,i
         CHARO   c#/*/,i    ;cout << "****" << endl
         CHARO   c#/*/,i
         CHARO   c#/*/,i
         CHARO   c#/*/,i
         CHARO   lineFeed,i
         RTS
;
;------- main()
Main:    JSR     PrintTri   ;PrintTri()
         JSR     PrintTri   ;PrintTri()
         JSR     PrintTri   ;PrintTri()
         STOP
         .END

