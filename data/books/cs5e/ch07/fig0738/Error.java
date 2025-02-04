package fig0738;

/**
 * The error code subclass of the abstract code class.
 *
 * <p>
 * An object of this class corresponds to an error in the source file.
 *
 * <p>
 * File: <code>Error.java</code>
 *
 * @author J. Stanley Warford
 */
public class Error extends ACode {

   private final String errorMessage;

   public Error(String errMessage) {
      errorMessage = errMessage;
   }

   @Override
   public String generateListing() {
      return "ERROR: " + errorMessage + "\n";
   }

   @Override
   public String generateCode() {
      return "";
   }
}
