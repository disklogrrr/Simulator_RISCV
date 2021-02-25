#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// define TRUE = 1 , FALSE = 0 
#define TRUE 1
#define FALSE 0

// set mode to use ChangePC  [ default = 0  jal = 1 ,  jalr = 2  , beq = 3 ] 
int mode = 0;

// instruction going to do 
unsigned long Instruction;

// output, instruction variable
long output, address, funct7, rs2, rs1, funct3, rd, opcode, immediate;
// sd , jal , beq  instrcution variable
long imm11_5, imm4_0;
long imm10_1, imm20, imm11, imm19_12;
long imm12, imm10_5, imm4_1, imm11;



//clock cycles
long long cycles = 0;

// registers
long long int regs[32];

// program counter
unsigned long pc = 0;

// memory
#define INST_MEM_SIZE 32*1024
#define DATA_MEM_SIZE 32*1024
unsigned long inst_mem[INST_MEM_SIZE]; //instruction memory
unsigned long long data_mem[DATA_MEM_SIZE]; //data memory

//misc. function
int init(char* filename);

void fetch();//fetch an instruction from a instruction memory
void decode();//decode the instruction and read data from register file
void exe();//perform the appropriate operation 
void mem();//access the data memory
void wb();//write result of arithmetic operation or data read from the data memory if required

void print_cycles();
void print_reg();
void print_pc();

// add ChangePC() function to update PC 
void ChangePC();

void ChangePC()
{
	// PC add 4
	if (mode == 0)
		pc += 4;

	// rd = PC + 4; go to PC + immediate 
	else if (mode == 1) {
		if (rd == 0)
			pc += address;

		else if (imm20 == 1) {
			output = pc + 4;
			pc -= address;
		}

		else {
			output = pc + 4;
			pc += address;
		}
	}

	// rd = PC + 4; go to rs1 + immediate
	else if (mode == 2) {

		if (rd == 0) //rd == 0 , no save and return
			pc = regs[rs1];

		else {
			output = pc + 4;
			pc += address;
		}
	}

	// if branch , pc = pc + imediate
	else if (mode == 3)
		pc += (immediate << 1);
}

void fetch()
{
	int order = 0;
	order = pc / 4;
	Instruction = inst_mem[order];   // get inst_mem order and instruction stack push 
}

void decode()
{
	opcode = Instruction & 0b1111111;	// masking 01111111 to get left(last) 3bits
	funct7 = Instruction >> 25;	// shift 25 to get right(first) 7bits
	funct3 = 0b111 & (Instruction >> 12); // shift 12 and masking 0111 to get left 3bits

	// add 
	if (opcode == 0b110011 && funct7 == 0b0 && funct3 == 0b0) {
		mode = 0;

		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		rs2 = (Instruction >> 20) & 0b11111; // shift 20 and masking 0001 1111 to get 5 bits
		ChangePC();
	}
	// addi
	else if (opcode == 0b10011 && funct3 == 0b0) {
		mode = 0;

		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		immediate = Instruction >> 20;	// shift 20 to get right 12 bits
		ChangePC();
	}
	// ld
	else if (opcode == 0b11 && funct3 == 0b11) {
		mode = 0;

		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		immediate = Instruction >> 20; // shift 20 to get right 12 bits
		ChangePC();
	}
	// sd
	else if (opcode == 0b100011 && funct3 == 0b11) {
		mode = 0;

		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		rs2 = (Instruction >> 20) & 0b11111; // shift 20 and masking 0001 1111 to get 5 bits

		imm11_5 = funct7;
		imm4_0 = rd;
		ChangePC();
	}
	// jal 
	else if (opcode == 0b1101111) {
		immediate = Instruction >> 12;
		rd = (Instruction >> 7) & 0b11111;

		imm10_1 = (immediate >> 9) & 0b1111111111; // immediate seperating
		imm11 = (immediate >> 8) & 0b1;
		imm19_12 = immediate & 0b11111111;
		imm20 = immediate >> 19;
	}
	// jalr 
	else if (opcode == 0b1100111 && funct3 == 0b0) {
		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		immediate = Instruction >> 20;	// shift 20 to get right 12 bits
	}
	// beq 
	else if (opcode == 0b1100011 && funct3 == 0b0) {
		rd = (Instruction >> 7) & 0b11111; // shift 7 and masking 0001 1111 to get 5 bits
		rs1 = (Instruction >> 15) & 0b11111; // shift 15 and masking 0001 1111 to get 5 bits
		rs2 = (Instruction >> 20) & 0b11111; // shift 20 and masking 0001 1111 to get 5 bits

		imm4_1 = (rd >> 1) & 0b1111; // //immediate seprating
		imm10_5 = funct7 & 0b111111;
		imm11 = rd & 0b1;
		imm12 = funct7 >> 6;
	}
}

void exe()
{
	// add
	if (opcode == 0b110011 && funct7 == 0b0 && funct3 == 0b0) {
		output = regs[rs1] + regs[rs2]; // output = rs1 + rs2
	}
	//addi
	else if (opcode == 0b10011 && funct3 == 0b0) {
		if ((immediate & 0b100000000000) == 0b100000000000) //if immediate = negative number , add 2's complement
			immediate = (0b1000000000000 - immediate) * -1;
		output = regs[rs1] + immediate; // output = rs1 + immediate
	}
	//ld
	else if (opcode == 0b11 && funct3 == 0b11) {
		address = regs[rs1] + immediate; // address = rs1 + immediate
	}
	//sd
	else if (opcode == 0b100011 && funct3 == 0b11) {
		immediate = imm4_0 + (imm11_5 << 4);
		address = regs[rs1] + immediate;
	}
	//jal
	else if (opcode == 0b1101111) {
		mode = 1;
		// address = immediate
		if (imm20 == 1) // If negative number  , make 2's complement
			address = (0b100000000000000000000 - ((imm20 << 19) + (imm19_12 << 11) + (imm11 << 10) + imm10_1)) << 1;
		else
			address = ((imm20 << 19) + (imm19_12 << 11) + (imm11 << 10) + imm10_1) << 1;
		ChangePC();
	}
	//jalr
	else if (opcode == 0b1100111 && funct3 == 0b0) {
		mode = 2;
		address = regs[rs1] + immediate << 1; // address = rs1 + immediate
		ChangePC();
	}
	//beq
	else if (opcode == 0b1100011 && funct3 == 0b0) {
		if (regs[rs1] - regs[rs2] == 0) {
			mode = 3;
			if (imm12 == 1) { // if immediate is negative number 
				immediate = 0b1000000000000 - ((imm12 << 11) + (imm11 << 10) + (imm10_5 << 4) + imm4_1);
				immediate = immediate * -1;
			}
			else {
				immediate = (imm12 << 11) + (imm11 << 10) + (imm10_5 << 4) + imm4_1;
			}
			ChangePC();
		}
		else {
			mode = 0;
			ChangePC();
		}
	}
}

void mem()
{
	// Instruction ld and sd 
	if (opcode == 0b11 && funct3 == 0b11) // ld
		output = data_mem[address]; // reading data from data memory ;

	else if (opcode == 0b100011 && funct3 == 0b11) // sd
		data_mem[address] = regs[rs2]; // wrting data to reg 
}

void wb()
{
	//sd
	if (opcode == 0b100011 && funct3 == 0b11) {
		return;
	}

	//ld
	else if (opcode == 0b11 && funct3 == 0b11) {
		if (rd == 0)
			return;
		else
			regs[rd] = output;
	}

	//add
	else if (opcode == 0b110011 && funct7 == 0b0 && funct3 == 0b0) {
		if (rd == 0)
			return;
		else
			regs[rd] = output;
	}

	//addi
	else if (opcode == 0b10011 && funct3 == 0b0) {
		if (rd == 0)
			return;
		else
			regs[rd] = output;
	}

	//jal
	else if (opcode == 0b1101111) {
		if (rd == 0)
			return;
		else
			regs[rd] = output;
	}

	//jalr
	else if (opcode == 0b1100111 && funct3 == 0b0) {
		if (rd == 0)
			return;
		else
			regs[rd] = output;
	}

	//beq
	else if (opcode == 0b1100011 && funct3 == 0b0) {
		return;
	}
}

int main(int ac, char* av[])
{

	if (ac < 3)
	{
		printf("./mips_sim filename mode\n");
		return -1;
	}


	char done = FALSE;
	if (init(av[1]) != 0)
		return -1;
	while (!done)
	{

		fetch();
		decode();
		exe();
		mem();
		wb();


		cycles++;    //increase clock cycle

		//if debug mode, print clock cycle, pc, reg 
		if (*av[2] == '0') {
			print_cycles();  //print clock cycles
			print_pc();		 //print pc
			print_reg();	 //print registers
		}

		// check the exit condition, do not delete!! 
		if (regs[9] == 10)  //if value in $t1 is 10, finish the simulation
			done = TRUE;
	}

	if (*av[2] == '1')
	{
		print_cycles();  //print clock cycles
		print_pc();		 //print pc
		print_reg();	 //print registers
	}

	return 0;
}


/* initialize all datapat elements
//fill the instruction and data memory
//reset the registers
*/
int init(char* filename)
{
	FILE* fp = fopen(filename, "r");
	int i;
	long inst;

	if (fp == NULL)
	{
		fprintf(stderr, "Error opening file.\n");
		return -1;
	}

	/* fill instruction memory */
	i = 0;
	while (fscanf(fp, "%lx", &inst) == 1)
	{
		inst_mem[i++] = inst;
	}


	/*reset the registers*/
	for (i = 0; i < 32; i++)
	{
		regs[i] = 0;
	}

	/*reset pc*/
	pc = 0;
	/*reset clock cycles*/
	cycles = 0;
	return 0;
}

void print_cycles()
{
	printf("---------------------------------------------------\n");

	printf("Clock cycles = %lld\n", cycles);
}

void print_pc()
{
	printf("PC	   = %ld\n\n", pc);
}

void print_reg()
{
	printf("x0   = %d\n", regs[0]);
	printf("x1   = %d\n", regs[1]);
	printf("x2   = %d\n", regs[2]);
	printf("x3   = %d\n", regs[3]);
	printf("x4   = %d\n", regs[4]);
	printf("x5   = %d\n", regs[5]);
	printf("x6   = %d\n", regs[6]);
	printf("x7   = %d\n", regs[7]);
	printf("x8   = %d\n", regs[8]);
	printf("x9   = %d\n", regs[9]);
	printf("x10  = %d\n", regs[10]);
	printf("x11  = %d\n", regs[11]);
	printf("x12  = %d\n", regs[12]);
	printf("x13  = %d\n", regs[13]);
	printf("x14  = %d\n", regs[14]);
	printf("x15  = %d\n", regs[15]);
	printf("x16  = %d\n", regs[16]);
	printf("x17  = %d\n", regs[17]);
	printf("x18  = %d\n", regs[18]);
	printf("x19  = %d\n", regs[19]);
	printf("x20  = %d\n", regs[20]);
	printf("x21  = %d\n", regs[21]);
	printf("x22  = %d\n", regs[22]);
	printf("x23  = %d\n", regs[23]);
	printf("x24  = %d\n", regs[24]);
	printf("x25  = %d\n", regs[25]);
	printf("x26  = %d\n", regs[26]);
	printf("x27  = %d\n", regs[27]);
	printf("x28  = %d\n", regs[28]);
	printf("x29  = %d\n", regs[29]);
	printf("x30  = %d\n", regs[30]);
	printf("x31  = %d\n", regs[31]);
	printf("\n");
}
