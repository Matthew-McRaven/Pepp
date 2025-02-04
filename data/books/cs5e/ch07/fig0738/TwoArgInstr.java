package fig0738;

/**
 * The two-argument instruction subclass of the abstract code class.
 *
 * <p>
 * File: <code>TwoArgInstr.java</code>
 *
 * @author J. Stanley Warford
 */
public class TwoArgInstr extends ACode {

   private final Mnemon mnemonic;
   private final AArg firstArg;
   private final AArg secondArg;

   public TwoArgInstr(Mnemon mn, AArg fArg, AArg sArg) {
      mnemonic = mn;
      firstArg = fArg;
      secondArg = sArg;
   }

   @Override
   public String generateListing() {
      return String.format("%s (%s, %s)\n",
              Maps.mnemonStringTable.get(mnemonic),
              firstArg.generateCode(),
              secondArg.generateCode());
   }

   @Override
   public String generateCode() {
      switch (mnemonic) {
         case M_SET:
            return String.format("%s <- %s\n",
                    firstArg.generateCode(),
                    secondArg.generateCode());
         case M_ADD:
            return String.format("%s <- %s + %s\n",
                    firstArg.generateCode(),
                    firstArg.generateCode(),
                    secondArg.generateCode());
         case M_SUB:
            return String.format("%s <- %s - %s\n",
                    firstArg.generateCode(),
                    firstArg.generateCode(),
                    secondArg.generateCode());
         case M_MUL:
            return String.format("%s <- %s * %s\n",
                    firstArg.generateCode(),
                    firstArg.generateCode(),
                    secondArg.generateCode());
         case M_DIV:
            return String.format("%s <- %s / %s\n",
                    firstArg.generateCode(),
                    firstArg.generateCode(),
                    secondArg.generateCode());
         default:
            return ""; // Should not occur.
      }
   }
}
