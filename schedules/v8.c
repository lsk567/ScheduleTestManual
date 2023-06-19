/**
 * @brief Loop (hyperperiod) unrolling for 4 iterations.
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
    {.op=BIT,   .rs1=15,     .rs2=-1},       // BIT if timeout, jump to line 15.
    
    // Iteration 1
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 1
    {.op=ADV2,  .rs1=2,     .rs2=10000000}, // ADV2 source2, 10000000

    // Iteration 2
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 2
    {.op=ADV2,  .rs1=2,     .rs2=10000000}, // ADV2 source2, 10000000

    // Iteration 3
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 3
    {.op=ADV2,  .rs1=2,     .rs2=10000000}, // ADV2 source2, 10000000

    // Iteration 4
    {.op=EXE,   .rs1=1,     .rs2=-1},       // EXE source2.0
    {.op=INC2,  .rs1=0,     .rs2=1},        // INC2 counter 0 => 4
    {.op=ADV2,  .rs1=2,     .rs2=10000000}, // ADV2 source2, 10000000

    {.op=SAC,   .rs1=-1,    .rs2=-1},       // Sync all workers And Clear counters
    {.op=JMP,   .rs1=0,     .rs2=-1},       // JMP to line 0
    {.op=STP,   .rs1=-1,    .rs2=-1},       // STP
};

const inst_t schedule_1[] = {
    {.op=BIT,   .rs1=39,    .rs2=-1},       // BIT if timeout, jump to line 39.
    
    // Iteration 1
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=ADV2,  .rs1=1,     .rs2=10000000}, // ADV2 source,  10000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=WU,    .rs1=0,     .rs2=1},        // WU counter 0 reaches 1
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000

    // Iteration 2
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=ADV2,  .rs1=1,     .rs2=10000000}, // ADV2 source,  10000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=WU,    .rs1=0,     .rs2=2},        // WU counter 0 reaches 2
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000

    // Iteration 3
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=ADV2,  .rs1=1,     .rs2=10000000}, // ADV2 source,  10000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=WU,    .rs1=0,     .rs2=3},        // WU counter 0 reaches 3
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000

    // Iteration 4
    {.op=EXE,   .rs1=0,     .rs2=-1},       // EXE source.0
    {.op=ADV2,  .rs1=1,     .rs2=10000000}, // ADV2 source,  10000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=EIT,   .rs1=3,     .rs2=-1},       // EIT sink.1
    {.op=WU,    .rs1=0,     .rs2=4},        // WU counter 0 reaches 4
    {.op=EIT,   .rs1=4,     .rs2=-1},       // EIT sink.2
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000
    {.op=EXE,   .rs1=2,     .rs2=-1},       // EXE sink.0
    {.op=ADV2,  .rs1=3,     .rs2=5000000},  // ADV2 sink, 5000000

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