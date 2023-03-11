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
 * Non-preemptive scheduler for the threaded runtime of the C target of Lingua
 * Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Edward A. Lee <eal@berkeley.edu>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */
#include "lf_types.h"
#if SCHEDULER == FS || (!defined(SCHEDULER) && defined(LF_THREADED))
#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif  // NUMBER_OF_WORKERS

#include <assert.h>

#include "platform.h"
#include "scheduler_instance.h"
#include "scheduler_sync_tag_advance.h"
#include "scheduler.h"
#include "semaphore.h"
#include "trace.h"
#include "util.h"

#include "../../static_schedule.h" // Generated

/////////////////// External Variables /////////////////////////
extern lf_mutex_t mutex;
extern const inst_t* static_schedules[];
extern uint32_t counters[];
extern const size_t num_counters;

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
        // Clear all the counters
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
        // Already initialized
        return;
    }

    _lf_sched_instance->pc = calloc(number_of_workers, sizeof(size_t));
    _lf_sched_instance->static_schedules = &static_schedules[0];
    _lf_sched_instance->reaction_instances = params->reaction_instances;
    _lf_sched_instance->reactor_self_instances = params->reactor_self_instances;
    _lf_sched_instance->counters = counters;
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
    size_t* pc = &_lf_sched_instance->pc[worker_number];
    const inst_t* current_schedule = _lf_sched_instance->static_schedules[worker_number];
    bool exit_loop = false;
    reaction_t* returned_reaction = NULL;
    opcode_t op;
    int rs1;
    int rs2;

    while (!exit_loop) {
        op  = current_schedule[*pc].op;
        rs1 = current_schedule[*pc].rs1;
        rs2 = current_schedule[*pc].rs2;

        char* op_str = NULL;
        switch (op) {
            case EIT: op_str = "EIT"; break;
            case EXE: op_str = "EXE"; break;
            case DU:  op_str = "DU";  break;
            case WU:  op_str = "WU";  break;
            case ADV: op_str = "ADV"; break;
            case JMP: op_str = "JMP"; break;
            case SAC: op_str = "SAC"; break;
            case INC: op_str = "INC"; break;
            case STP: op_str = "STP"; break;
        }

        LF_PRINT_DEBUG("*** Current instruction for worker %d: [Line %zu] %s %d %d", worker_number, *pc, op_str, rs1, rs2);

        switch (op) {
            // EIT: "Execute-If-Triggered"
            // Check if the reaction status is "queued."
            // If so, return the reaction pointer and advance pc.
            case EIT:
            {
                reaction_t* reaction = _lf_sched_instance->reaction_instances[rs1];
                if (reaction->status == queued) {
                    returned_reaction = reaction;
                    exit_loop = true;
                } else
                    LF_PRINT_DEBUG("*** Worker %d skip execution", worker_number);
                *pc += 1; // Increment pc.
                break;
            }
            // EXE: "EXEcute"
            case EXE:
            {
                reaction_t* reaction = _lf_sched_instance->reaction_instances[rs1];
                returned_reaction = reaction;
                exit_loop = true;
                *pc += 1; // Increment pc.
                break;
            }
            // If the instruction is Wait, block until
            // the reaction is finished (by checking
            // the semaphore) and process the next instruction
            // until we process an Execute.
            case WU:
            {
                while(_lf_sched_instance->counters[rs1] < rs2) {
                    LF_PRINT_DEBUG("*** Worker %d waiting", worker_number);
                }
                *pc += 1; // Increment pc.
                break;
            }
            case DU:
            {
                LF_PRINT_DEBUG("*** NOT IMPLEMENTED: DU");
                exit_loop = true;
                break;
            }
            case ADV:
            {
                lf_mutex_lock(&mutex);
                _lf_sched_instance->reactor_self_instances[rs1]->tag.time += rs2;
                _lf_sched_instance->reactor_self_instances[rs1]->tag.microstep = 0;
                lf_mutex_unlock(&mutex);
                *pc += 1; // Increment pc.
                break;
            }
            case JMP:
            {
                // LF_PRINT_DEBUG("*** NOT IMPLEMENTED: JMP");
                // exit_loop = true;
                *pc = rs1;
                break;
            }
            case INC:
            {
                lf_mutex_lock(&mutex);
                _lf_sched_instance->counters[rs1] += rs2;
                lf_mutex_unlock(&mutex);
                *pc += 1; // Increment pc.
                break;
            }
            case SAC:
            {
                _lf_sched_wait_for_work(worker_number);
                *pc += 1; // Increment pc.
                break;
            }
            case STP:
            {
                exit_loop = true;
                break;
            } 
        }
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
    LF_PRINT_DEBUG("*** Inside lf_sched_done_with_reaction");
    // If the reaction status is queued, change it back to inactive.
    // We do not check for error here because the EXE instruction
    // can execute a reaction with an "inactive" status.
    // The reason is that since runtime does not advance
    // global time, the next timer events will not be
    // scheduled and put onto the event queue. The next
    // timer events are encoded directly into the schedule
    // using the EXE instructions.
    lf_bool_compare_and_swap(&done_reaction->status, queued, inactive);
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
        lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.",
                              reaction->status, inactive);
    }
}
#endif
#endif