### ALU Functions

| **ALU Control** || **Result**        | **Status bits** ||      |          |
|:------|--------:|:------------------:|:-----|----------|-------|---------:|
| (bin) | (dec)   |                    | **N**| **Zout** | **V** | **Cout** |
| 0000  | 0       | A                  | N    | Z        | 0     | 0        |
| 0001  | 1       | A plus B           | N    | Z        | V     | C        |
| 0010  | 2       | A plus B plus Cin  | N    | Z        | V     | C        |
| 0011  | 3       | A plus ~B plus 1   | N    | Z        | V     | C        |
| 0100  | 4       | A plus ~B plus Cin | N    | Z        | V     | C        |
| 0101  | 5       | A AND B            | N    | Z        | 0     | 0        |
| 0110  | 6       | A NAND B           | N    | Z        | 0     | 0        |
| 0111  | 7       | A OR B             | N    | Z        | 0     | 0        |
| 1000  | 8       | A NOR B            | N    | Z        | 0     | 0        |
| 1009  | 9       | A XOR B            | N    | Z        | 0     | 0        |
| 1010  | 10      | ~A                 | N    | Z        | 0     | 0        |
| 1011  | 11      | ASL A              | N    | Z        | V     | C        |
| 1100  | 12      | ROL A              | N    | Z        | 0     | C        |
| 1101  | 13      | ASR A              | N    | Z        | 0     | C        |
| 1110  | 14      | ROR A              | N    | Z        | 0     | C        |
| 1111  | 15      | 0                  | A[4] | A[5]     | A[6]  | A[7]     |
