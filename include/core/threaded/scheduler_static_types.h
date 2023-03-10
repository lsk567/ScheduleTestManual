/**
 * @file instruction_FS.h
 * @author Shaokai Lin <shaokai@eecs.berkeley.edu>
 * @brief Format of the instruction set
 */
enum opcode_t {SET, EXE, DU, WU, ADV, JUMP, STOP};

typedef struct inst_t {
    enum opcode_t   op;
    int             rs1;
    int             rs2;
} inst_t;