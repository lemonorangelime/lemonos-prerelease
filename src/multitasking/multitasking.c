#include <graphics/graphics.h>
#include <input/input.h>
#include <multitasking.h>
#include <memory.h>
#include <linked.h>
#include <interrupts/apic.h>
#include <cpuid.h>
#include <util.h>
#include <pit.h>
#include <terminal.h>
#include <string.h>
#include <simd.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <panic.h>
#include <boot/boot.h>
#include <syscall.h>
#include <power/sleep.h>
#include <personalities.h>

//

linked_t * procs;
linked_t * proc_step;
process_t ** current_processes;
process_t ** active_processes;
uint64_t proc_count = 0;
uint64_t taskp = 0;
uint64_t pid_top = 0;
int multitasking_done = 0;

int max_quantum;

size_t stack_size = 0xffff;

// wow ! so complex !
void multitasking_default_event_handler(event_t * event) {
	return;
}
int multitasking_default_kill_handler(int signal) {
	return 0;
}

int force_alive_kill_handler(int signal) {
	return 1;
}

process_t ** get_current_process() {
	int id = get_apic_id();
	return &current_processes[id];
}

process_t ** get_active_process() {
	int id = get_apic_id();
	return &active_processes[id];
}

process_t * alloc_process(uint16_t * name) {
	process_t * process = malloc(sizeof(process_t));
	memset(process, 0, sizeof(process_t));
	process->name = ustrdup(name);
	process->active_process = process;
	process->pid = pid_top++;
	process->eip = (uintptr_t) process_thumb;
	process->personality = get_personality_by_type(PERSONALITY_LINUX);
	process->fds = NULL;
	process->fd_top = 3;
	process->killed = 1;
	process->system = 0;
	process->quantum = max_quantum / 2;
	process->windows = NULL;
	process->allocs = NULL;
	process->recv_event = multitasking_default_event_handler;
	process->recv_global_event = multitasking_default_event_handler;
	process->kill = multitasking_default_kill_handler;
	if (sse_level != -1) {
		process->fpusave = malloc(512);
		memset(process->fpusave, 0, 512);
	}
	hook_stdout(process, ansi_stdout_handler, ansi_stdout_constructor);
	return process;
}

void process_set_quantum(process_t * proc, int quantum) {
	if (!quantum) {
		return; // what the fuck are you doing
	}
	proc->quantum = quantum;
}

void append_process(process_t * process) {
	int save = interrupts_enabled();
	disable_interrupts();
	procs = linked_add(procs, process);
	proc_count++;
	interrupts_restore(save);
}

void process_create_stack(process_t * process, uint32_t size) {
	process->stack = multitasking_alloc_stack(size);
	process->stack_size = size;
	process->esp = (uint32_t) process->stack;
	process->ebp = process->esp;
}

void process_suicide() {
	kill(0xffffffffffffffff, 9);
	yield();
}

void process_push_startup_arguments(process_t * process, void * eip) {
	uint32_t suicide = (uint32_t) process_suicide;
	multitasking_push(process, &suicide, 4);
	multitasking_push(process, &eip, 4);
}

void process_push_args(process_t * process, int argc, char * argv[]) {
	uint32_t eip;
	if ((argc == 0) || (argv == NULL)) {
		argc = 0;
		multitasking_pop(process, &eip, 4); // pop process_thumb()'s return address
		multitasking_push(process, &argc, 4); // slide argc under it
		multitasking_push(process, &eip, 4); // and throw the return address back on top
		return;
	}
	uint32_t * stack_argv = malloc(argc * 4);
	multitasking_pop(process, &eip, 4); // pop process_thumb()'s return address
	// now slide argc and argv and process under the address

	// push all the strings one after another
	int real_argc = 0; // keep track of how many we actually wrote
	for (int i = 0; i < argc; i++) {
		char * arg = argv[i];
		if (!arg) {
			break; // stop here
		}
		int length = strlen(arg) + 1; // include NULL byte

		// push this string's content
		multitasking_prepush(process, length);
		process->esp &= ~0b1111; // align to 16 (SSE alignment)
		stack_argv[i] = process->esp; // add to our new "argv"
		multitasking_postpush(process, arg, length); // pushy
		real_argc++; // we just pushed one so do this
	}
	// align to 16 (SSE alignment)
	process->esp &= ~0b1111;

	// now push the array of pointers
	multitasking_prepush(process, real_argc * 4);
	memcpy32((void *) process->esp, stack_argv, real_argc);

	multitasking_push(process, &real_argc, 4); // throw argc into the stack
	multitasking_push(process, &eip, 4); // and slide the return address back on top
	free(stack_argv); // gooby
}

process_t * create_process(uint16_t * name, void * eip) {
	process_t * process = alloc_process(name);
	process_create_stack(process, stack_size);
	process_push_startup_arguments(process, eip);
	append_process(process);
	process->killed = 0;
	return process;
}

process_t * create_process_sized(uint16_t * name, void * eip, uint32_t stack) {
	process_t * process = alloc_process(name);
	process_create_stack(process, stack);
	process_push_startup_arguments(process, eip);
	append_process(process);
	process->killed = 0;
	return process;
}

process_t * create_process_paused(uint16_t * name, void * eip) {
	process_t * process = alloc_process(name);
	process_create_stack(process, stack_size);
	process_push_startup_arguments(process, eip);
	append_process(process);
	return process;
}

process_t * create_process_nostack(uint16_t * name) {
	process_t * process = alloc_process(name);
	append_process(process);
	return process;
}

void multitasking_track_allocation(process_t * process, void * alloc) {
	// the arrayification of the OS will happen soon
	// just to note: process->allocs should NEVER be a dynamic array
	linked_t * top = process->allocs_top;
	if (!top) {
		process->allocs = linked_add(process->allocs, alloc);
		process->allocs_top = process->allocs;
		return;
	}
	linked_t * node = linked_create(alloc);
	node->back = top;
	top->next = node;
	process->allocs_top = node;
}

void multitasking_untrack_allocation(process_t * process, void * alloc) {
	linked_t * top = process->allocs_top;
	if (top->p == alloc) {
		process->allocs_top = top->back;
		process->allocs = linked_delete(top);
		return;
	}
	linked_t * node = linked_find(process->allocs, linked_find_generic, alloc);
	process->allocs = linked_delete(node);
}

int multitasking_destroy_window(linked_t * node, void * p) {
	window_t * window = node->p;
	gfx_cleanup_window(window);
}

int multitasking_destroy_alloc(linked_t * node, void * p) {
	free(node->p);
}

void cleanup_process(process_t * process) {
	// cleanup everything this process did after its been killed
	free(process->name);
	if (sse_level != -1) {
		free(process->fpusave);
	}
	multitasking_free_stack(process->stack, process->stack_size);
	linked_destroy_all(process->windows, multitasking_destroy_window, 0);
	linked_destroy_all(process->allocs, multitasking_destroy_alloc, 0);
	free(process->allocs);
	free(process->windows);
	free(process);
}

// create dead process that listens for events
process_t * create_event_zombie(uint16_t * name, input_callback_t handler) {
	process_t * process = alloc_process(name);
	process->killed = 1;
	process->recv_global_event = handler;
	process->eip = 0xdeaddead;
	process->ebp = 0xdeaddead;
	process->esp = 0xdeaddead;
	process->stack = (void *) 0xdeaddead;
	process->stack_size = 0xdeaddead;
	if (sse_level != -1) {
		free(process->fpusave);
	}
	append_process(process);
	return process;
}

void multitasking_inc_taskp() {
	taskp++;
	if (taskp >= proc_count) {
		taskp = 0;
	}
}

int multitasking_kill_callback(linked_t * node, void * p) {
	process_t * process = node->p;
	uint64_t * pid = p;
	if (process->pid == *pid) {
		return 1;
	}
	return 0;
}

int ppause(process_t * proc, int paused) {
	proc->killed = paused;
}

int pause(uint64_t pid, int paused) {
	linked_t * node = linked_find(procs, multitasking_kill_callback, &pid);
	process_t * process;
	if (!node) {
		return -1;
	}
	return ppause(node->p, paused);
}

linked_t * multitasking_find_pid(uint64_t pid) {
	process_t * current_process = *get_current_process();
	if (pid == 0xffffffffffffffff) {
		pid = current_process->pid;
	}
	linked_t * node = linked_find(procs, multitasking_kill_callback, &pid);
	if (!node) {
		return NULL;
	}
	return node;
}

process_t * multitasking_get_pid(uint64_t pid) {
	linked_t * node = multitasking_find_pid(pid);
	process_t * process;
	if (!node) {
		return NULL;
	}
	process = (process_t *) node->p;
	return process;
}

process_t * get_next_step() {
	process_t * process;
	proc_step = proc_step->next;
	if (proc_step == 0) {
		proc_step = procs;
	}
	process = (process_t *) proc_step->p;
	while (process->killed == 1) {
		proc_step = proc_step->next;
		if (proc_step == 0) {
			proc_step = procs;
		}
		process = (process_t *) proc_step->p;
	}
	return process;
}

int kill(uint64_t pid, int signal) {
	int save = interrupts_enabled();
	disable_interrupts();
	linked_t * node = multitasking_find_pid(pid);
	process_t ** current_process = get_current_process();
	process_t * curprocess = *current_process;
	process_t * process;
	int iscurrent = 0;
	if (!node) {
		return -1;
	}
	if (signal == 0) {
		return 0;
	}
	process = (process_t *) node->p;

	// dont allow process to ignore signal 9, 18, or 19, unless it is system process
	if ((signal != 9 && signal != 18 && signal != 19) || process->system) {
		if (process->kill(signal) == -1) {
			return -1;
		}
	}
	if (signal == 18 || signal == 19) {
		pause(pid, signal == 19);
		return 0;
	}
	iscurrent = process->pid == curprocess->pid;
	process->killed = 1;
	if (iscurrent) {
		*current_process = get_next_step();
		curprocess = *current_process;
		*get_active_process() = curprocess->active_process;
	}
	procs = linked_delete(node);
	free(node);
	proc_count--;
	cleanup_process(process);
	if (iscurrent) {
		if (sse_level != -1) {
			asm volatile("fxrstor (%0)" :: "r"(curprocess->fpusave));
		}
		asm_task_switch(curprocess->eip, curprocess->ebp, curprocess->esp);
	}
	interrupts_restore(save);
}

void switch_task() {
	if ((proc_count | multitasking_done | (uintptr_t) procs) == 0 || proc_count == 1 || in_sleep_mode == 1) {
		return;
	}

	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	process_t ** current_process = get_current_process();
	process_t ** active_process = get_active_process();
	process_t * process = *current_process;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));
	if (sse_level != -1) {
		asm volatile("fxsave (%0); finit" :: "r"(process->fpusave));
	}
	eip = geteip();
	if (eip == 0) {
		return;
	}
	process->eip = eip;
	process->esp = esp;
	process->ebp = ebp;
	process->active_process = *active_process;
	process = get_next_step();

	eip = process->eip;
	esp = process->esp;
	ebp = process->ebp;
	*current_process = process;
	*active_process = process->active_process;
	if (sse_level != -1) {
		asm volatile("fxrstor (%0)" :: "r"(process->fpusave));
	}
	asm_task_switch(eip, ebp, esp);
}

// holy
int multitasking_fork(uint32_t esp) {
	process_t * current_process = *get_current_process();
	uint16_t * name = malloc((ustrlen(current_process->name) + 4) * 2);
	process_t * proc;
	ustrcpy(name, current_process->name);
	ustrcat(name, u"-2");
	proc = create_process_paused(name, asm_fork_return);
	free(name);
	memcpy(proc->stack - proc->stack_size, current_process->stack - current_process->stack_size, proc->stack_size);
	proc->esp = ((uint32_t) proc->stack) - (((uint32_t) current_process->stack) - esp);
	proc->ebp = proc->esp;
	proc->system = current_process->system;
	proc->active_process = proc;
	proc->kill = current_process->kill; // should we do this?

	uint32_t fork_return = (uint32_t) asm_fork_return;
	multitasking_push(proc, &fork_return, 4);

	proc->killed = 0;
	return 1;
}

// push onto another process's stack
void multitasking_push(process_t * proc, void * buffer, int length) {
	proc->esp -= length;
	memcpy((void *) proc->esp, buffer, length);
}

// move esp prior to push
void multitasking_prepush(process_t * proc, int length) {
	proc->esp -= length;
}

// now push
void multitasking_postpush(process_t * proc, void * buffer, int length) {
	memcpy((void *) proc->esp, buffer, length);
}

// push from another process's stack
void multitasking_pop(process_t * proc, void * buffer, int length) {
	memcpy(buffer, (void *) proc->esp, length);
	proc->esp += length;
}

void multitasking_handle_exception(int exception, registers_t * regs) {
	disable_interrupts(); // obviously

	// if the process is a system process, panic instead of killing
	process_t * current_process = *get_current_process();
	current_process->killed = 1;
	if (current_process->system != 0) {
		panic(exception, regs);
		return;
	}

	printf(u"MURDERING %s FOR %s\n", current_process->name, error_name(exception));
	kill(0xffffffffffffffff, 9);
}

void multitasking_init() {
	// :/
	current_processes = malloc(cpus * sizeof(process_t *));
	active_processes = malloc(cpus * sizeof(process_t *));
	process_t ** current_process = get_current_process();
	process_t ** active_process = get_active_process();
	process_t * process = malloc(sizeof(process_t));
	*active_process = process;
	*current_process = process;
	memset(process, 0, sizeof(process_t));
	process->name = ustrdup(u"init");
	process->personality = get_personality_by_type(PERSONALITY_LINUX);
	process->active_process = process;
	process->system = 1;
	process->killed = 0;
	process->quantum = 1;
	process->recv_event = multitasking_default_event_handler;
	process->recv_global_event = multitasking_default_event_handler;
	process->kill = force_alive_kill_handler;
	if (sse_level != -1) {
		process->fpusave = malloc(512);
		memset(process->fpusave, 0, 512);
	}
	hook_stdout(process, ansi_stdout_handler, ansi_stdout_constructor);

	pid_top++;
	proc_count++;
	procs = linked_add(procs, process);
	proc_step = procs;
	multitasking_done = 1;
}

void * multitasking_alloc_stack(uintptr_t size) {
	return malloc(size) + size;
}

int multitasking_free_stack(void * stack, uintptr_t size) {
	return free(stack - size);
}
