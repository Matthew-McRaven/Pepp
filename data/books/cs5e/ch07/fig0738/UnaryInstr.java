package fig0738;

/**
 * The unary instruction subclass of the abstract code class.
 *
 * <p>
 * File: <code>UnaryInstr.java</code>
 *
 * @author J. Stanley Warford
 */
public class UnaryInstr extends ACode {

   private final Mnemon mnemonic;

   public UnaryInstr(Mnemon mn) {
      mnemonic = mn;
   }

   @Override
   public String generateListing() {
      return Maps.mnemonStringTable.get(mnemonic) + "\n";
   }

   @Override
   public String generateCode() {
      switch (mnemonic) {
         case M_STOP:
            return "stop\n";
         case M_END:
            return "";
         default:
            return ""; // Should not occur.
      }
   }
}
