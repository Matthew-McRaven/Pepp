package fig0738;

/**
 * The integer token subclass of the abstract token.
 *
 * <p>
 * File: <code>TInteger.java</code>
 *
 * @author J. Stanley Warford
 */
public class TInteger extends AToken {

   private final int intValue;

   public TInteger(int i) {
      intValue = i;
   }

   public int getIntValue() {
      return intValue;
   }
}
