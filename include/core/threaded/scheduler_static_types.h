/**
 * @file instruction_FS.h
 * @author Shaokai Lin <shaokai@eecs.berkeley.edu>
 * @brief Format of the instruction set
 */
typedef enum {
    ADV,
    BIT,
    DU,
    EIT,
    EXE,
    INC,
    JMP,
    SAC,
    STP,
    WU,
} opcode_t;

typedef struct inst_t {
    opcode_t    op;
    int         rs1;
    int         rs2;
} inst_t;