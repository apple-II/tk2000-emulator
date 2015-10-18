/****************************************************************************
*
*  APPLE //E EMULATOR FOR WINDOWS                    
*
*  Copyright (C) 1994-96, Michael O'Brien.  All rights reserved.
*
***/

/*  Emulador do computador TK2000 (Microdigital)
 *  por Fábio Belavenuto - Copyright (C) 2004
 *
 *  Adaptado do emulador Applewin por Michael O'Brien
 *  Part of code is Copyright (C) 2003-2004 Tom Charlesworth
 *
 *  Este arquivo é distribuido pela Licença Pública Geral GNU.
 *  Veja o arquivo Licenca.txt distribuido com este software.
 *
 *  ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 *
 */

// Emula a CPU 65C02

#include "tk_stdhdr.h"
#include "tk_cpu.h"
#include "tk_memoria.h"
#include "tk_main.h"
#include "tk_debug.h"

#define  AF_SIGN       0x80
#define  AF_OVERFLOW   0x40
#define  AF_RESERVED   0x20
#define  AF_BREAK      0x10
#define  AF_DECIMAL    0x08
#define  AF_INTERRUPT  0x04
#define  AF_ZERO       0x02
#define  AF_CARRY      0x01

#define  INT_IRQ       0x01
#define  INT_NMI       0x02

#define  SHORTOPCODES  22
#define  BENCHOPCODES  33

// Variáveis

//DWORD  cpuemtype = CPU_COMPILING;
regsrec regs;
int     flagsint  = 0;

/****************************************************************************
*
*  GENERAL PURPOSE MACROS
*
***/

#define AF_TO_EF  flagc = (regs.ps & AF_CARRY);                             \
                  flagi = (regs.ps & AF_INTERRUPT);                              \
                  flagn = (regs.ps & AF_SIGN);                              \
                  flagv = (regs.ps & AF_OVERFLOW);                          \
                  flagz = (regs.ps & AF_ZERO);
#define EF_TO_AF  regs.ps = (regs.ps & ~(AF_CARRY | AF_SIGN |               \
                                         AF_OVERFLOW | AF_ZERO |            \
                                         AF_INTERRUPT ))                    \
                              | (flagc ? AF_CARRY    : 0)                   \
                              | (flagi ? AF_INTERRUPT: 0)                   \
                              | (flagn ? AF_SIGN     : 0)                   \
                              | (flagv ? AF_OVERFLOW : 0)                   \
                              | (flagz ? AF_ZERO     : 0);
#define CYC(a)   cycles += a; \
				 g_nCumulativeCycles += a; \
				 g_dCumulativeCycles += a; \
				 cumulativecycles += a;
/*
#define POP      (*(mem+((regs.sp >= 0x1FF) ? (regs.sp = 0x100) : ++regs.sp)))
#define PUSH(a)  *(mem+regs.sp--) = (a);                                    \
                 if (regs.sp < 0x100)                                       \
                   regs.sp = 0x1FF;
*/
#define POP      (mem_readb((regs.sp >= 0x1FF) ? (regs.sp = 0x100) : ++regs.sp, 1))
#define PUSH(a)  mem_writeb(regs.sp--, (a)); if (regs.sp < 0x100) regs.sp = 0x1FF;
#define SETNZ(a) {                                                          \
                   flagn = ((a) & 0x80);                                    \
                   flagz = !(a & 0xFF);                                     \
                 }
#define SETZ(a)  flagz = !(a & 0xFF);
#define TOBCD(a) (((((a)/10) % 10) << 4) | ((a) % 10))
#define TOBIN(a) (((a) >> 4)*10 + ((a) & 0x0F))

#define READ	(mem_readb(addr, 1))
#define WRITE(valor) { mem_writeb(addr, (byte)valor); }


/****************************************************************************
*
*  ADDRESSING MODE MACROS
*
***/

#define ABS      addr = mem_readw(regs.pc, 1);  regs.pc += 2;
#define ABSX     addr = mem_readw(regs.pc, 1)+(WORD)regs.x;  regs.pc += 2;
#define ABSY     addr = mem_readw(regs.pc, 1)+(WORD)regs.y;  regs.pc += 2;
#define IABS     addr = mem_readw(mem_readw(regs.pc, 1), 1);  regs.pc += 2;  
#define IMM      addr = regs.pc++;
#define INDX     addr = mem_readw((mem_readb(regs.pc++, 1)+regs.x) & 0xFF, 1);
#define INDY     addr = mem_readw(mem_readb(regs.pc++, 1), 1)+(WORD)regs.y;
#define REL      addr = (signed char)mem_readb(regs.pc++, 1);
#define ZPG      addr = mem_readb(regs.pc++, 1);
#define ZPGX     addr = (mem_readb(regs.pc++, 1)+regs.x) & 0xFF;
#define ZPGY     addr = (mem_readb(regs.pc++, 1)+regs.y) & 0xFF;

// Verifica se houve mudança de página, adicionando um ciclo extra
#define BRANCHTAKEN	WORD pca = (regs.pc & 0xFF00);	 						\
					regs.pc += addr;										\
					if ((regs.pc & 0xFF00) != pca) { CYC(1) }

/****************************************************************************
*
*  INSTRUCTION MACROS
*
***/

#define ADC      temp = READ;                                               \
                 if (regs.ps & AF_DECIMAL) {                                \
					unsigned int p1, p2 = 0;								\
					p1 = (regs.a & 0x0F) + (temp & 0x0F) + (flagc != 0);	\
					if (p1 > 0x09)											\
					{														\
						p1 -= 0x0A;											\
						p2 = 0x10;											\
					}														\
					p2 += (regs.a & 0xF0) + (temp & 0xF0);					\
					if (p2 > 0x90)											\
					{														\
						flagc = 1;											\
						p2 -= 0xA0;											\
					}														\
					else													\
						flagc = 0;											\
					regs.a = (p2 & 0xF0) | (p1 & 0x0F);						\
					SETNZ(regs.a);											\
                 }                                                          \
                 else {                                                     \
                   val    = regs.a+temp+(flagc != 0);                       \
                   flagc  = (val > 0xFF);                                   \
                   flagv  = (((regs.a & 0x80) == (temp & 0x80)) &&          \
                             ((regs.a & 0x80) != (val & 0x80)));            \
                   regs.a = val & 0xFF;                                     \
                   SETNZ(regs.a);                                           \
                 }
#define AND      regs.a &= READ;                                            \
                 SETNZ(regs.a)
#define ASL      val   = READ << 1;                                         \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define ASLA     val   = regs.a << 1;                                       \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 regs.a = (BYTE)val;
#define BCC      if (!flagc){ BRANCHTAKEN CYC(1) }
#define BCS      if ( flagc){ BRANCHTAKEN CYC(1) }
#define BEQ      if ( flagz){ BRANCHTAKEN CYC(1) }
#define BIT      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 flagn = val & 0x80;                                        \
                 flagv = val & 0x40;
#define BITI     flagz = !(regs.a & READ);

#define BMI      if ( flagn){ BRANCHTAKEN CYC(1) }
#define BNE      if (!flagz){ BRANCHTAKEN CYC(1) }
#define BPL      if (!flagn){ BRANCHTAKEN CYC(1) }
#define BRA      regs.pc += addr;
#define BRK      PUSH(++regs.pc >> 8)                                         \
                 PUSH(regs.pc & 0xFF)                                       \
                 EF_TO_AF                                                   \
                 regs.ps |= AF_BREAK;                                       \
                 PUSH(regs.ps)                                              \
                 regs.ps |= AF_INTERRUPT;                                   \
                 regs.pc = mem_readw(0xFFFE, 1);
#define BVC      if (!flagv){ BRANCHTAKEN CYC(1) }
#define BVS      if ( flagv){ BRANCHTAKEN CYC(1) }
#define CLC      flagc = 0;
#define CLD      regs.ps &= ~AF_DECIMAL;
#define CLI      regs.ps &= ~AF_INTERRUPT;
#define CLV      flagv = 0;
#define CMP      val   = READ;                                              \
                 flagc = (regs.a >= val);                                   \
                 val   = regs.a-val;                                        \
                 SETNZ(val)
#define CPX      val   = READ;                                              \
                 flagc = (regs.x >= val);                                   \
                 val   = regs.x-val;                                        \
                 SETNZ(val)
#define CPY      val   = READ;                                              \
                 flagc = (regs.y >= val);                                   \
                 val   = regs.y-val;                                        \
                 SETNZ(val)
#define DEA      --regs.a;                                                  \
                 SETNZ(regs.a)
#define DEC      val = READ-1;                                              \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define DEX      --regs.x;                                                  \
                 SETNZ(regs.x)
#define DEY      --regs.y;                                                  \
                 SETNZ(regs.y)
#define EOR      regs.a ^= READ;                                            \
                 SETNZ(regs.a)
#define INA      ++regs.a;                                                  \
                 SETNZ(regs.a)
#define INC      val = READ+1;                                              \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define INX      ++regs.x;                                                  \
                 SETNZ(regs.x)
#define INY      ++regs.y;                                                  \
                 SETNZ(regs.y)
#define JMP      regs.pc = addr;
#define JSR      --regs.pc;                                                 \
                 PUSH(regs.pc >> 8)                                         \
                 PUSH(regs.pc & 0xFF)                                       \
                 regs.pc = addr;
#define LDA      regs.a = READ;                                             \
                 SETNZ(regs.a)
#define LDX      regs.x = READ;                                             \
                 SETNZ(regs.x)
#define LDY      regs.y = READ;                                             \
                 SETNZ(regs.y)
#define LSR      val   = READ;                                              \
                 flagc = (val & 1);                                         \
                 flagn = 0;                                                 \
                 val >>= 1;                                                 \
                 SETZ(val)                                                  \
                 WRITE(val)
#define LSRA     flagc = (regs.a & 1);                                      \
                 flagn = 0;                                                 \
                 regs.a >>= 1;                                              \
                 SETZ(regs.a)
#define NOP
#define ORA      regs.a |= READ;                                            \
                 SETNZ(regs.a)
#define PHA      PUSH(regs.a)
#define PHP      EF_TO_AF                                                   \
                 regs.ps |= AF_RESERVED;                                    \
                 PUSH(regs.ps)
#define PHX      PUSH(regs.x)
#define PHY      PUSH(regs.y)
#define PLA      regs.a = POP;                                              \
                 SETNZ(regs.a)
#define PLP      regs.ps = POP;                                             \
                 AF_TO_EF
#define PLX      regs.x = POP;                                              \
                 SETNZ(regs.x)
#define PLY      regs.y = POP;                                              \
                 SETNZ(regs.y)
#define ROL      val   = (READ << 1) | (flagc != 0);                        \
                 flagc = (val > 0xFF);                                      \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define ROLA     val    = (((WORD)regs.a) << 1) | (flagc != 0);             \
                 flagc  = (val > 0xFF);                                     \
                 regs.a = val & 0xFF;                                       \
                 SETNZ(regs.a);
#define ROR      temp  = READ;                                              \
                 val   = (temp >> 1) | (flagc ? 0x80 : 0);                  \
                 flagc = temp & 1;                                          \
                 SETNZ(val)                                                 \
                 WRITE(val)
#define RORA     val    = (((WORD)regs.a) >> 1) | (flagc ? 0x80 : 0);       \
                 flagc  = regs.a & 1;                                       \
                 regs.a = val & 0xFF;                                       \
                 SETNZ(regs.a)
#define RTI      regs.ps = POP;                                             \
                 AF_TO_EF                                                   \
                 regs.pc = POP;                                             \
                 regs.pc |= (((WORD)POP) << 8);
#define RTS      regs.pc = POP;                                             \
                 regs.pc |= (((WORD)POP) << 8);                             \
                 ++regs.pc;
#define SBC      temp = READ;                                               \
                 if (regs.ps & AF_DECIMAL) {                                \
					unsigned int p1, p2 = 0;								\
					p1 = (regs.a & 0x0F) - (temp & 0x0F) - (flagc == 0);	\
					if (p1 > 0x09)											\
					{														\
						p1 += 0x0A;											\
						p2 = 0x10;											\
					}														\
					p2 = (regs.a & 0xF0) - (temp & 0xF0) - p2;				\
					if (p2 > 0x90)											\
					{														\
						flagc = 1;											\
						p2 += 0xA0;											\
					}														\
					else													\
						flagc = 0;											\
					regs.a = (p2 & 0xF0) | (p1 & 0x0F);						\
					SETNZ(regs.a);                                         \
                 }                                                          \
                 else {                                                     \
                   val    = regs.a-temp-!flagc;                             \
                   flagc  = (val < 0x8000);                                 \
                   flagv  = (((regs.a & 0x80) != (temp & 0x80)) &&          \
                             ((regs.a & 0x80) != (val & 0x80)));            \
                   regs.a = val & 0xFF;                                     \
                   SETNZ(regs.a);                                           \
                 }                 
#define SEC      flagc = 1;
#define SED      regs.ps |= AF_DECIMAL;
#define SEI      regs.ps |= AF_INTERRUPT;
#define STA      WRITE(regs.a)
#define STX      WRITE(regs.x)
#define STY      WRITE(regs.y)
#define STZ      WRITE(0)
#define TAX      regs.x = regs.a;                                           \
                 SETNZ(regs.x)
#define TAY      regs.y = regs.a;                                           \
                 SETNZ(regs.y)
#define TRB      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 val  &= ~regs.a;                                           \
                 WRITE(val)
#define TSB      val   = READ;                                              \
                 flagz = !(regs.a & val);                                   \
                 val   |= regs.a;                                           \
                 WRITE(val)
#define TSX      regs.x = regs.sp & 0xFF;                                   \
                 SETNZ(regs.x)
#define TXA      regs.a = regs.x;                                           \
                 SETNZ(regs.a)
#define TXS      regs.sp = 0x100 | regs.x;
#define TYA      regs.a = regs.y;                                           \
                 SETNZ(regs.a)
#define INVALID1
#define INVALID2 ++regs.pc;
#define INVALID3 regs.pc += 2;


/****************************************************************************
*
*  OPCODE TABLE
*
***/

//===========================================================================
DWORD InternalCpuExecute (DWORD totalcycles)
{
	WORD addr;
	BOOL flagc;
	BOOL flagi;
	BOOL flagn;
	BOOL flagv;
	BOOL flagz;
	WORD temp;
	WORD val;
	DWORD cycles = 0;

	AF_TO_EF

	do {
#ifdef CPUDEBUG
		fprintf(arquivocpu, "%4X\n", regs.pc);
		fflush(arquivocpu);
#endif
		switch (mem_readb(regs.pc++, 1))
		{
			case 0x00:       BRK           CYC(7)  break;
			case 0x01:       INDX ORA      CYC(6)  break;
			case 0x02:       INVALID2      CYC(2)  break;
			case 0x03:       INVALID2      CYC(1)  break;
			case 0x04:       INVALID2      CYC(5)  break;
			case 0x05:       ZPG ORA       CYC(3)  break;
			case 0x06:       ZPG ASL       CYC(5)  break;
			case 0x07:       INVALID2      CYC(1)  break;
			case 0x08:       PHP           CYC(3)  break;
			case 0x09:       IMM ORA       CYC(2)  break;
			case 0x0A:       ASLA          CYC(2)  break;
			case 0x0B:       INVALID2      CYC(1)  break;
			case 0x0C:       INVALID3      CYC(6)  break;
			case 0x0D:       ABS ORA       CYC(4)  break;
			case 0x0E:       ABS ASL       CYC(6)  break;
			case 0x0F:       INVALID3      CYC(1)  break;
			case 0x10:       REL BPL       CYC(2)  break;
			case 0x11:       INDY ORA      CYC(5)  break;
			case 0x12:       INVALID2      CYC(5)  break;
			case 0x13:       INVALID2      CYC(1)  break;
			case 0x14:       INVALID2      CYC(5)  break;
			case 0x15:       ZPGX ORA      CYC(4)  break;
			case 0x16:       ZPGX ASL      CYC(6)  break;
			case 0x17:       INVALID2      CYC(1)  break;
			case 0x18:       CLC           CYC(2)  break;
			case 0x19:       ABSY ORA      CYC(4)  break;
			case 0x1A:       INVALID1      CYC(2)  break;
			case 0x1B:       INVALID3      CYC(1)  break;
			case 0x1C:       INVALID3      CYC(6)  break;
			case 0x1D:       ABSX ORA      CYC(4)  break;
			case 0x1E:       ABSX ASL      CYC(6)  break;
			case 0x1F:       INVALID3      CYC(1)  break;
			case 0x20:       ABS JSR       CYC(6)  break;
			case 0x21:       INDX AND      CYC(6)  break;
			case 0x22:       INVALID2      CYC(2)  break;
			case 0x23:       INVALID2      CYC(1)  break;
			case 0x24:       ZPG BIT       CYC(3)  break;
			case 0x25:       ZPG AND       CYC(3)  break;
			case 0x26:       ZPG ROL       CYC(5)  break;
			case 0x27:       INVALID2      CYC(1)  break;
			case 0x28:       PLP           CYC(4)  break;
			case 0x29:       IMM AND       CYC(2)  break;
			case 0x2A:       ROLA          CYC(2)  break;
			case 0x2B:       INVALID2      CYC(1)  break;
			case 0x2C:       ABS BIT       CYC(4)  break;
			case 0x2D:       ABS AND       CYC(2)  break;
			case 0x2E:       ABS ROL       CYC(6)  break;
			case 0x2F:       INVALID3      CYC(1)  break;
			case 0x30:       REL BMI       CYC(2)  break;
			case 0x31:       INDY AND      CYC(5)  break;
			case 0x32:       INVALID2      CYC(5)  break;
			case 0x33:       INVALID2      CYC(1)  break;
			case 0x34:       INVALID2      CYC(4)  break;
			case 0x35:       ZPGX AND      CYC(4)  break;
			case 0x36:       ZPGX ROL      CYC(6)  break;
			case 0x37:       INVALID2      CYC(1)  break;
			case 0x38:       SEC           CYC(2)  break;
			case 0x39:       ABSY AND      CYC(4)  break;
			case 0x3A:       INVALID1      CYC(2)  break;
			case 0x3B:       INVALID3      CYC(1)  break;
			case 0x3C:       INVALID3      CYC(4)  break;
			case 0x3D:       ABSX AND      CYC(4)  break;
			case 0x3E:       ABSX ROL      CYC(6)  break;
			case 0x3F:       INVALID3      CYC(1)  break;
			case 0x40:       RTI           CYC(6)  break;
			case 0x41:       INDX EOR      CYC(6)  break;
			case 0x42:       INVALID2      CYC(2)  break;
			case 0x43:       INVALID2      CYC(1)  break;
			case 0x44:       INVALID2      CYC(3)  break;
			case 0x45:       ZPG EOR       CYC(3)  break;
			case 0x46:       ZPG LSR       CYC(5)  break;
			case 0x47:       INVALID2      CYC(1)  break;
			case 0x48:       PHA           CYC(3)  break;
			case 0x49:       IMM EOR       CYC(2)  break;
			case 0x4A:       LSRA          CYC(2)  break;
			case 0x4B:       INVALID3      CYC(1)  break;
			case 0x4C:       ABS JMP       CYC(3)  break;
			case 0x4D:       ABS EOR       CYC(4)  break;
			case 0x4E:       ABS LSR       CYC(6)  break;
			case 0x4F:       INVALID3      CYC(1)  break;
			case 0x50:       REL BVC       CYC(2)  break;
			case 0x51:       INDY EOR      CYC(5)  break;
			case 0x52:       INVALID2      CYC(5)  break;
			case 0x53:       INVALID2      CYC(1)  break;
			case 0x54:       INVALID2      CYC(4)  break;
			case 0x55:       ZPGX EOR      CYC(4)  break;
			case 0x56:       ZPGX LSR      CYC(6)  break;
			case 0x57:       INVALID2      CYC(1)  break;
			case 0x58:       CLI           CYC(2)  break;
			case 0x59:       ABSY EOR      CYC(4)  break;
			case 0x5A:       INVALID1      CYC(3)  break;
			case 0x5B:       INVALID3      CYC(1)  break;
			case 0x5C:       INVALID3      CYC(8)  break;
			case 0x5D:       ABSX EOR      CYC(4)  break;
			case 0x5E:       ABSX LSR      CYC(6)  break;
			case 0x5F:       INVALID3      CYC(1)  break;
			case 0x60:       RTS           CYC(6)  break;
			case 0x61:       INDX ADC      CYC(6)  break;
			case 0x62:       INVALID2      CYC(2)  break;
			case 0x63:       INVALID2      CYC(1)  break;
			case 0x64:       INVALID2      CYC(3)  break;
			case 0x65:       ZPG ADC       CYC(3)  break;
			case 0x66:       ZPG ROR       CYC(5)  break;
			case 0x67:       INVALID2      CYC(1)  break;
			case 0x68:       PLA           CYC(4)  break;
			case 0x69:       IMM ADC       CYC(2)  break;
			case 0x6A:       RORA          CYC(2)  break;
			case 0x6B:       INVALID3      CYC(1)  break;
			case 0x6C:       IABS JMP      CYC(6)  break;
			case 0x6D:       ABS ADC       CYC(4)  break;
			case 0x6E:       ABS ROR       CYC(6)  break;
			case 0x6F:       INVALID3      CYC(1)  break;
			case 0x70:       REL BVS       CYC(2)  break;
			case 0x71:       INDY ADC      CYC(5)  break;
			case 0x72:       INVALID2      CYC(5)  break;
			case 0x73:       INVALID2      CYC(1)  break;
			case 0x74:       INVALID2      CYC(4)  break;
			case 0x75:       ZPGX ADC      CYC(4)  break;
			case 0x76:       ZPGX ROR      CYC(6)  break;
			case 0x77:       INVALID2      CYC(1)  break;
			case 0x78:       SEI           CYC(2)  break;
			case 0x79:       ABSY ADC      CYC(4)  break;
			case 0x7A:       INVALID1      CYC(4)  break;
			case 0x7B:       INVALID3      CYC(1)  break;
			case 0x7C:       INVALID3      CYC(6)  break;
			case 0x7D:       ABSX ADC      CYC(4)  break;
			case 0x7E:       ABSX ROR      CYC(6)  break;
			case 0x7F:       INVALID3      CYC(1)  break;
			case 0x80:       INVALID2      CYC(3)  break;
			case 0x81:       INDX STA      CYC(6)  break;
			case 0x82:       INVALID2      CYC(2)  break;
			case 0x83:       INVALID2      CYC(1)  break;
			case 0x84:       ZPG STY       CYC(3)  break;
			case 0x85:       ZPG STA       CYC(3)  break;
			case 0x86:       ZPG STX       CYC(3)  break;
			case 0x87:       INVALID2      CYC(1)  break;
			case 0x88:       DEY           CYC(2)  break;
			case 0x89:       IMM BITI      CYC(2)  break;
			case 0x8A:       TXA           CYC(2)  break;
			case 0x8B:       INVALID3      CYC(1)  break;
			case 0x8C:       ABS STY       CYC(4)  break;
			case 0x8D:       ABS STA       CYC(4)  break;
			case 0x8E:       ABS STX       CYC(4)  break;
			case 0x8F:       INVALID3      CYC(1)  break;
			case 0x90:       REL BCC       CYC(2)  break;
			case 0x91:       INDY STA      CYC(6)  break;
			case 0x92:       INVALID2      CYC(5)  break;
			case 0x93:       INVALID2      CYC(1)  break;
			case 0x94:       ZPGX STY      CYC(4)  break;
			case 0x95:       ZPGX STA      CYC(4)  break;
			case 0x96:       ZPGY STX      CYC(4)  break;
			case 0x97:       INVALID2      CYC(1)  break;
			case 0x98:       TYA           CYC(2)  break;
			case 0x99:       ABSY STA      CYC(5)  break;
			case 0x9A:       TXS           CYC(2)  break;
			case 0x9B:       INVALID3      CYC(1)  break;
			case 0x9C:       INVALID3      CYC(4)  break;
			case 0x9D:       ABSX STA      CYC(5)  break;
			case 0x9E:       INVALID3      CYC(5)  break;
			case 0x9F:       INVALID3      CYC(1)  break;
			case 0xA0:       IMM LDY       CYC(2)  break;
			case 0xA1:       INDX LDA      CYC(6)  break;
			case 0xA2:       IMM LDX       CYC(2)  break;
			case 0xA3:       INVALID2      CYC(1)  break;
			case 0xA4:       ZPG LDY       CYC(3)  break;
			case 0xA5:       ZPG LDA       CYC(3)  break;
			case 0xA6:       ZPG LDX       CYC(3)  break;
			case 0xA7:       INVALID2      CYC(1)  break;
			case 0xA8:       TAY           CYC(2)  break;
			case 0xA9:       IMM LDA       CYC(2)  break;
			case 0xAA:       TAX           CYC(2)  break;
			case 0xAB:       INVALID3      CYC(1)  break;
			case 0xAC:       ABS LDY       CYC(4)  break;
			case 0xAD:       ABS LDA       CYC(4)  break;
			case 0xAE:       ABS LDX       CYC(4)  break;
			case 0xAF:       INVALID3      CYC(1)  break;
			case 0xB0:       REL BCS       CYC(2)  break;
			case 0xB1:       INDY LDA      CYC(5)  break;
			case 0xB2:       INVALID2      CYC(5)  break;
			case 0xB3:       INVALID2      CYC(1)  break;
			case 0xB4:       ZPGX LDY      CYC(4)  break;
			case 0xB5:       ZPGX LDA      CYC(4)  break;
			case 0xB6:       ZPGY LDX      CYC(4)  break;
			case 0xB7:       INVALID2      CYC(1)  break;
			case 0xB8:       CLV           CYC(2)  break;
			case 0xB9:       ABSY LDA      CYC(4)  break;
			case 0xBA:       TSX           CYC(2)  break;
			case 0xBB:       INVALID3      CYC(1)  break;
			case 0xBC:       ABSX LDY      CYC(4)  break;
			case 0xBD:       ABSX LDA      CYC(4)  break;
			case 0xBE:       ABSY LDX      CYC(4)  break;
			case 0xBF:       INVALID3      CYC(1)  break;
			case 0xC0:       IMM CPY       CYC(2)  break;
			case 0xC1:       INDX CMP      CYC(6)  break;
			case 0xC2:       INVALID2      CYC(2)  break;
			case 0xC3:       INVALID2      CYC(1)  break;
			case 0xC4:       ZPG CPY       CYC(3)  break;
			case 0xC5:       ZPG CMP       CYC(3)  break;
			case 0xC6:       ZPG DEC       CYC(5)  break;
			case 0xC7:       INVALID2      CYC(1)  break;
			case 0xC8:       INY           CYC(2)  break;
			case 0xC9:       IMM CMP       CYC(2)  break;
			case 0xCA:       DEX           CYC(2)  break;
			case 0xCB:       INVALID3      CYC(1)  break;
			case 0xCC:       ABS CPY       CYC(4)  break;
			case 0xCD:       ABS CMP       CYC(4)  break;
			case 0xCE:       ABS DEC       CYC(5)  break;
			case 0xCF:       INVALID3      CYC(1)  break;
			case 0xD0:       REL BNE       CYC(2)  break;
			case 0xD1:       INDY CMP      CYC(5)  break;
			case 0xD2:       INVALID2      CYC(5)  break;
			case 0xD3:       INVALID2      CYC(1)  break;
			case 0xD4:       INVALID2      CYC(4)  break;
			case 0xD5:       ZPGX CMP      CYC(4)  break;
			case 0xD6:       ZPGX DEC      CYC(6)  break;
			case 0xD7:       INVALID2      CYC(1)  break;
			case 0xD8:       CLD           CYC(2)  break;
			case 0xD9:       ABSY CMP      CYC(4)  break;
			case 0xDA:       INVALID1      CYC(3)  break;
			case 0xDB:       INVALID3      CYC(1)  break;
			case 0xDC:       INVALID3      CYC(4)  break;
			case 0xDD:       ABSX CMP      CYC(4)  break;
			case 0xDE:       ABSX DEC      CYC(6)  break;
			case 0xDF:       INVALID3      CYC(1)  break;
			case 0xE0:       IMM CPX       CYC(2)  break;
			case 0xE1:       INDX SBC      CYC(6)  break;
			case 0xE2:       INVALID2      CYC(2)  break;
			case 0xE3:       INVALID2      CYC(1)  break;
			case 0xE4:       ZPG CPX       CYC(3)  break;
			case 0xE5:       ZPG SBC       CYC(3)  break;
			case 0xE6:       ZPG INC       CYC(5)  break;
			case 0xE7:       INVALID2      CYC(1)  break;
			case 0xE8:       INX           CYC(2)  break;
			case 0xE9:       IMM SBC       CYC(2)  break;
			case 0xEA:       NOP           CYC(2)  break;
			case 0xEB:       INVALID3      CYC(1)  break;
			case 0xEC:       ABS CPX       CYC(4)  break;
			case 0xED:       ABS SBC       CYC(4)  break;
			case 0xEE:       ABS INC       CYC(6)  break;
			case 0xEF:       INVALID3      CYC(1)  break;
			case 0xF0:       REL BEQ       CYC(2)  break;
			case 0xF1:       INDY SBC      CYC(5)  break;
			case 0xF2:       INVALID2      CYC(5)  break;
			case 0xF3:       INVALID2      CYC(1)  break;
			case 0xF4:       INVALID2      CYC(4)  break;
			case 0xF5:       ZPGX SBC      CYC(4)  break;
			case 0xF6:       ZPGX INC      CYC(6)  break;
			case 0xF7:       INVALID2      CYC(1)  break;
			case 0xF8:       SED           CYC(2)  break;
			case 0xF9:       ABSY SBC      CYC(4)  break;
			case 0xFA:       INVALID1      CYC(4)  break;
			case 0xFB:       INVALID3      CYC(1)  break;
			case 0xFC:       INVALID3      CYC(4)  break;
			case 0xFD:       ABSX SBC      CYC(4)  break;
			case 0xFE:       ABSX INC      CYC(6)  break;
			case 0xFF:       INVALID3      CYC(1)  break;
		}
		if (flagsint & INT_NMI)
		{
			flagsint &= ~INT_NMI;
			PUSH(regs.pc >> 8)
			PUSH(regs.pc & 0xFF)
			EF_TO_AF
			PUSH(regs.ps)
			regs.pc = mem_readw(0xFFFA, 1);
			CYC(6)
		}
		if ((flagsint & INT_IRQ) && (regs.ps & AF_INTERRUPT))
		{
			flagsint &= ~INT_IRQ;
			PUSH(regs.pc >> 8)
			PUSH(regs.pc & 0xFF)
			EF_TO_AF
			PUSH(regs.ps)
			regs.ps &= ~AF_INTERRUPT;
			regs.pc = mem_readw(0xFFFE, 1);
			CYC(6)
		}
	}
	while (cycles < totalcycles);
  
	EF_TO_AF
	return cycles;
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void CpuDestroy ()
{
	//
}

//===========================================================================
DWORD CpuExecute (DWORD cycles)
{
	static BOOL laststep = 0;
	DWORD result = 0;

	// IF WE ARE SINGLE STEPPING, USING THE INTERPRETIVE EMULATOR
	if (!cycles)
	{
		laststep = 1;
		result=InternalCpuExecute(0);
	}

	// OTHERWISE, USE THE CURRENT EMULATOR.  IF WE HAVE BEEN REQUESTED TO
	// EXECUTE MORE THAN 65535 CYCLES, BREAK THE REQUEST INTO MULTIPLE
	// 65535-CYCLE CHUNKS, BECAUSE SOME OF THE EXTERNAL EMULATORS KEEP TRACK
	// OF THE CYCLE COUNT USING ONLY 16 BITS.
	else
	{
		if (laststep)
		{
			laststep = 0;
		}
		if (cycles <= 0xFFFF)
			result=InternalCpuExecute(cycles);
		else
		{
			do
			{
				DWORD cyclestoexec = MIN(65535,cycles);
				result += InternalCpuExecute(cyclestoexec);
				cycles -= cyclestoexec;
			}
			while (cycles);
		}
	}
	return result;
}

//===========================================================================
void CpuIRQ()
{
  flagsint |= INT_IRQ;
}

//===========================================================================
void CpuNMI()
{
  flagsint |= INT_NMI;
}

//===========================================================================
void CpuInitialize()
{
	CpuDestroy();

	regs.a  = 0;
	regs.x  = 0;
	regs.y  = 0;
	regs.ps = 0x20;
	regs.pc = mem_readw(0xFFFC, 1);
	regs.sp = 0x01FF;
}

//===========================================================================
void CpuReinitialize ()
{
	//
}

// EOF