package fig0738;

/**
 * The enumerated values for the states of the parser finite state machine.
 *
 * <p>
 * File: <code>ParseState.java</code>
 *
 * @author J. Stanley Warford
 */
public enum ParseState {

   PS_START, PS_UNARY, PS_FUNCTION, PS_OPEN, PS_1ST_OPRND, PS_NON_UNARY1,
   PS_COMMA, PS_2ND_OPRND, PS_NON_UNARY2, PS_FINISH
}
