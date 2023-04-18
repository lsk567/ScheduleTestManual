/**
 * @brief A supposedly generated file.
 * 
 * reaction array:
 * [0=source.0, 1=source2.0, 2=sink.0, 3=sink.1, 4=sink.2]
 * 
 * reactor array:
 * [0=source, 1=source2, 2=sink]
 * 
 * counting variable arrays:
 * [0=sink.1 waiting on sink.0 & source.0, 1=sink.2 waiting on sink.1 & source2.0]
 */

// #include "core/threaded/scheduler_instructions.h"

const inst_t schedule_0[] = {
    {.op=BIT,   .rs1=13,     .rs2=-1},      // BIT if timeout, jump to line 13.
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=INC,   .rs1=0,     .rs2=1},        // INC counter 0 by 1
    {.op=WU,    .rs1=0,     .rs2=2},        // WU  counter 0 reaches 2
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=INC,   .rs1=1,     .rs2=1},        // INC counter 1 by 1
    {.op=WU,    .rs1=1,     .rs2=2},        // WU  counter 1 reaches 2
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV,   .rs1=2,     .rs2=5000000},  // ADV sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV,   .rs1=2,     .rs2=5000000},  // ADV sink, 5000000
    {.op=SAC,   .rs1=-1,    .rs2=-1},       // Sync all workers And Clear counters
    {.op=JMP,   .rs1=0,     .rs2=-1},       // JMP to line 0
    {.op=STP,   .rs1=-1,     .rs2=-1},      // STP
};

const inst_t schedule_1[] = {
    {.op=BIT,   .rs1=9,     .rs2=-1},       // BIT if timeout, jump to line 9.
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=INC,   .rs1=0,     .rs2=1},        // INC counter 0 by 1
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC,   .rs1=1,     .rs2=1},        // INC counter 1 by 1
    {.op=ADV,   .rs1=0,     .rs2=10000000}, // ADV source,  10000000
    {.op=ADV,   .rs1=1,     .rs2=10000000}, // ADV source2, 10000000
    {.op=SAC,   .rs1=-1,    .rs2=-1},       // Sync all workers And Clear counters
    {.op=JMP,   .rs1=0,     .rs2=-1},       // JMP to line 0
    {.op=STP,   .rs1=-1,     .rs2=-1},      // STP
};

const inst_t* static_schedules[] = {
    schedule_0,
    schedule_1,
};

volatile uint32_t counters[] = {
    0, 0, 0, 0
};

const size_t num_counters = 4;