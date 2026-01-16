#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEM_SIZE 8192
#define SP_INITIAL 0x2000

typedef struct {
    uint16_t regs[16];   // R0-R15 (R14=SP, R15=PC)
    uint16_t memory[MEM_SIZE];
    uint8_t flag_z;
    uint8_t flag_c;
    bool accessed[MEM_SIZE];
} cpu_t;

#define SP (cpu->regs[14])
#define PC (cpu->regs[15])

//Decodificação
#define GET_OPCODE(ir)   ((ir) & 0x000F)
#define GET_RN(ir)       (((ir) >> 4)  & 0x000F)
#define GET_RM(ir)       (((ir) >> 8)  & 0x000F)
#define GET_RD(ir)       (((ir) >> 12) & 0x000F)
#define GET_IM(ir)       (((ir) >> 4)  & 0x00FF)   

uint16_t mem_read(cpu_t *cpu, uint16_t addr){
    if(addr == 0xF000){ //CHAR IN
        char c;
        if(scanf(" %c", &c) != 1) exit(0);
        return (uint16_t)c;
    }
    else if(addr == 0xF002) {// INT IN
        int i;
        printf("IN => ");
        if(scanf("%d", &i) != 1) exit(0);
        return (uint16_t)i;
    }
    else if (addr < MEM_SIZE) { // Read generico
        return cpu->memory[addr];
    }
    return 0;
}

void mem_write(cpu_t *cpu, uint16_t addr, uint16_t val){
    if(addr == 0xF001){ //CHAR OUT
        printf("OUT <= %c", (char)val);
        return;
    }
    else if(addr == 0xF003) { //INT OUT
        printf("OUT <= %d", (int16_t)val);
        return;
    }
    else if (addr < MEM_SIZE) { // Write generico
        cpu->memory[addr] = val;
        return;
    }
    return;
}

int16_t sign_extend(uint16_t val, int bits){
    int shift = 16 - bits; //Quanto deve ser deslocado
    uint16_t new_number = (int16_t) (val << shift) >> shift;
    //Primeiro "arrasta" o valor para a esquerda com <<
    //Depois "arrasta" novamente pra direita
    //Deixa um "rastro" com o sinal do valor agora com 16 bits
    return new_number;
}
void add(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t rn){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    uint16_t result = rmval+rnval;
    cpu->regs[rd] = result;

    //Atualiza flag_c
    if(result < rnval){
        cpu->flag_c = 1;
    }else{
        cpu->flag_c = 0;
    }
    //Atualiza flag_z
    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }
}

void addi(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t im){
    uint16_t rmval = cpu->regs[rm];
    uint16_t result = im + rmval;
    cpu->regs[rd] = result;

    //Atualiza flag_c
    if(result < rmval){
        cpu->flag_c = 1;
    }else{
        cpu->flag_c = 0;
    }
    //Atualiza flag_z
    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }
}

void sub(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t rn){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    uint16_t result = rmval-rnval;
    cpu->regs[rd] = result;
    //Atualiza flag_c
    if(rmval < rnval){
        cpu->flag_c = 1;
    }else{
        cpu->flag_c = 0;
    }
    //Atualiza flag_z
    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }
} 

void subi(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t im){
    uint16_t rmval = cpu->regs[rm];
    uint16_t result = rmval - im;
    cpu->regs[rd] = result;

    //Atualiza flag_c
    if(rmval < im){
        cpu->flag_c = 1;
    }else{
        cpu->flag_c = 0;
    }
    //Atualiza flag_z
    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }
}

void logic_and(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t rn){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    uint16_t result = rmval & rnval;
    cpu->regs[rd] = result;

    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }

    cpu->flag_c = 0;
}

void logic_or(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t rn){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    uint16_t result = rmval | rnval;
    cpu->regs[rd] = result;

    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }

    cpu->flag_c = 0;
}

void shr(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t im){
    uint16_t rmval = cpu->regs[rm];
    uint16_t result = rmval >> im;
    cpu->regs[rd] = result;

    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }

    cpu->flag_c = 0;
}

void shl(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t im){
    uint16_t rmval = cpu->regs[rm];
    uint16_t result = rmval << im;
    cpu->regs[rd] = result;

    if(cpu->regs[rd] == 0){
        cpu->flag_z = 1;
    }else{
        cpu->flag_z = 0;
    }

    cpu->flag_c = 0;
}

void cmp(cpu_t *cpu, uint16_t rm, uint16_t rn){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    cpu->flag_z = rmval == rnval;
    cpu->flag_c = rmval < rnval;
}

void str(cpu_t *cpu, uint16_t rm, uint16_t rn, uint16_t im){
    uint16_t rmval = cpu->regs[rm];
    uint16_t rnval = cpu->regs[rn];
    uint16_t addr = rmval + im;
    mem_write(cpu, addr, rnval);
    cpu->accessed[addr] = true;
}
void ldr(cpu_t *cpu, uint16_t rd, uint16_t rm, uint16_t im){
    uint16_t rdval = cpu->regs[rd];
    uint16_t rmval = cpu->regs[rm];
    uint16_t addr = rmval + im;
    uint16_t new_val = mem_read(cpu, addr);
    cpu->regs[rd] = new_val;
    cpu->accessed[addr] = true;
}

void dumpcpu(cpu_t *cpu){
    printf("<== CPU Registers ==>\n");
    for(int i=0; i<16; i++) printf("R%d = 0X%04hX\n", i, cpu->regs[i]);
    printf("Z=%d\n", cpu->flag_z);
    printf("C=%d\n", cpu->flag_c);
    
    for(int i=0; i<MEM_SIZE; i++){
        if(cpu->accessed[i]) printf("[0X%04X] = [0X%04hX]\n", i, cpu->memory[i]);
    }
    
    if(SP != SP_INITIAL){
        for(int i = SP_INITIAL - 1; i >= SP; i--){
            printf("[0x%04X] = 0x%04hX\n", i, cpu->memory[i]);
        }
    }
}
int main(int argc, char*argv[]){

    
    cpu_t system = {0};
    cpu_t *cpu = &system;
    bool breakpoints[MEM_SIZE] = {false};
    for (int i = 1; i < argc; i++) {
        int addr = (int)strtol(argv[i], NULL, 10);

        if (addr >= 0 && addr < MEM_SIZE) {
            breakpoints[addr] = true;
        }
    }
    memset(cpu->memory, 0x0000, MEM_SIZE);
    SP = SP_INITIAL; //Inicializando a pilha
    
    uint16_t addr, content;
    // loop para carregar a memoria
    while(scanf("%hx %hx%*[^\n]", &addr, &content) == 2){ //Verifica se o scanf está conseguindo ler dois numeros (addr e content)
        if(!addr && !content) break;
        cpu->memory[addr] = content;
    }
    //loop de execução
    do{
        uint16_t original_pc = PC;
            if(breakpoints[PC]) dumpcpu(cpu);

            uint16_t ir = mem_read(cpu, PC);
            PC++;

        //Decodificação do opcode
        uint16_t opcode = GET_OPCODE(ir);

        switch(opcode){
            case 0X0: //JMP incondicional
            {
                uint16_t im = ir >> 4;
                int16_t imextended = sign_extend(im, 12);//int e não uint pq o PC pode ir pra frente ou pra tras
                PC = PC + imextended;
                break;
            }
            case 0x1: //Jump
            {
                uint8_t decision = ir >> 14;
                int16_t jmp = (int16_t) ((ir >> 4) << 6) >> 6; // Retira o OPCODE e os dois primeiros bits(decision)
                switch(decision){
                    case 0X0: //JEQ
                        if(cpu->flag_z == 1) PC = PC + jmp;
                        break;
                    case 0X1: //JNE
                        if(cpu->flag_z == 0) PC = PC + jmp;
                        break;
                    case 0X2: //JLT
                        if(cpu->flag_z == 0 && cpu->flag_c == 1)PC = PC +jmp;
                        break;
                    case 0X3: //JGE
                        if(cpu->flag_z == 1 ||cpu->flag_c == 0) PC = PC +jmp;
                        break;
                }
                break;
            }
            case 0X2: //LDR
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t im = GET_RN(ir);
                    ldr(cpu, rd, rm, im);
                    break;
                }
            case 0X3: //STR
                {
                    uint16_t im = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    str(cpu, rm, rn, im);
                    break;
                }
            case 0X4: //MOV
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t im = GET_IM(ir);
                    uint16_t im_extended = sign_extend(im, 8);
                    cpu->regs[rd] = im_extended;
                    break;
                }   
            case 0X5: //ADD
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    add(cpu, rd, rm, rn);
                    break;
                }
            case 0X6: //ADDI
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t im = GET_RN(ir); // Imediato tem 4bits, msm valor do RN
                    uint16_t rm = GET_RM(ir);
                    addi(cpu, rd, rm, im);
                    break;
                }

            case 0X7: //SUB
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    sub(cpu, rd, rm, rn);
                    break;
                }
            case 0X8: //SUBI
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t im = GET_RN(ir); // Imediato tem 4bits, msm valor do RN
                    uint16_t rm = GET_RM(ir);
                    subi(cpu, rd, rm, im);
                    break;
                }
            case 0X9: //AND
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    logic_and(cpu, rd, rm, rn);
                    break;
                }
            case 0XA: //OR
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    logic_or(cpu, rd, rm, rn);
                    break;
                }
            case 0XB: //SHR
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t im = GET_RN(ir); // Imediato tem 4bits, msm valor do RN
                    uint16_t rm = GET_RM(ir);
                    shr(cpu, rd, rm, im);
                    break;
                }
            case 0XC: //SHL
                {
                    uint16_t rd = GET_RD(ir);
                    uint16_t im = GET_RN(ir); // Imediato tem 4bits, msm valor do RN
                    uint16_t rm = GET_RM(ir);
                    shl(cpu, rd, rm, im);
                    break;
                }
            case 0XD: //CMP
                {
                    uint16_t rm = GET_RM(ir);
                    uint16_t rn = GET_RN(ir);
                    cmp(cpu, rm, rn);
                    break;
                }
            case 0XE: //PUSH
                {
                    uint16_t rn = GET_RN(ir);
                    uint16_t val = cpu->regs[rn];
                    SP--;
                    mem_write(cpu, SP, val);
                    break;
                }
            case 0XF: //POP && HALT/
                if(ir == 0XFFFF){ //HALT
                    dumpcpu(cpu);
                    return 0;
                }else{ //POP
                    uint16_t rd = GET_RD(ir);
                    uint16_t val = mem_read(cpu, SP);
                    cpu->regs[rd] = val;
                    SP++;
                }
                break;
            default:
                printf("\nInstrução desconhecida %X em %X\n", opcode, PC-1);
                return 1;
        }
    }while(1);
    return 0;
}