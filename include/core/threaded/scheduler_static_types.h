/**
 * @file instruction_FS.h
 * @author Shaokai Lin <shaokai@eecs.berkeley.edu>
 * @brief Format of the instruction set
 */
typedef enum {EIT, EXE, DU, WU, ADV, INC, SAC, JMP, STP} opcode_t;

typedef struct inst_t {
    opcode_t    op;
    int         rs1;
    int         rs2;
} inst_t;