### Setting the C bit on subtraction

There are two different philosophies for setting the carry bit after subtraction.

Z80, x86, and SPARC treat the carry flag as a "borrow flag" when doing a subtraction. When subtracting x - y, if x is less than y (treating both operands as unsigned), the carry flag is set because there is a borrow. A BRC after a `SUBr` or `CPr` instruction is equivalent to "branch if unsigned overflow" in this case.

On other processor families, such as ARM and PowerPC, the carry flag after a subtraction is set to the adder carry output after computing (x + ~y + 1). When subtracting x - y, if x is greater than or equal to y (treating both operands as unsigned), the carry flag is set. A BRC after a `SUBr` or `CPr` instruction is equivalent to "branch if not unsigned overflow" in this case.

Versions of Pep/9 previous to 8.1.0 set the C bit on subtraction according to the first philosophy. However, starting with version 8.1.0 and for all Pep/9 versions, the C bit on subtraction is set according to the second philosophy. This is consistent with the section "The Carry Bit" in Chapter 3, and the adder/subtracter circuit in Figure 10.53 in the text.
