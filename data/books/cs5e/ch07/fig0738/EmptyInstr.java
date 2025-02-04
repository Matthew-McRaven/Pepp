package fig0738;

/**
 * The empty instruction code subclass of the abstract code class.
 *
 * <p>
 * An object of this class corresponds to an empty line in the source file.
 *
 * <p>
 * File: <code>EmptyInstr.java</code>
 *
 * @author J. Stanley Warford
 */
public class EmptyInstr extends ACode {
   // For an empty source line.

   @Override
   public String generateListing() {
      return "\n";
   }

   @Override
   public String generateCode() {
      return "";
   }
}
