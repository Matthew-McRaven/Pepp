package fig0738;

import java.util.ArrayList;

/**
 * The parser implemented as a finite state machine.
 *
 * <p>
 * File: <code>Translator.java</code>
 *
 * @author J. Stanley Warford
 */
public class Translator {

   private final InBuffer b;
   private Tokenizer t;
   private ACode aCode;

   public Translator(InBuffer inBuffer) {
      b = inBuffer;
   }

   // Sets aCode and returns boolean true if end statement is processed.
   private boolean parseLine() {
      boolean terminate = false;
      AArg localFirstArg = new IntArg(0);
      AArg localSecondArg;
      Mnemon localMnemon = Mnemon.M_END; // Useless initialization
      AToken aToken;
      aCode = new EmptyInstr();
      ParseState state = ParseState.PS_START;
      do {
         aToken = t.getToken();
         switch (state) {
            case PS_START:
               if (aToken instanceof TIdentifier) {
                  TIdentifier localTIdentifier = (TIdentifier) aToken;
                  String tempStr = localTIdentifier.getStringValue();
                  if (Maps.unaryMnemonTable.containsKey(
                        tempStr.toLowerCase())) {
                     localMnemon = Maps.unaryMnemonTable.get(
                           tempStr.toLowerCase());
                     aCode = new UnaryInstr(localMnemon);
                     terminate = localMnemon == Mnemon.M_END;
                     state = ParseState.PS_UNARY;
                  } else if (Maps.nonUnaryMnemonTable.containsKey(
                        tempStr.toLowerCase())) {
                     localMnemon = Maps.nonUnaryMnemonTable.get(
                           tempStr.toLowerCase());
                     state = ParseState.PS_FUNCTION;
                  } else {
                     aCode = new Error(
                           "Line must begin with function identifier.");
                  }
               } else if (aToken instanceof TEmpty) {
                  aCode = new EmptyInstr();
                  state = ParseState.PS_FINISH;
               } else {
                  aCode = new Error(
                        "Line must begin with function identifier.");
               }
               break;
            case PS_FUNCTION:
               if (aToken instanceof TLeftParen) {
                  state = ParseState.PS_OPEN;
               } else {
                  aCode = new Error(
                        "Left parenthesis expected after function.");
               }
               break;
            case PS_OPEN:
               if (aToken instanceof TIdentifier) {
                  TIdentifier localTIdentifier = (TIdentifier) aToken;
                  localFirstArg = new IdentArg(
                        localTIdentifier.getStringValue());
                  state = ParseState.PS_1ST_OPRND;
               } else {
                  aCode = new Error("First argument not an identifier.");
               }
               break;
            case PS_1ST_OPRND:
               if (localMnemon == Mnemon.M_NEG
                     || localMnemon == Mnemon.M_ABS) {
                  if (aToken instanceof TRightParen) {
                     aCode = new OneArgInstr(localMnemon, localFirstArg);
                     state = ParseState.PS_NON_UNARY1;
                  } else {
                     aCode = new Error(
                           "Right parenthesis expected after argument.");
                  }
               } else if (aToken instanceof TComma) {
                  state = ParseState.PS_COMMA;
               } else {
                  aCode = new Error(
                        "Comma expected after first argument.");
               }
               break;
            case PS_COMMA:
               if (aToken instanceof TIdentifier) {
                  TIdentifier localTIdentifier = (TIdentifier) aToken;
                  localSecondArg = new IdentArg(
                        localTIdentifier.getStringValue());
                  aCode = new TwoArgInstr(
                        localMnemon, localFirstArg, localSecondArg);
                  state = ParseState.PS_2ND_OPRND;
               } else if (aToken instanceof TInteger) {
                  TInteger localTInteger = (TInteger) aToken;
                  localSecondArg = new IntArg(
                        localTInteger.getIntValue());
                  aCode = new TwoArgInstr(
                        localMnemon, localFirstArg, localSecondArg);
                  state = ParseState.PS_2ND_OPRND;
               } else {
                  aCode = new Error(
                        "Second argument not an identifier or integer.");
               }
               break;
            case PS_2ND_OPRND:
               if (aToken instanceof TRightParen) {
                  state = ParseState.PS_NON_UNARY2;
               } else {
                  aCode = new Error(
                        "Right parenthesis expected after argument.");
               }
               break;
            case PS_UNARY:
            case PS_NON_UNARY1:
            case PS_NON_UNARY2:
               if (aToken instanceof TEmpty) {
                  state = ParseState.PS_FINISH;
               } else {
                  aCode = new Error("Illegal trailing character.");
               }
               break;
         }
      } while (state != ParseState.PS_FINISH && !(aCode instanceof Error));
      return terminate;
   }

   public void translate() {
      ArrayList<ACode> codeTable = new ArrayList<>();
      int numErrors = 0;
      t = new Tokenizer(b);
      boolean terminateWithEnd = false;
      b.getLine();
      while (b.inputRemains() && !terminateWithEnd) {
         terminateWithEnd = parseLine(); // Sets aCode and returns boolean.
         codeTable.add(aCode);
         if (aCode instanceof Error) {
            numErrors++;
         }
         b.getLine();
      }
      if (!terminateWithEnd) {
         aCode = new Error("Missing \"end\" sentinel.");
         codeTable.add(aCode);
         numErrors++;
      }
      if (numErrors == 0) {
         System.out.print("Object code:\n");
         for (ACode code : codeTable) {
            System.out.printf("%s", code.generateCode());
         }
      }
      if (numErrors == 1) {
         System.out.print("One error was detected.\n");
      } else if (numErrors > 1) {
         System.out.printf("%d errors were detected.\n", numErrors);
      }
      System.out.print("\nProgram listing:\n");
      for (ACode code : codeTable) {
         System.out.printf("%s", code.generateListing());
      }
   }
}
