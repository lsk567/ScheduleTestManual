#if defined(LF_THREADED)
/* Non-preemptive scheduler for the threaded runtime of the C target of Lingua
Franca. */

/*************
Copyright (c) 2022, The University of Texas at Dallas. Copyright (c) 2022, The
University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * A fully static (FS) scheduler for the threaded runtime of the C target of
 * Lingua Franca.
 *
 * @author{Shaokai Lin <shaokai@berkeley.edu>}
 */
#include "lf_types.h"
#if SCHEDULER == FS || (!defined(SCHEDULER) && defined(LF_THREADED))
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "platform.h"
#include "reactor_common.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "semaphore.h"
#include "trace.h"
#include "util.h"

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;
extern const inst_t* static_schedules[];
extern volatile uint32_t hyperperiod_iterations[];
extern volatile uint32_t counters[];
extern const size_t num_counters;
extern tag_t current_tag;
extern tag_t stop_tag;
extern instant_t start_time;

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;

/////////////////// Scheduler Private API /////////////////////////

/**
 * @brief If there is work to be done, notify workers individually.
 *
 * This assumes that the caller is not holding any thread mutexes.
 */
void _lf_sched_notify_workers() {
    // Note: All threads are idle. Therefore, there is no need to lock the mutex
    // while accessing the executing queue (which is pointing to one of the
    // reaction queues).
    size_t workers_to_awaken = 
        _lf_sched_instance->_lf_sched_number_of_idle_workers;
    LF_PRINT_DEBUG("Scheduler: Notifying %zu workers.", workers_to_awaken);
    _lf_sched_instance->_lf_sched_number_of_idle_workers -= workers_to_awaken;
    LF_PRINT_DEBUG("Scheduler: New number of idle workers: %zu.",
                _lf_sched_instance->_lf_sched_number_of_idle_workers);
    if (workers_to_awaken > 1) {
        // Notify all the workers except the worker thread that has called this
        // function.
        lf_semaphore_release(_lf_sched_instance->_lf_sched_semaphore,
                             (workers_to_awaken - 1));
    }
}

/**
 * @brief Wait until the scheduler assigns work.
 *
 * If the calling worker thread is the last to become idle, it will call on the
 * scheduler to distribute work. Otherwise, it will wait on
 * '_lf_sched_instance->_lf_sched_semaphore'.
 *
 * @param worker_number The worker number of the worker thread asking for work
 * to be assigned to it.
 */
void _lf_sched_wait_for_work(size_t worker_number) {
    // Increment the number of idle workers by 1 and
    // check if this is the last worker thread to become idle.
    if (lf_atomic_add_fetch(&_lf_sched_instance->_lf_sched_number_of_idle_workers,
                            1) ==
        _lf_sched_instance->_lf_sched_number_of_workers) {
        // Last thread to go idle
        LF_PRINT_DEBUG("Scheduler: Worker %zu is the last idle thread.",
                    worker_number);
        // Clear all the counters.
        for (int i = 0; i < num_counters; i++) {
            counters[i] = 0;
        }
        // Call on the scheduler to distribute work or advance tag.
        _lf_sched_notify_workers();
    } else {
        // Not the last thread to become idle.
        // Wait for work to be released.
        lf_semaphore_acquire(_lf_sched_instance->_lf_sched_semaphore);
    }
}

/**
 * @brief BIT: Branch If Timeout
 * Check if timeout is reached. If not, don't do anything.
 * If so, jump to a specified location (rs1).
 * 
 * FIXME: Should the timeout value be an operand?
 * FIXME: Use a global variable num_active_reactors instead of iterating over
 * a for loop.
 */
void execute_inst_BIT(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    bool stop = true;
    for (int i = 0; i < _lf_sched_instance->num_reactor_self_instances; i++) {
        if (!_lf_sched_instance->reactor_reached_stop_tag[i]) {
            stop = false;
            break;
        }
    }

    LF_PRINT_DEBUG("Start time is %ld. Current tag is (%ld, %d). Stop tag is (%ld, %d). Stop array: ", start_time, current_tag.time, current_tag.microstep, stop_tag.time, stop_tag.microstep);
    for (int i = 0; i < _lf_sched_instance->num_reactor_self_instances; i++) {
        LF_PRINT_DEBUG("(%ld, %d)",
            _lf_sched_instance->reactor_self_instances[i]->tag.time,
            _lf_sched_instance->reactor_self_instances[i]->tag.microstep);
        LF_PRINT_DEBUG("%d", _lf_sched_instance->reactor_reached_stop_tag[i]);
    }

    if (stop) *pc = rs1;    // Jump to a specified location.
    else *pc += 1;          // Increment pc.
}

/**
 * @brief EIT: "Execute-If-Triggered"
 * Check if the reaction status is "queued."
 * If so, return the reaction pointer and advance pc.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_EIT(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    reaction_t* reaction = _lf_sched_instance->reaction_instances[rs1];
    if (reaction->status == queued) {
        *returned_reaction = reaction;
        *exit_loop = true;
    } else
        LF_PRINT_DEBUG("*** Worker %zu skip execution", worker_number);
    *pc += 1; // Increment pc.
}

/**
 * @brief EXE: Execute a reaction
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_EXE(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    reaction_t* reaction = _lf_sched_instance->reaction_instances[rs1];
    *returned_reaction = reaction;
    *exit_loop = true;
    *pc += 1; // Increment pc.
}

/**
 * @brief DU: Delay Until a physical timepoint (rs1) plus an offset (rs2) is reached.
 * 
 * @param worker_number 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_DU(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    // FIXME: There seems to be an overflow problem.
    // When wakeup_time overflows but lf_time_physical() doesn't,
    // lf_sleep_until_locked() terminates immediately. 
    instant_t wakeup_time = physical_start_time + rs1 * (*iteration + 1);
    LF_PRINT_DEBUG("physical_start_time: %ld, wakeup_time: %ld, rs1: %lld, iteration+1: %d, current_physical_time: %ld\n", physical_start_time, wakeup_time, rs1, (*iteration + 1), lf_time_physical());
    LF_PRINT_DEBUG("*** Worker %zu delaying", worker_number);
    lf_sleep_until_locked(wakeup_time);
    LF_PRINT_DEBUG("*** Worker %zu done delaying", worker_number);
    *pc += 1; // Increment pc.
}

/**
 * @brief WU: Wait until a counting variable reaches a specified value.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_WU(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    LF_PRINT_DEBUG("*** Worker %zu waiting", worker_number);
    while(_lf_sched_instance->counters[rs1] < rs2);
    LF_PRINT_DEBUG("*** Worker %zu done waiting", worker_number);
    *pc += 1; // Increment pc.
}

/**
 * @brief ADV: Advance time for a particular reactor.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_ADV(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    
    // This mutex is quite expensive.
    lf_mutex_lock(&mutex);

    self_base_t* reactor =
        _lf_sched_instance->reactor_self_instances[rs1];
    reactor->tag.time += rs2;
    reactor->tag.microstep = 0;

    if (_lf_is_tag_after_stop_tag(reactor->tag)) {
        _lf_sched_instance->reactor_reached_stop_tag[rs1] = true;
    }
   
    lf_mutex_unlock(&mutex);

    *pc += 1; // Increment pc.
}

/**
 * @brief ADV: Advance time for a particular reactor.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_ADV2(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {

    self_base_t* reactor =
        _lf_sched_instance->reactor_self_instances[rs1];
    reactor->tag.time += rs2;
    reactor->tag.microstep = 0;

    if (_lf_is_tag_after_stop_tag(reactor->tag)) {
        _lf_sched_instance->reactor_reached_stop_tag[rs1] = true;
    }
   
    *pc += 1; // Increment pc.
}

/**
 * @brief JMP: Jump to a particular line in the schedule.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_JMP(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    if (rs2 != -1) *iteration += 1;
    *pc = rs1;
}

/**
 * @brief SAC: (Sync-And-Clear) synchronize all workers until all execute SAC
 * and let the last idle worker reset all counters to 0.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_SAC(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    tracepoint_worker_wait_starts(worker_number);
    _lf_sched_wait_for_work(worker_number);
    tracepoint_worker_wait_ends(worker_number);
    *pc += 1; // Increment pc.
}

/**
 * @brief INC: INCrement a counter (rs1) by an amount (rs2).
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_INC(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    lf_mutex_lock(&mutex);
    _lf_sched_instance->counters[rs1] += rs2;
    lf_mutex_unlock(&mutex);
    *pc += 1; // Increment pc.
}

/**
 * @brief INC2: [Lock-free] INCrement a counter (rs1) by an amount (rs2).
 * The compiler needs to guarantee a single writer.
 * 
 * @param rs1 
 * @param rs2 
 * @param pc 
 * @param returned_reaction 
 * @param exit_loop 
 */
void execute_inst_INC2(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    _lf_sched_instance->counters[rs1] += rs2;
    *pc += 1; // Increment pc.
}

/**
 * @brief STP: SToP the execution.
 * 
 */
void execute_inst_STP(size_t worker_number, long long int rs1, long long int rs2, size_t* pc,
    reaction_t** returned_reaction, bool* exit_loop, volatile int* iteration) {
    *exit_loop = true;
}

/**
 * @brief Execute an instruction
 * 
 * @param op the opcode
 * @param rs1 the first operand
 * @param rs2 the second operand
 * @param pc a pointer to the program counter
 * @param returned_reaction a pointer to a reaction to be executed 
 * 
 * FIXME: This feels like a bad design in the abstraction.
 * @param exit_loop a pointer to a boolean indicating whether
 *                  the outer while loop should be exited
 */
void execute_inst(size_t worker_number, opcode_t op, long long int rs1, long long int rs2,
    size_t* pc, reaction_t** returned_reaction, bool* exit_loop, volatile volatile int* iteration) {
    char* op_str = NULL;
    switch (op) {
        case ADV:
            op_str = "ADV";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_ADV(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case ADV2:
            op_str = "ADV2";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_ADV2(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case BIT:
            op_str = "BIT";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_BIT(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
         case DU:  
            op_str = "DU";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_DU(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case EIT:
            op_str = "EIT";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_EIT(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case EXE:
            op_str = "EXE";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_EXE(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case INC:
            op_str = "INC";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_INC(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case INC2:
            op_str = "INC2";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_INC2(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case JMP:
            op_str = "JMP";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_JMP(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case SAC:
            op_str = "SAC";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_SAC(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case STP:
            op_str = "STP";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_STP(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        case WU:
            op_str = "WU";
            LF_PRINT_DEBUG("*** Current instruction for worker %zu: [Line %zu] %s %lld %lld",
                            worker_number, *pc, op_str, rs1, rs2);
            execute_inst_WU(worker_number, rs1, rs2, pc, returned_reaction, exit_loop, iteration);
            break;
        default:
            lf_print_error_and_exit("Invalid instruction: %d", op);
    }
}

///////////////////// Scheduler Init and Destroy API /////////////////////////
/**
 * @brief Initialize the scheduler.
 *
 * This has to be called before other functions of the scheduler can be used.
 * If the scheduler is already initialized, this will be a no-op.
 *
 * @param number_of_workers Indicate how many workers this scheduler will be
 *  managing.
 * @param option Pointer to a `sched_params_t` struct containing additional
 *  scheduler parameters.
 */
void lf_sched_init(
    size_t number_of_workers,
    sched_params_t* params
) {
    LF_PRINT_DEBUG("Scheduler: Initializing with %zu workers", number_of_workers);
    if(!init_sched_instance(&_lf_sched_instance, number_of_workers, params)) {
        // FIXME: This is not the best practice and seems to take advantage of a
        //        bug in the runtime.
        //        lf_sched_init() is for some reason called twice.
        //        Once in lf_reactor_c_main() in reactor_threaded.c.
        //        Another in initialize() -> _lf_initialize_trigger_objects()
        //        -> lf_sched_init(), also in reactor_threaded.c.
        //        This implementation takes advantage of the fact that when
        //        lf_sched_init() is called the second time, start_time is set
        //        to a meaningful value. When the first time lf_sched_init() is
        //        called, start_time has not been set.

        // Initialize the local tags for the FS scheduler.
        for (int i = 0; i < _lf_sched_instance->num_reactor_self_instances; i++) {
            _lf_sched_instance->reactor_self_instances[i]->tag.time = start_time;
            _lf_sched_instance->reactor_self_instances[i]->tag.microstep = 0;
            LF_PRINT_DEBUG("(%ld, %d)",
                _lf_sched_instance->reactor_self_instances[i]->tag.time,
                _lf_sched_instance->reactor_self_instances[i]->tag.microstep);
        }

        // Already initialized
        return;
    }

    _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
    _lf_sched_instance->static_schedules = &static_schedules[0];
    _lf_sched_instance->reaction_instances = params->reaction_instances;
    _lf_sched_instance->reactor_self_instances = params->reactor_self_instances;
    _lf_sched_instance->num_reactor_self_instances = params->num_reactor_self_instances;
    _lf_sched_instance->reactor_reached_stop_tag = params->reactor_reached_stop_tag;
    _lf_sched_instance->counters = counters;

    // FIXME: Why does this show a negative value?
    LF_PRINT_DEBUG("start_time = %ld", start_time);
}

/**
 * @brief Free the memory used by the scheduler.
 *
 * This must be called when the scheduler is no longer needed.
 */
void lf_sched_free() {
    LF_PRINT_DEBUG("Freeing the pointers in the scheduler struct.");
    free(_lf_sched_instance->pc);
    free(_lf_sched_instance->reactor_self_instances);
    free(_lf_sched_instance->reaction_instances);
}

///////////////////// Scheduler Worker API (public) /////////////////////////
/**
 * @brief Ask the scheduler for one more reaction.
 *
 * This function blocks until it can return a ready reaction for worker thread
 * 'worker_number' or it is time for the worker thread to stop and exit (where a
 * NULL value would be returned).
 *
 * @param worker_number
 * @return reaction_t* A reaction for the worker to execute. NULL if the calling
 * worker thread should exit.
 */
reaction_t* lf_sched_get_ready_reaction(int worker_number) {
    LF_PRINT_DEBUG("Worker %d inside lf_sched_get_ready_reaction", worker_number);
    
    const inst_t*   current_schedule    = _lf_sched_instance->static_schedules[worker_number];
    reaction_t*     returned_reaction   = NULL;
    bool            exit_loop           = false;
    size_t*         pc                  = &_lf_sched_instance->pc[worker_number];
    opcode_t        op;
    long long int   rs1;
    long long int   rs2;
    volatile int*   iteration           = &hyperperiod_iterations[worker_number];

    while (!exit_loop) {
        op  = current_schedule[*pc].op;
        rs1 = current_schedule[*pc].rs1;
        rs2 = current_schedule[*pc].rs2;

        // Execute the current instruction
        execute_inst(worker_number, op, rs1, rs2, pc,
                    &returned_reaction, &exit_loop, iteration);

        LF_PRINT_DEBUG("Worker %d: returned_reaction = %p, exit_loop = %d",
                        worker_number, returned_reaction, exit_loop);
    }

    LF_PRINT_DEBUG("Worker %d leaves lf_sched_get_ready_reaction", worker_number);
    return returned_reaction;
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' is done
 * executing the 'done_reaction'.
 *
 * @param worker_number The worker number for the worker thread that has
 * finished executing 'done_reaction'.
 * @param done_reaction The reaction that is done.
 */
void lf_sched_done_with_reaction(size_t worker_number,
                                 reaction_t* done_reaction) {
    LF_PRINT_DEBUG("*** Worker %zu inside lf_sched_done_with_reaction, done with %s", worker_number, done_reaction->name);
    // If the reaction status is queued, change it back to inactive.
    // We do not check for error here because the EXE instruction
    // can execute a reaction with an "inactive" status.
    // The reason is that since runtime does not advance
    // global time, the next timer events will not be
    // scheduled and put onto the event queue. The next
    // timer events are encoded directly into the schedule
    // using the EXE instructions.
    lf_bool_compare_and_swap(&done_reaction->status, queued, inactive);

    LF_PRINT_DEBUG("*** Worker %zu reports updated status for %s: %u", worker_number, done_reaction->name, done_reaction->status);
}

/**
 * @brief Inform the scheduler that worker thread 'worker_number' would like to
 * trigger 'reaction' at the current tag.
 *
 * If a worker number is not available (e.g., this function is not called by a
 * worker thread), -1 should be passed as the 'worker_number'.
 *
 * This scheduler ignores the worker number.
 *
 * The scheduler will ensure that the same reaction is not triggered twice in
 * the same tag.
 *
 * @param reaction The reaction to trigger at the current tag.
 * @param worker_number The ID of the worker that is making this call. 0 should
 *  be used if there is only one worker (e.g., when the program is using the
 *  unthreaded C runtime). -1 is used for an anonymous call in a context where a
 *  worker number does not make sense (e.g., the caller is not a worker thread).
 *
 */
void lf_sched_trigger_reaction(reaction_t* reaction, int worker_number) {
    LF_PRINT_DEBUG("*** Inside lf_sched_trigger_reaction");
    LF_PRINT_DEBUG("*** Worker %d triggering reaction %s", worker_number, reaction->name);
    // Mark a reaction as queued, so that it will be executed when workers do work.
    if (!lf_bool_compare_and_swap(&reaction->status, inactive, queued)) {
        // FIXME: Uncommenting the code below yields weird exception.
        // lf_print_error_and_exit("Worker %d reports unexpected reaction status for reaction %s: %d. Expected %d.",
        //                         worker_number, reaction->name,
        //                         reaction->status, inactive);
    }
}
#endif
#endif