package fig0738;

/**
 * The identifier argument subclass of the abstract argument class.
 *
 * <p>
 * File: <code>IdentArg.java</code>
 *
 * @author J. Stanley Warford
 */
public class IdentArg extends AArg {

   private final String identValue;

   public IdentArg(String str) {
      identValue = str;
   }

   @Override
   public String generateCode() {
      return identValue;
   }
}
