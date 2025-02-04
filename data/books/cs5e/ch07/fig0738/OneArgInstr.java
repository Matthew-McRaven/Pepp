package fig0738;

/**
 * The one-argument instruction subclass of the abstract code class.
 *
 * <p>
 * File: <code>OneArgInstr.java</code>
 *
 * @author J. Stanley Warford
 */
public class OneArgInstr extends ACode {

   private final Mnemon mnemonic;
   private final AArg aArg;

   public OneArgInstr(Mnemon mn, AArg aArg) {
      mnemonic = mn;
      this.aArg = aArg;
   }

   @Override
   public String generateListing() {
      return String.format("%s (%s)\n",
              Maps.mnemonStringTable.get(mnemonic),
              aArg.generateCode());
   }

   @Override
   public String generateCode() {
      switch (mnemonic) {
         case M_ABS:
            return String.format("%s <- |%s|\n",
                    aArg.generateCode(),
                    aArg.generateCode());
         case M_NEG:
            return String.format("%s <- -%s\n",
                    aArg.generateCode(),
                    aArg.generateCode());
         default:
            return ""; // Should not occur.
      }
   }
}
