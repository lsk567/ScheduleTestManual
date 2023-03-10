/**
 * @brief A supposedly generated file.
 * 
 * Six instructions are needed.
 * 1. SET   op1,    op2 : set a variable (op1) to a number (op2);
 * 2. EXE   op1         : execute a reaction (op1) if enabled (i.e. triggered);
 * 3. DU    op1,    op2 : delay until a physical timepoint (op1) plus an offset (op2) is reached;
 * 4. WU    op1         : wait until a condition variable (op1) is true;
 * 5. ADV   op1,    op2 : advance the logical time of a reactor (op1) by a specified amount (op2);
 * 6. JUMP  op1         : jump to a location;
 * 7. STOP              : stop the execution.
 * 
 * reaction array:
 * [0=source.0, 1=source2.0, 2=sink.0, 3=sink.1, 4=sink.2]
 * 
 * reactor array:
 * [0=source, 1=source2, 2=sink]
 * 
 * semaphore arrays:
 * [0=sink.0 & source.0, 1=sink.1 & source2.0, 2=hyperperiod_sync]
 */

// enum opcode_t {SET, EXE, DU, WU, ADV, JUMP, STOP};

// typedef struct inst_t {
//     enum opcode_t   op;
//     int             rs1;
//     int             rs2;
// } inst_t;

static const inst_t schedule_1[] = {
    {.op=EXE,   .rs1=2,     .rs2=-1},                    // EXE sink.0
    {.op=WU,    .rs1=0,     .rs2=-1},                    // WU  cond[0]
    {.op=EXE,   .rs1=3,     .rs2=-1},                    // EXE sink.1
    {.op=WU,    .rs1=1,     .rs2=-1},                    // WU  cond[1]
    {.op=EXE,   .rs1=4,     .rs2=-1},                    // EXE sink.2
    {.op=ADV,   .rs1=2,     .rs2=5000000},  // ADV sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},                    // EXE sink.0
    {.op=ADV,   .rs1=2,     .rs2=5000000},  // ADV sink, 5000000
    {.op=WU,    .rs1=2,     .rs2=-1},                    // WU  cond[2] 
    {.op=JUMP,  .rs1=0,     .rs2=-1},                    // JUMP line_0
};

static const inst_t schedule_2[] = {
    {.op=EXE,   .rs1=0,     .rs2=-1},                    // EXE  source.0
    {.op=EXE,   .rs1=1,     .rs2=-1},                    // EXE  source2.0
    {.op=ADV,   .rs1=0,     .rs2=10000000}, // ADV  source,  10000000
    {.op=ADV,   .rs1=1,     .rs2=10000000}, // ADV  source2, 10000000
    {.op=WU,    .rs1=2,     .rs2=-1},                    // WU   cond[2] 
    {.op=JUMP,  .rs1=0,     .rs2=-1},                    // JUMP line_0
};

static const inst_t* static_schedules[] = {
    schedule_1,
    schedule_2,
};

static const int schedule_lengths[] = {
    10,
    6,
};