#include "spimcore.h"

//Mask
//[31:26]1111 1100 0000 0000 0000 0000 0000 0000
#define OP_MASK     0xFC000000
//[25:21]0000 0011 1110 0000 0000 0000 0000 0000
#define R1_MASK     0x03E00000
//[20:16]0000 0000 0001 1111 0000 0000 0000 0000
#define R2_MASK     0x001F0000
//[15:11]0000 0000 0000 0000 1111 1000 0000 0000
#define R3_MASK     0x0000F800
//[5:0]0000 0000 0000 0000 0000 0000 0011 1111
#define FUNCT_MASK  0x0000003F
//[15:0]0000 0000 0000 0000 1111 1111 1111 1111
#define OFFSET_MASK 0x0000FFFF
//[25:0]0000 0011 1111 1111 1111 1111 1111 1111
#define JSEC_MASK   0x03FFFFFF 

//Op code
//R-type 000000
#define R_FORMAT_OP 0x00
//Lw 100011
#define LW_OP       0x23
//Sw 101011
#define SW_OP 		0x2B
//Beq 000100
#define BEQ_OP 		0x04
//Addi 001000
#define ADDI_OP 	0x08
//Lui 001111
#define LUI_OP 		0x0F
//Slti 001010
#define SLTI_OP 	0x0A
//Sltiu 001011
#define SLTIU_OP 	0x0B
//J 000010
#define JUMP_OP 	0x02

//Extension
#define POSITIVEEXTENSION 0x0000FFFF
#define NEGATIVEEXTENSION 0xFFFF0000

//Function code
#define FUNCT_SLTU 0x2B
#define FUNCT_SLT  0x2A
#define FUNCT_ADD  0x20
#define FUNCT_SUB  0x22
#define FUNCT_AND  0x24
#define FUNCT_OR   0x25

/* ALU */
/* 10 Points */
//A ALU without overflow???
//Since we do not care about overflow, we can simply convert A and B to 
//integer for the camparation in slt unsigned.
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
	switch(ALUControl){
		case 0:
			*ALUresult = A + B;
			break;
		case 1:
			*ALUresult = A - B;
			break;
		//What is the difference between the usigned A, B and A, B?
		//A, B is not guaranteed to be signed or unsigned.
		case 2:
			*ALUresult = (int)A<(int)B?1:0;
			break;
		case 3:
			*ALUresult = A<B?1:0;
			break;
		case 4:
			*ALUresult = A & B;
			break;
		case 5:
			*ALUresult = A | B;
			break;
		//What is the meaning that just shift B itself by 16 bits? 
		//It doesn't change anything. I guess it needs to be store in the result.
		//Otherwise, Zero will not be guaranteed.
		case 6:
			*ALUresult = B << 16;
			break;
		case 7:
			*ALUresult = ~A;
	}
	if(*ALUresult == 0)
		*Zero = 1;
	else
		*Zero = 0;
	//printf("ALU check\n");
}

/* instruction fetch */
/* 10 Points */
//Get instruction address from PC, find instruction in the Mem, write it to *instruction
//Check aligned first, 
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
	//Deal with word-aligned
	if(PC % 4 != 0) return 1;
	*instruction = Mem[PC>>2];
	//printf("instruction_fetch check, instruction:%x PC>>2:%x\n",*instruction, PC>>2);
	return 0;
}


/* instruction partition */
/* 10 Points */
//Use mask and shift to get each part of the instruction
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
	*op = (instruction & OP_MASK) >> 26;
	*r1 = (instruction & R1_MASK) >> 21;
	*r2 = (instruction & R2_MASK) >> 16;
	*r3 = (instruction & R3_MASK) >> 11;
	*funct = (instruction & FUNCT_MASK);
	*offset = (instruction & OFFSET_MASK);
	*jsec = (instruction & JSEC_MASK);
	//printf("instruction_partition check, op:%x r1:%x r2:%x r3:%x funct:%x offset:%x jsec:%x\n",*op,*r1,*r2,*r3,*funct,*offset,*jsec);

}



/* instruction decode */
/* 15 Points */
//Implement the Control logic with 6 bits op code input
int instruction_decode(unsigned op,struct_controls *controls)
{
	switch(op){
		case R_FORMAT_OP:
			controls->RegDst = 1;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 0;
			//R-type
			controls->ALUOp = 7;
			controls->MemWrite = 0;
			controls->ALUSrc = 0;
			controls->RegWrite = 1;
			break;
		case LW_OP:
			controls->RegDst = 0;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 1;
			controls->MemtoReg = 1;
			//ALU do add
			controls->ALUOp = 0;
			controls->MemWrite = 0;
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			break;
		case SW_OP:
			controls->RegDst = 2;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 2;
			//ALU do add
			controls->ALUOp = 0;
			controls->MemWrite = 1;
			controls->ALUSrc = 1;
			controls->RegWrite = 0;
			break;
		case BEQ_OP:
			controls->RegDst = 2;
			controls->Jump = 0;
			controls->Branch = 1;
			controls->MemRead = 0;
			controls->MemtoReg = 2;
			//ALU do sub
			controls->ALUOp = 1;
			controls->MemWrite = 0;
			controls->ALUSrc = 0;
			controls->RegWrite = 0;
			break;
		case ADDI_OP:
			controls->RegDst = 0;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 0;
			//ALU do add
			controls->ALUOp = 0;
			controls->MemWrite = 0;
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			break;
		case LUI_OP:
			controls->RegDst = 0;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 0;
			//ALU do shift left extension
			controls->ALUOp = 6;
			controls->MemWrite = 0;
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			break;
		case SLTI_OP:
			controls->RegDst = 0;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 0;
			//ALU do set less than immediate
			controls->ALUOp = 2;
			controls->MemWrite = 0;
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			break;
		case SLTIU_OP:
			controls->RegDst = 0;
			controls->Jump = 0;
			controls->Branch = 0;
			controls->MemRead = 0;
			controls->MemtoReg = 0;
			//ALU do sub
			controls->ALUOp = 3;
			controls->MemWrite = 0;
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			break;
		case JUMP_OP:
			controls->RegDst = 2;
			controls->Jump = 1;
			controls->Branch = 2;
			controls->MemRead = 0;
			controls->MemtoReg = 2;
			//ALU do don't care
			controls->ALUOp = 0;
			controls->MemWrite = 0;
			controls->ALUSrc = 2;
			controls->RegWrite = 0;
			break;
		//halt case
		default:
			return 1;
	}
	//Valid case
	/*
	printf("instruction_decode check, %x%x%x%x%x%x%x%x%x\n",controls->RegDst,controls->Jump,
		controls->Branch,controls->MemRead,controls->MemtoReg,controls->ALUOp,controls->MemWrite,
		controls->ALUSrc,controls->RegWrite);
	*/
	return 0;
}

/* Read Register */
/* 5 Points */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
	*data1 = Reg[r1];
	*data2 = Reg[r2];
	//printf("read_register check, data1:%x r1:%x data2:%x r2:%x\n",*data1,r1,*data2,r2);

}


/* Sign Extend */
/* 10 Points */
void sign_extend(unsigned offset,unsigned *extended_value)
{
	if((offset>>15) == 1){
		*extended_value = offset | NEGATIVEEXTENSION;
	}else{
		*extended_value = offset & POSITIVEEXTENSION;
	}
	//printf("sign_extend check, extended_value:%x\n",*extended_value);

}

/* ALU operations */
/* 10 Points */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
	unsigned inputData1 = data1;
	unsigned inputData2 = (ALUSrc == 0) ? data2 : extended_value;
	char inputALUOp;
	//not R-type
	if(ALUOp != 7){
		inputALUOp = ALUOp;
	}
	//R-type
	else{
		switch(funct){
			case FUNCT_ADD:
				inputALUOp = 0;
				break;
			case FUNCT_SUB:
				inputALUOp = 1;
				break;
			case FUNCT_AND:
				inputALUOp = 4;
				break;
			case FUNCT_OR:
				inputALUOp = 5;
				break;
			case FUNCT_SLT:
				inputALUOp = 2;
				break;
			case FUNCT_SLTU:
				inputALUOp = 3;
				break;
			default:
				return 1;
		}
	}
	//printf("ALU function pick check, inputALUOp:%x\n",inputALUOp);
	ALU(inputData1, inputData2, inputALUOp, ALUresult, Zero);
	//printf("ALU_operations check, ALUresult:%x\n",*ALUresult);
	return 0;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
	if(MemWrite == 1 && MemRead == 0){
		Mem[ALUresult>>2] = data2;
		//printf("rw_memory check write, ALUresult>>2:%x\n", ALUresult>>2);
		return 0;
	}
	if(MemWrite == 0 && MemRead == 1){
		*memdata = Mem[ALUresult>>2];
		//printf("rw_memory check read, memdata:%x ALUresult>>2:%x\n",*memdata,ALUresult>>2);
		return 0;
	}
	if(MemWrite == 0 && MemRead == 0){
		//printf("rw_memory check do nothing\n");
		return 0;
	}
	return 1;
}


/* Write Register */
/* 10 Points */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
	unsigned inputData;
	unsigned RegisterDestination;
	if(RegWrite == 1){
		inputData = (MemtoReg == 0) ? ALUresult : memdata;
		RegisterDestination = (RegDst == 0) ? r2 : r3;
		Reg[RegisterDestination] = inputData;
	}
	//printf("write_register check\n");

}

/* PC update */
/* 10 Points */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
	*PC = *PC + 4;
	if(Jump == 1){
		*PC = (jsec<<2) | (*PC & 0xF0000000);
	}
	if(Zero == 1 && Branch == 1){
		*PC = *PC + (extended_value << 2);
	}
	//printf("PC_update check\n");
}

