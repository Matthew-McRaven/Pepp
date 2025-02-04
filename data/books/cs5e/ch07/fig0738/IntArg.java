package fig0738;

/**
 * The integer argument subclass of the abstract argument class.
 *
 * <p>
 * File: <code>IntArg.java</code>
 *
 * @author J. Stanley Warford
 */
public class IntArg extends AArg {

   private final int intValue;

   public IntArg(int i) {
      intValue = i;
   }

   @Override
   public String generateCode() {
      return String.format("%d", intValue);
   }
}
