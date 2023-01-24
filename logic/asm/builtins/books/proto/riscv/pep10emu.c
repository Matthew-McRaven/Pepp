#include <stdint.h>
#include <stdio.h>
enum mask {
	MASK_N = 8,
	MASK_Z = 4,
	MASK_V =2,
	MASK_C = 1
};

enum Instructions {
    i_stop = 0, i_ret = 1, i_rettr = 2, i_movspa = 3, i_movflga = 4, i_movaflg = 5, i_nota = 6,
    i_notx = 7, i_nega = 8, i_negx = 9, i_asla = 10, i_aslx = 11, i_asra = 12, i_asrx = 13,
 	i_rola = 14, i_rolx = 15, i_rora = 16, i_rorx = 17, i_br = 18, i_brle = 20, i_brlt = 22,
    i_breq = 24, i_brne = 26, i_brge = 28, i_brgt = 30, i_brv = 32, i_brc = 34, i_call = 36,
	i_nop0 = 38, i_nop1 = 39, i_nop = 40, i_deci = 48, i_deco = 56, i_hexo = 64, i_stro = 72,

    i_addsp = 80, i_subsp = 88, i_adda = 96, i_addx = 104, i_suba = 112, i_subx = 120,
	i_anda = 128,i_andx = 136, i_ora = 144, i_orx = 152, i_cpwa = 160, i_cpwx = 168, 
	i_cpba = 176, i_cpbx = 184, i_ldwa = 192, i_ldwx = 200, i_ldba = 208, i_ldbx = 216,
	i_stwa = 224, i_stwx = 232, i_stba = 240, i_stbx = 248
};

// Decoder table for instructions in the Pep/9 instruction set.
enum Instructions instruction_array[256] =
{
    i_stop,    i_ret,     i_rettr,   i_movspa,  i_movflga, i_movaflg, i_nota,    i_notx,      // 7
    i_nega,    i_negx,    i_asla,    i_aslx,    i_asra,    i_asrx,    i_rola,    i_rolx,      // 15
    i_rora,    i_rorx,    i_br,      i_br,      i_brle,    i_brle,    i_brlt,    i_brlt,      // 23
    i_breq,    i_breq,    i_brne,    i_brne,    i_brge,    i_brge,    i_brgt,    i_brgt,      // 31
    i_brv,     i_brv,     i_brc,     i_brc,     i_call,    i_call,    i_nop0,    i_nop1,      // 39
    i_nop,     i_nop,     i_nop,     i_nop,     i_nop,     i_nop,     i_nop,     i_nop,       // 47
    i_deci,    i_deci,    i_deci,    i_deci,    i_deci,    i_deci,    i_deci,    i_deci,      // 55
    i_deco,    i_deco,    i_deco,    i_deco,    i_deco,    i_deco,    i_deco,    i_deco,      // 63
    i_hexo,    i_hexo,    i_hexo,    i_hexo,    i_hexo,    i_hexo,    i_hexo,    i_hexo,      // 71
    i_stro,    i_stro,    i_stro,    i_stro,    i_stro,    i_stro,    i_stro,    i_stro,      // 79
    i_addsp,   i_addsp,   i_addsp,   i_addsp,   i_addsp,   i_addsp,   i_addsp,   i_addsp,     // 87
    i_subsp,   i_subsp,   i_subsp,   i_subsp,   i_subsp,   i_subsp,   i_subsp,   i_subsp,     // 95
    i_adda,    i_adda,    i_adda,    i_adda,    i_adda,    i_adda,    i_adda,    i_adda,      // 103
    i_addx,    i_addx,    i_addx,    i_addx,    i_addx,    i_addx,    i_addx,    i_addx,      // 111
    i_suba,    i_suba,    i_suba,    i_suba,    i_suba,    i_suba,    i_suba,    i_suba,      // 119
    i_subx,    i_subx,    i_subx,    i_subx,    i_subx,    i_subx,    i_subx,    i_subx,      // 127
    i_anda,    i_anda,    i_anda,    i_anda,    i_anda,    i_anda,    i_anda,    i_anda,      // 135
    i_andx,    i_andx,    i_andx,    i_andx,    i_andx,    i_andx,    i_andx,    i_andx,      // 143
    i_ora,     i_ora,     i_ora,     i_ora,     i_ora,     i_ora,     i_ora,     i_ora,       // 151
    i_orx,     i_orx,     i_orx,     i_orx,     i_orx,     i_orx,     i_orx,     i_orx,       // 159
    i_cpwa,    i_cpwa,    i_cpwa,    i_cpwa,    i_cpwa,    i_cpwa,    i_cpwa,    i_cpwa,      // 167
    i_cpwx,    i_cpwx,    i_cpwx,    i_cpwx,    i_cpwx,    i_cpwx,    i_cpwx,    i_cpwx,      // 175
    i_cpba,    i_cpba,    i_cpba,    i_cpba,    i_cpba,    i_cpba,    i_cpba,    i_cpba,      // 183
    i_cpbx,    i_cpbx,    i_cpbx,    i_cpbx,    i_cpbx,    i_cpbx,    i_cpbx,    i_cpbx,      // 191
    i_ldwa,    i_ldwa,    i_ldwa,    i_ldwa,    i_ldwa,    i_ldwa,    i_ldwa,    i_ldwa,      // 199
    i_ldwx,    i_ldwx,    i_ldwx,    i_ldwx,    i_ldwx,    i_ldwx,    i_ldwx,    i_ldwx,      // 207
    i_ldba,    i_ldba,    i_ldba,    i_ldba,    i_ldba,    i_ldba,    i_ldba,    i_ldba,      // 215
    i_ldbx,    i_ldbx,    i_ldbx,    i_ldbx,    i_ldbx,    i_ldbx,    i_ldbx,    i_ldbx,      // 223
    i_stwa,    i_stwa,    i_stwa,    i_stwa,    i_stwa,    i_stwa,    i_stwa,    i_stwa,      // 231
    i_stwx,    i_stwx,    i_stwx,    i_stwx,    i_stwx,    i_stwx,    i_stwx,    i_stwx,      // 239
    i_stba,    i_stba,    i_stba,    i_stba,    i_stba,    i_stba,    i_stba,    i_stba,      // 247
    i_stbx,    i_stbx,    i_stbx,    i_stbx,    i_stbx,    i_stbx,    i_stbx,    i_stbx       // 255
};

enum AddressingModes
{
    i_addr, d_addr, n_addr,
    s_addr, sf_addr, x_addr,
    sx_addr, sfx_addr,
    NONE_ADDR =0x9
};

// Decoder table for instructions in the Pep/9 instruction set.
enum AddressingModes instr_addr_mode[256] =
{
    NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,      	  // 7
    NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,NONE_ADDR,		  // 15
    NONE_ADDR,NONE_ADDR,i_addr,x_addr,i_addr,x_addr,i_addr,x_addr,     						  // 23
    i_addr,x_addr,i_addr,x_addr,i_addr,x_addr,i_addr,x_addr,      							  // 31
    i_addr,x_addr,i_addr,x_addr,i_addr,x_addr,    NONE_ADDR,    NONE_ADDR,      			  // 39
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       						  // 47
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       						  // 55
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 63
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 71
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 79
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 87
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,     						  // 95
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,    						  // 103
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,     						  // 111
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       						  // 119
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,     	   					  // 127
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 135
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       						  // 143
   	i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,      						  // 151
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       					 	  // 159
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 167
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 175
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 							  // 183
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 					          // 191
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 					          // 199
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 					          // 207
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 215
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 223
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 231
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,    						  // 239
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr, 						      // 247
    i_addr,d_addr,n_addr,s_addr,sf_addr,x_addr,sx_addr,sfx_addr,       						  // 255
};

struct CPU {
	short ACC;
	short IDX;
	short SP;
	short PC;
	unsigned char IR;
	unsigned short OS;
	short TR;
	unsigned char NZVC; 
};
// designated init to prevent first few bytes from being 0's
unsigned char memory[0x10000] = { [ 0 ... 2 ] = 0b110, 0 };
void stop();
void printi(int);
void prints(const char*);
unsigned short addr_d(unsigned short addr)
{
	unsigned char lo, hi;
	hi = memory[addr]; lo = memory[addr+1];
	return (hi<<8) + (lo);
};

inline void not(unsigned short* reg, unsigned char* NZVC) {
	unsigned short new = ~*reg;
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	*reg = ~(*reg);
}

inline void neg(unsigned short* reg, unsigned char* NZVC) {
	unsigned short new = ~(*reg) + 1;
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	// Don't rember how to do overflow.
	// *NZVC = (*NZVC & MASK_V) | ((new & *reg) >> 15 ? MASK_V:0);
	*reg = new;
}
inline void asl(unsigned short* reg, unsigned char* NZVC) {
	unsigned short new = (*reg) << 1;
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	// Don't rember how to do overflow.
	// *NZVC = (*NZVC & MASK_V) | ((new & *reg) >> 15 ? MASK_V:0);
	*NZVC = (*NZVC & MASK_C) | ((*reg & 0x8000)? MASK_C:0);
	*reg = new;
}
inline void asr(unsigned short* reg, unsigned char* NZVC) {
	// Keep high order bit the same
	unsigned short new = (*reg) >> 1;
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	*NZVC = (*NZVC & MASK_C) | ((*reg & 1)? MASK_C:0);
	*reg = new;
}
inline void rol(unsigned short* reg, unsigned char* NZVC) {
	unsigned short new = (*reg) << 1 | ( *NZVC & MASK_C ? 1 : 0);
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	*NZVC = (*NZVC & MASK_C) | ((*reg & 0x8000)? MASK_C:0);
	*reg = new;
}

inline void ror(unsigned short* reg, unsigned char* NZVC) {
	unsigned short new = (0x8000 & ((*reg) >> 1)) | ( *NZVC & MASK_C ? 0x8000 : 0);
	*NZVC = (*NZVC & MASK_N) | (new & 0x8000 ? MASK_N:0);
	*NZVC = (*NZVC & MASK_Z) | (new == 0  ? MASK_Z:0);
	*NZVC = (*NZVC & MASK_C) | ((*reg & 0x1)? MASK_C:0);
	*reg = new;
}

int isunary(unsigned char val){ return instr_addr_mode[val] == NONE_ADDR;}
void execute_unary(struct CPU* cpu){
	switch(cpu->IR) {
	case i_stop: 
		stop(); 
		break;
	case i_ret: 
		cpu->PC = addr_d(cpu->SP);
 		cpu->SP += 2;
		break;
	case i_rettr: break; 
	case i_movspa:
		cpu->ACC = cpu->SP;
		break;
	case i_movflga:
		cpu->ACC = cpu->NZVC;
		break;
	case i_movaflg:
		cpu->NZVC = cpu->ACC & 0b1111;
		break;
	case i_nota: not(&cpu->ACC, &cpu->NZVC); break;
	case i_notx: not(&cpu->IDX, &cpu->NZVC); break;
	case i_nega: neg(&cpu->ACC, &cpu->NZVC); break;
	case i_negx: neg(&cpu->IDX, &cpu->NZVC); break;
	case i_asla: asl(&cpu->ACC, &cpu->NZVC); break;
	case i_aslx: asl(&cpu->IDX, &cpu->NZVC); break;
	case i_asra: asr(&cpu->ACC, &cpu->NZVC); break;
	case i_asrx: asr(&cpu->IDX, &cpu->NZVC); break;
	case i_rola: rol(&cpu->ACC, &cpu->NZVC); break;
	case i_rolx: rol(&cpu->IDX, &cpu->NZVC); break;
	case i_rora: ror(&cpu->ACC, &cpu->NZVC); break;
	case i_rorx: ror(&cpu->IDX, &cpu->NZVC); break;
		
	}
}
unsigned short decode_addr(struct CPU* cpu, unsigned char ir, unsigned short op_val) { 
	switch (instr_addr_mode[ir]) {
	case i_addr: return op_val;
	case d_addr: return addr_d(op_val);
	case n_addr: return addr_d(addr_d(op_val));
	case s_addr: return addr_d(op_val + cpu->SP);
	case sf_addr: return addr_d(addr_d(op_val + cpu->SP));
	case sx_addr: return addr_d(op_val + cpu->IDX + cpu->SP);
	case sfx_addr: return addr_d(addr_d(op_val + cpu->SP) + cpu->IDX);

	}
}
void execute_nonunary(struct CPU* cpu){
	unsigned short temp;
	switch(cpu->IR) {
	case i_br: cpu->PC=decode_addr(cpu, cpu->IR, cpu->OS); break;
		
	}
}
int main() {
	struct CPU cpu = {.ACC=0, .IDX=0, .SP=0,. PC=0, .TR=0, .NZVC=0};
    // Begin von neuman cycle
loop:while(1) {
		// Fetch IR and increment PC
		cpu.IR = memory[cpu.PC];
		cpu.PC++;
		printi(cpu.ACC);
		prints("\n");
		// Execute code if non-unary
		if(isunary(cpu.IR)) execute_unary(&cpu);
		// Otherwise fetch OS.
		else {
			cpu.OS = addr_d(cpu.PC);
			cpu.PC += 2;
			execute_nonunary(&cpu);
		}
	}
    stop();
}

void prints(const char* a){ // ptr is passed through register a0
	// Move a to register a0.
	__asm__ volatile(
		"add a0, x0, %0\n" 
		"li a7, 4\n"
		"ecall"
		: : "r"(a)
	);
}

void printi(int a){ // ptr is passed through register a0
	// Move a to register a0.
	__asm__ volatile(
		"add a0, x0, %0\n" 
		"li a7, 34\n"
		"ecall"
		: : "r"(a)
	);
}

void stop() {
	asm volatile("li a7, 10");
	asm volatile("ecall");
}



