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
extern const int schedule_lengths[];

/////////////////// Scheduler Variables and Structs /////////////////////////
_lf_sched_instance_t* _lf_sched_instance;

/////////////////// Scheduler Private API /////////////////////////

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
    LF_PRINT_DEBUG("NOT IMPLEMENTED: _lf_sched_wait_for_work");
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
    _lf_sched_instance->schedule_lengths = schedule_lengths;
    _lf_sched_instance->reaction_instances = params->reaction_instances;
    _lf_sched_instance->reactor_self_instances = params->reactor_self_instances;
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

    while (*pc < _lf_sched_instance->schedule_lengths[worker_number] && !exit_loop) {
        LF_PRINT_DEBUG("Current instruction for worker %d: [Line %zu] %d %d %d", worker_number, *pc, current_schedule[*pc].op, current_schedule[*pc].rs1, current_schedule[*pc].rs2);

        switch (current_schedule[*pc].op) {
            case SET:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: SET");
                exit_loop = true;
                break;
            }
            // If the instruction is Execute,
            // return the reaction pointer and advance pc.
            case EXE:
            {
                reaction_t* reaction = _lf_sched_instance->reaction_instances[current_schedule[*pc].rs1];
                if (reaction->status == queued) {
                    returned_reaction = reaction;
                    exit_loop = true;
                } else
                    LF_PRINT_DEBUG("Worker %d skip execution", worker_number);
                *pc += 1;
                break;
            }
            // If the instruction is Wait, block until
            // the reaction is finished (by checking
            // the semaphore) and process the next instruction
            // until we process an Execute.
            case WU:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: WU");
                exit_loop = true;
                break;
            }
            case DU:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: DU");
                exit_loop = true;
                break;
            }
            case ADV:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: ADV");
                exit_loop = true;
                break;
            }
            case JUMP:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: JUMP");
                exit_loop = true;
                break;
            }
            case STOP:
            {
                LF_PRINT_DEBUG("NOT IMPLEMENTED: STOP");
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
    if (!lf_bool_compare_and_swap(&done_reaction->status, queued, inactive)) {
        lf_print_error_and_exit("Unexpected reaction status: %d. Expected %d.",
                              done_reaction->status, queued);
    }
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
    // Mark a reaction as queued, so that it will be executed when workers do work.
    reaction->status = queued;
}
#endif
#endif