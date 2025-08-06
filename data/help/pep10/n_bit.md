### Setting the N bit on `CPr`

Normally, the N bit duplicates the sign bit, so that N is 1 when the result of the operation is negative.
The compare instruction `CPr` subtracts the operand from the register and sets the status bits without storing the result of the subtraction.

As long as there is no overflow when the operands are interpreted as signed integers, the N bit is set appropriately for a subsequent conditional branch instruction.
If the result of the subtraction yields an overflow and the N bit were set as usual, the subsequent conditional branch instruction might execute an erroneous branch.
Consequently, if the `CPr` subtraction operation overflows and sets the V bit, then the N bit is inverted from its normal value and does not duplicate the sign bit.

With this adjustment, the compare operation extends the range of valid comparisons.
Even though there is an overflow, the N bit is set as if there were no overflow so that a subsequent conditional branch will operate as expected.
