/**
 * @brief Implement lock-free INC2 and lock-free WU.
 *
 * reaction array:
 * [0=source.0, 1=source2.0, 2=sink.0, 3=sink.1, 4=sink.2]
 * 
 * reactor array:
 * [0=main, 1=source, 2=source2, 3=sink]
 * 
 * counting variable arrays:
 * [0=thread 0]
 */

#include <stdint.h>
#include <stddef.h> // size_t
#include "../core/threaded/scheduler_instructions.h"

const inst_t schedule_0[] = {
    {.op=BIT,   .rs1=9,     .rs2=-1},       // BIT if timeout, jump to line 9.
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 1
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 2
    {.op=ADV,   .rs1=1,     .rs2=10000000}, // ADV source,  10000000
    {.op=ADV,   .rs1=2,     .rs2=10000000}, // ADV source2, 10000000
    {.op=SAC,   .rs1=-1,    .rs2=-1},       // Sync all workers And Clear counters
    {.op=JMP,   .rs1=0,     .rs2=-1},       // JMP to line 0
    {.op=STP,   .rs1=-1,    .rs2=-1},       // STP
};

const inst_t schedule_1[] = {
    {.op=BIT,   .rs1=11,    .rs2=-1},       // BIT if timeout, jump to line 13.
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=WU,    .rs1=0,     .rs2=1},        // WU counter 0 reaches 1
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=WU,    .rs1=0,     .rs2=2},        // WU counter 0 reaches 2
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV,   .rs1=3,     .rs2=5000000},  // ADV sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV,   .rs1=3,     .rs2=5000000},  // ADV sink, 5000000
    {.op=SAC,   .rs1=-1,    .rs2=-1},       // Sync all workers And Clear counters
    {.op=JMP,   .rs1=0,     .rs2=-1},       // JMP to line 0
    {.op=STP,   .rs1=-1,    .rs2=-1},       // STP
};

const inst_t* static_schedules[] = {
    schedule_0,
    schedule_1,
};

volatile uint32_t counters[] = {
    0
};

const size_t num_counters = 1;