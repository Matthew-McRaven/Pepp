package fig0738;

/**
 * Utility functions to test for digit characters and alphabetic characters.
 *
 * <p>
 * File: <code>Util.java</code>
 *
 * @author J. Stanley Warford
 */
public class Util {

    public static boolean isDigit(char ch) {
        return ('0' <= ch) && (ch <= '9');
    }

    public static boolean isAlpha(char ch) {
        return (('a' <= ch) && (ch <= 'z') || ('A' <= ch) && (ch <= 'Z'));
    }
}
