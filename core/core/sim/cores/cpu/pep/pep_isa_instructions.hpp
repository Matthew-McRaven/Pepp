#include <tuple>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep_shared_ops.hpp"
#include "core/integers.h"

class PepISA3CPU;

using Op = isa::SharedOp;


// Read word at Mem[PC] and store to OS, incrementing PC by 2.
// Return the /address/ of the operand value, which is usable for both load and store instructions.
// For store-type operands, this is the address you write to. For load-type operands, you will need to read from this
// address to get the actual operand specifier.
u16 decode_op_addr(PepISA3CPU *self, isa::SharedAddrMode addr);
// Read word at Mem[PC] and store to OS, incrementing PC by 2. Return the /value/ of the operand, which is usable for
// load-type instructions. This is an optimization over decode_op_addr, which incurrs an extra memory read for load-type
// instructions. Must not be used for store-type instructions
u16 decode_op_value_word(PepISA3CPU *self, isa::SharedAddrMode addr);
u8 decode_op_value_byte(PepISA3CPU *self, isa::SharedAddrMode addr);

void unimpl_handler(PepISA3CPU *);

void handle_ret(PepISA3CPU *self);
void handle_sret(PepISA3CPU *self);
void handle_movflga(PepISA3CPU *self);
void handle_movaflg(PepISA3CPU *self);
void handle_movspa(PepISA3CPU *self);
void handle_movasp(PepISA3CPU *self);
void handle_nop(PepISA3CPU *);

void handle_negr(PepISA3CPU *self, isa::Pep10::Register reg);
void handle_aslr(PepISA3CPU *self, isa::Pep10::Register reg);
void handle_asrr(PepISA3CPU *self, isa::Pep10::Register reg);
void handle_notr(PepISA3CPU *self, isa::Pep10::Register reg);
void handle_rolr(PepISA3CPU *self, isa::Pep10::Register reg);
void handle_rorr(PepISA3CPU *self, isa::Pep10::Register reg);

enum class BranchCondition { UNCONDITIONAL, LE, LT, EQ, NE, GE, GT, V, C };

void handle_branch(PepISA3CPU *self, Op op, BranchCondition cond, u16 op_val);
// Specialization of handle_branch() which executes more efficiently.
void handle_unconditional_branch(PepISA3CPU *self, Op op, u16 op_val);
void handle_call(PepISA3CPU *self, Op op, u16 op_val);

void handle_addsp(PepISA3CPU *self, Op op, u16 op_val);
void handle_subsp(PepISA3CPU *self, Op op, u16 op_val);
void handle_addr(PepISA3CPU *self, Op op, u16 op_val);
void handle_subr(PepISA3CPU *self, Op op, u16 op_val);

enum class Bitop {
  AND,
  OR,
  XOR,
};

void handle_bitopr(PepISA3CPU *self, Op op, Bitop bitop, u16 op_val);
void handle_cpwr(PepISA3CPU *self, Op op, u16 op_val);
void handle_cpbr(PepISA3CPU *self, Op op, u8 u16);

void handle_ldwr(PepISA3CPU *self, Op op, u16 u16);
void handle_ldbr(PepISA3CPU *self, Op op, u8 u16);
void handle_stwr(PepISA3CPU *self, Op op, u16 op_addr);
void handle_stbr(PepISA3CPU *self, Op op, u16 op_addr);
