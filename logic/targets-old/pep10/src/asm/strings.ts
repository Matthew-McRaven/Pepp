// eslint-disable-next-line import/prefer-default-export
export const unquotedStringToBytes = (string:string):Uint8Array | string => {
  const bytes:number[] = [];

  // Last char was a \, so we now expect an escape sequence
  let expectFollowBash = false;
  // We're in the middle of a hex escape code, and this is how many chars we have left. If 0, not in hex code.
  let hexCharsNeeded = 0;
  // Track hex chars seen in the current escape code
  let hexChars = [];

  // Attempt the most nested parse first (hex code=>escape code=>normal) so we can use normal iteration logic.
  // At end, must check if any condition flags remain set, indicating the string ends in an incomplete escape sequence.
  for (let index = 0; index < string.length; index += 1) {
    const char = string[index];
    if (hexCharsNeeded !== 0) {
      // TODO: Fix placeholder errors
      if (!/^[a-fA-f0-9]$/.test(char)) {
        return 'Invalid hex escape code. Expected hex value, got {PLACEHOLDER}';
      }
      hexChars.push(char);
      hexCharsNeeded -= 1;
      if (hexChars.length === 2) {
        bytes.push(Number.parseInt(hexChars.join(''), 16));
        hexChars = [];
      }
    } else if (expectFollowBash) {
      // Parse our list of acceptable escape sequences
      expectFollowBash = false;
      // TODO: handle escape \\
      switch (char) {
        case 'b':
          bytes.push(8);
          break;
        case 'f':
          bytes.push(12);
          break;
        case 'n':
          bytes.push(10);
          break;
        case 'r':
          bytes.push(13);
          break;
        case 't':
          bytes.push(9);
          break;
        case 'v':
          bytes.push(11);
          break;
          // Hex codes need to consume two additional chars.
        case 'x':
          hexCharsNeeded = 2;
          break;
        case 'X':
          hexCharsNeeded = 2;
          break;
        default:
          return `Invalid escape code \\${char}`;
      }
    } else if (char === '\\') {
      // Starting an escape sequence
      expectFollowBash = true;
    } else bytes.push(char.charCodeAt(0));
  }
  // Check that wwe did not run out of input parsing escape codes.
  if (expectFollowBash) return 'String ends in invalid escape code';
  if (hexCharsNeeded > 0) return 'String ends in invalid hex escape code';
  // TODO: split multi-byte characters into [0-255]
  return Uint8Array.from(bytes);
};
