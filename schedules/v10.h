/**
 * @brief Use WU for synchronization and use DU for releasing tasks.
 *
 * reaction array:
 * [0=source.0, 1=source2.0, 2=sink.0, 3=sink.1, 4=sink.2]
 * 
 * reactor array:
 * [0=source, 1=source2, 2=sink]
 * 
 * counting variable arrays:
 * [0=thread 0]
 */

// #include "core/threaded/scheduler_instructions.h"

const inst_t schedule_0[] = {
    {.op=BIT,   .rs1=5,             .rs2=-1},           // BIT if timeout, jump to line 5.
    
    {.op=EXE,   .rs1=1,             .rs2=-1},           // EXE source2.0
    {.op=INC2,  .rs1=0,             .rs2=1},            // INC2 counter 0 => 1
    {.op=ADV2,  .rs1=1,             .rs2=10000000LL},   // ADV2 source2, 10000000
    
    {.op=DU,    .rs1=10000000LL,    .rs2=-1},           // DU until 10 ms
    {.op=JMP,   .rs1=0,             .rs2=1},            // JMP to line 0, increment hyperperiod iteration
    {.op=STP,   .rs1=-1,            .rs2=-1},           // STP
};

const inst_t schedule_1[] = {
    {.op=BIT,   .rs1=12,            .rs2=-1},           // BIT if timeout, jump to line 12.
    
    // Iteration 1
    {.op=EXE,   .rs1=0,             .rs2=-1},           // EXE source.0
    {.op=ADV2,  .rs1=0,             .rs2=10000000LL},   // ADV2 source,  10000000
    {.op=EXE,   .rs1=2,             .rs2=-1},           // EXE sink.0
    {.op=EIT,   .rs1=3,             .rs2=-1},           // EIT sink.1
    {.op=WU,    .rs1=0,             .rs2=1},            // WU counter 0 reaches 1
    {.op=EIT,   .rs1=4,             .rs2=-1},           // EIT sink.2
    {.op=ADV2,  .rs1=2,             .rs2=5000000LL},    // ADV2 sink, 5000000
    {.op=DU,    .rs1=5000000LL,     .rs2=-1},           // DU until 5 ms
    {.op=EXE,   .rs1=2,             .rs2=-1},           // EXE sink.0
    {.op=ADV2,  .rs1=2,             .rs2=5000000LL},    // ADV2 sink, 5000000

    {.op=DU,    .rs1=10000000LL,    .rs2=-1},           // DU until 10 ms
    {.op=JMP,   .rs1=0,             .rs2=1},            // JMP to line 0, increment hyperperiod iteration
    {.op=STP,   .rs1=-1,            .rs2=-1},           // STP
};

const inst_t* static_schedules[] = {
    schedule_0,
    schedule_1,
};

volatile uint32_t counters[] = {
    0
};

// Note: there would be a race condition if the threads are not keeping track of
// its own hyperperiod.
volatile uint32_t hyperperiod_iterations[] = {
    0,
    0
};

const size_t num_counters = 1;