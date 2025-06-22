#pragma once

typedef struct process process_t;

#include <stdio.h>
#include <input/input.h>
#include <stdint.h>
#include <asm.h>
#include <fs/fs.h>
#include <linked.h>
#include <personalities.h>

typedef int (* kill_callback_t)(int signal);

typedef struct process {
	uint16_t * name; // your name
	uint64_t pid; // id
	void * stack; // pointer to stack bottom
	uintptr_t stack_size; // stack size
	uint32_t ebp, esp, eip; // register save
	void * active_process; // active process save
	int killed; // is this process killed
	int system; // is system process
	int quantum; // time quantum allotted
	void * fpusave; // fpu register save buffer
	// for cleaning up after lazy processes
	linked_t * windows; // windows created
	linked_t * allocs; // tracks allocations made
	linked_t * allocs_top;
	input_callback_t recv_event; // recieve a targeted event
	input_callback_t recv_global_event; // recieve a global event
	stdout_handler_t stdin_handler;
	stdout_handler_t stdout_handler;
	kill_callback_t kill; // called when you tell a process to kill (not called for signal 9)
	fd_list_t * fds;
	personality_t * personality;
	int fd_top;
	void * stdin_priv;
	char * stdin_buffer;
	size_t stdin_size;
	void * stdout_priv;
	char * expecting;
	size_t expecting_length;
} process_t;

extern int multitasking_done;
// extern process_t * current_process;
// extern process_t * active_process;
extern linked_t * procs;
extern uint64_t proc_count;
extern uint64_t pid_top;

extern int max_quantum;

process_t ** get_current_process();
process_t ** get_active_process();
process_t * create_process(uint16_t * name, void * eip);
process_t * create_process_paused(uint16_t * name, void * eip);
process_t * create_process_sized(uint16_t * name, void * eip, uint32_t stack);
process_t * create_process_nostack(uint16_t * name);
process_t * create_event_zombie(uint16_t * name, input_callback_t handler);
int force_alive_kill_handler(int signal);
void switch_task();
void multitasking_init();
void * multitasking_alloc_stack();
void process_create_stack(process_t * process, uint32_t size);
process_t * multitasking_get_pid(uint64_t pid);
void multitasking_track_allocation(process_t * process, void * alloc);
void multitasking_untrack_allocation(process_t * process, void * alloc);
int multitasking_stack_overflow(void * stack, uintptr_t size, uintptr_t esp);
int multitasking_free_stack(void * stack, uintptr_t size);
void process_push_startup_arguments(process_t * process, void * eip);
void multitasking_handle_exception(int exception, registers_t * regs);
int kill(uint64_t pid, int signal);
void dead_process();
void yield();
void multitasking_push(process_t * proc, void * buffer, int length);
void multitasking_prepush(process_t * proc, int length);
void multitasking_postpush(process_t * proc, void * buffer, int length);
void multitasking_pop(process_t * proc, void * buffer, int length);
void process_push_args(process_t * process, int argc, char * argv[]);

// not really in multitasking.c
void clean_fpu();
void process_thumb();
void process_suicide();
void asm_task_switch(uint32_t eip, uint32_t ebp, uint32_t esp);
void asm_switch_stack(uint32_t esp);
int fork();
void asm_fork_return();
uint32_t geteip();
