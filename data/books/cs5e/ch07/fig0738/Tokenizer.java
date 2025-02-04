package fig0738;

/**
 * The lexical analyzer implemented as a finite state machine.
 *
 * <p>
 * File: <code>Tokenizer.java</code>
 *
 * @author J. Stanley Warford
 */
public class Tokenizer {

   private final InBuffer b;

   public Tokenizer(InBuffer inBuffer) {
      b = inBuffer;
   }

   public AToken getToken() {
      char nextChar;
      StringBuffer localStringValue = new StringBuffer();
      int localIntValue = 0;
      int sign = +1;
      AToken aToken = new TEmpty();
      LexState state = LexState.LS_START;
      do {
         nextChar = b.advanceInput();
         switch (state) {
            case LS_START:
               if (Util.isAlpha(nextChar)) {
                  localStringValue.append(nextChar);
                  state = LexState.LS_IDENT;
               } else if (nextChar == '-') {
                  sign = -1;
                  state = LexState.LS_SIGN;
               } else if (nextChar == '+') {
                  state = LexState.LS_SIGN;
               } else if (Util.isDigit(nextChar)) {
                  localIntValue = nextChar - '0';
                  state = LexState.LS_INTEGER;
               } else if (nextChar == ',') {
                  aToken = new TComma();
                  state = LexState.LS_STOP;
               } else if (nextChar == '(') {
                  aToken = new TLeftParen();
                  state = LexState.LS_STOP;
               } else if (nextChar == ')') {
                  aToken = new TRightParen();
                  state = LexState.LS_STOP;
               } else if (nextChar == '\n') {
                  state = LexState.LS_STOP;
               } else if (nextChar != ' ') {
                  aToken = new TInvalid();
               }
               break;
            case LS_IDENT:
               if (Util.isAlpha(nextChar) || Util.isDigit(nextChar)) {
                  localStringValue.append(nextChar);
               } else {
                  b.backUpInput();
                  aToken = new TIdentifier(localStringValue);
                  state = LexState.LS_STOP;
               }
               break;
            case LS_SIGN:
               if (Util.isDigit(nextChar)) {
                  localStringValue.append(nextChar);
                  state = LexState.LS_INTEGER;
               } else {
                  aToken = new TInvalid();
               }
               break;
            case LS_INTEGER:
               if (Util.isDigit(nextChar)) {
                  localIntValue = 10 * localIntValue + nextChar - '0';
               } else {
                  b.backUpInput();
                  aToken = new TInteger(sign * localIntValue);
                  state = LexState.LS_STOP;
               }
               break;
         }
      } while ((state != LexState.LS_STOP) && !(aToken instanceof TInvalid));
      return aToken;
   }
}
