#include <personalities.h>
#include <personalities/linux/syscall.h>

#include <stdint.h>
#include <stddef.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdarg.h>
#include <pit.h>
#include <interrupts/irq.h>
#include <memory.h>
#include <syscall.h>
#include <input/input.h>
#include <multitasking.h>
#include <util.h>
#include <string.h>
#include <uname.h>
#include <terminal.h>
#include <drivers/thermal/thermal.h>
#include <multiboot.h>
#include <cpuid.h>
#include <sysinfo.h>
#include <graphics/font.h>
#include <unicode.h>
#include <input/layout.h>
#include <net/socket.h>
#include <net/arp.h>
#include <bin_formats/elf.h>
#include <time.h>
#include <graphics/displays.h>
#include <graphics/gpu.h>

static personality_t personality;

void personality_linux_init() {
	personality_register(&personality);
}

void do_nothing_linux_syscall(registers_t * regs) {}

void exit_linux_syscall(registers_t * regs) {
	kill(0xffffffffffffffff, 9);
	enable_interrupts();
	yield();
	sleep(); // wait to die
}

void fork_linux_syscall(registers_t * regs) {
	regs->eax = 0;
	if (fork() == 1) {
		regs->eax = 1;
	}
}

void clone_linux_syscall(registers_t * regs) {
	
}

void read_linux_syscall(registers_t * regs) {
	void * buffer = (void *) regs->ecx;
	size_t len = regs->edx;
	uint32_t fd = regs->ebx;
	size_t read = 0;
	process_t * current_process = *get_current_process();
	if (fd == 0) {
		enable_interrupts();
		while (read < len) {
			ssize_t t = current_process->stdin_handler(current_process, buffer, len);
			if (t < 1) {
				yield();
				continue;
			}
			buffer += t;
			read += t;
			len -= t;
		}
		disable_interrupts();
	}
}

// the worst
void write_linux_syscall(registers_t * regs) {
	char * text = (char *) regs->ecx;
	size_t len = regs->edx;
	uint32_t fd = regs->ebx;
	char * temp = NULL;
	size_t stringlen;
	if (fd < 3) {
		if (fd == 0) {
			regs->eax = 0;
			return;
		}
		process_t * current_process = *get_current_process();
		if (((uint8_t) text[len - 1]) & 0b10000000) {
			if (current_process->expecting) {
				temp = malloc(current_process->expecting_length + len);
				memcpy(temp, current_process->expecting, current_process->expecting_length);
				memcpy(temp + current_process->expecting_length, text, len);
				free(current_process->expecting);
				current_process->expecting = temp;
				current_process->expecting_length += len;
				return;
			}
			current_process->expecting = malloc(len);
			current_process->expecting_length = len;
			memcpy(current_process->expecting, text, len);
			return;
		}
		if (current_process->expecting) {
			temp = malloc(len + current_process->expecting_length);
			memcpy(temp, current_process->expecting, current_process->expecting_length);
			memcpy(temp + current_process->expecting_length, text, len);
			len += current_process->expecting_length;
			text = temp;
			free(current_process->expecting);
			current_process->expecting = NULL;
		}
		uint16_t * unicode = malloc((len * 2) + 4); // we use UTF-16 internally, so do this
		utf8toutf16l(text, unicode, len);
		regs->eax = current_process->stdout_handler(current_process, unicode, ustrlen(unicode));
		free(unicode);
		if (temp) {
			free(temp);
		}
		return;
	}
	fd_t * fds = find_fd(fd);
	if (!fds) {
		regs->eax = -1;
		return;
	}
	fds->write(fds, text, len);
	regs->eax = len;
}

void getpid_linux_syscall(registers_t * regs) {
	process_t * current_process = *get_current_process();
	regs->eax = current_process->pid;
}

void uname_linux_syscall(registers_t * regs) {
	uname_t * name = (uname_t *) regs->ebx;
	uname(name);
}

void gettherm_linux_syscall(registers_t * regs) {
	regs->eax = get_cpu_temp();
}

void lctl_linux_syscall(registers_t * regs) {
	switch (regs->ebx) {
		case LCTL_MULTIBOOT_MAGIC: // if eax is MULTIBOOT_MAGIC return the header
			regs->eax = (uint32_t) multiboot_header;
			return;
		case LCTL_CPU_TYPE_MAGIC: // if eax is CPU_TYPE_MAGIC return the cpu type
			regs->eax = cpu_type;
			return;
		case LCTL_CPU_VENDOR_MAGIC: // if eax is CPU_TYPE_MAGIC return the cpu vendor
			regs->eax = (uint32_t) cpu_vendor8; // return cpu vendor as 8 bit char array (ours is usually 16 bit)
			return;
		case LCTL_PERF_STATE_MAGIC:
			perf_state_t state; // write the state of the perf driver down
			state.f.fstring = perf_intel_fstring;
			state.f.speedstep = perf_intel_speedstep;
			state.f.turbo = perf_intel_turbo;
			state.f.deephalt = perf_deep_halt_supported;
			state.f.monitorless = perf_monitorless;
			regs->eax = state.i; // return it to the caller as an integer
			return;
		case LCTL_REQUEST_FRAMEBUFFER:
			framebuffer_spec_t * spec = (void *) regs->ecx;
			spec->fb = root_window.fb;
			spec->width = root_window.size.width;
			spec->height = root_window.size.height;
			spec->bpp = 32;
			regs->eax = (uint32_t) spec;
			return;
		case LCTL_TIMER_SET_FREQUENCY:
			if (regs->ecx == pit_freq) {
				return; // yeah sure let me do that for you
			}
			pit_disable();
			pit_init(regs->ecx);
			return;
		case LCTL_TIMER_FREQUENCY:
			regs->eax = (uint32_t) pit_freq;
			return;
		case LCTL_TIMER_TIMESTAMP:
			regs->eax = (uint32_t) &ticks;
			return;
		case LCTL_GET_LAYOUT:
			int layer = regs->ecx;
			uint16_t (* layout)[][98] = layout_get_layer(layer);
			regs->eax = (uint32_t) layout;
			return;
		case LCTL_FONT_GET_BITMAPS:
			regs->eax = (uint32_t) font;
			return;
		case LCTL_FONT_GET_SIZE:
			regs->eax = (uint32_t) font_dword_size;
			return;
		case LCTL_INITRD_LOAD: // :/
			char * name = (char *) regs->ecx;
			int argc = regs->edx;
			char ** argv = (char **) regs->esi;
			regs->eax = exec_initrd(name, argc, argv);
			return;
		case LCTL_GET_PCB:
			regs->eax = (uint32_t) multitasking_get_pid(regs->ecx);
			return;
		case LCTL_GET_PERSONALITY:
			regs->eax = (uint32_t) get_personality_by_type(regs->ecx);
			return;
		case LCTL_GET_NAMED_PERSONALITY:
			regs->eax = (uint32_t) get_personality_by_name((uint16_t *) regs->ecx);
			return;
		case LCTL_DISPLAY_LISTEN: {
			display_t * display = (display_t *) regs->ecx;
			display_listen_t listener = (display_listen_t) regs->edx;
			void * priv = (void *) regs->esi;
			display_listen(display, listener, priv);
			regs->eax = 0; // we should figure out an error code for you
			return;
		}
		case LCTL_DISPLAY_GET_DEFAULT:
			regs->eax = (uint32_t) display_get_default();
			return;
		case LCTL_DISPLAY_RESIZE: {
			display_t * display = (display_t *) regs->ecx;
			int width = (int) regs->edx;
			int height = (int) regs->esi;
			display_resize(display, width, height);
			regs->eax = 0; // we should figure out an error code for you
			return;
		}
		case LCTL_DISPLAY_CRUNCH: {
			display_t * display = (display_t *) regs->ecx;
			int bpp = (int) regs->edx;
			display_crunch(display, bpp);
			regs->eax = 0; // we should figure out an error code for you
			return;
		}
        case LCTL_GPU_AUTOSELECT: {
			regs->eax = (uint32_t) gpu_get_default();
			return;
		}
        case LCTL_GPU_ALLOC: {
			// allocate VRAM (todo...)
			regs->eax = 0x12c000;
			return;
		}
        case LCTL_GPU_CALL: {
			switch (regs->ecx) {
				case GPUCAP_FUNC_RECT_FILL:
					gpu_surface_t * surface = (gpu_surface_t *) regs->edx;
					// gpu_rect_fill(/* how */, surface->fb, surface->width, surface->height, surface->bpp, /* how */);
					return;
			}
			return;
		}
        case LCTL_GPU_GETCAP: {
			return;
		}
	}
}

int socket_write_fd(fd_t * fd, void * buffer, uint32_t size) {
	if (fd->type != FD_SOCKET) {
		return -1;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	if (!sockfd->socket || !sockfd->client) {
		return -1;
	}
	if ((sockfd->flags & FD_SOCKET_FLAG_SERVER) != 0) {
		return -1;
	}
	if ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) == 0) {
		return -1;
	}
	socket_write(sockfd->client, buffer, size);
	return size;
}

void socket_linux_syscall(registers_t * regs) {
	if (regs->ebx != AF_INET || regs->edx != 0) {
		regs->eax = -1; // we only support IPv4 protocol
		return;
	}
	switch (regs->ecx) {
		case SOCK_DGRAM:
			fd_socket_t * fd = (fd_socket_t *) create_fd(FD_SOCKET, socket_write_fd);
			if (!fd) {
				regs->eax = -1;
				return;
			}
			fd->protocol = SOCK_DGRAM;
			fd->timeout = 0;
			regs->eax = fd->number;
			return;
	}
	regs->eax = -1;
}

void bind_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	if (((sockfd->flags & FD_SOCKET_FLAG_SERVER) != 0) || ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) != 0)) {
		regs->eax = -1;
		return;
	}
	sockaddr_t * sockaddr = (sockaddr_t *) regs->ecx;
	socket_t * socket = socket_open(SOCKET_UDP, ntohw(sockaddr->sin_port), 0, 0);
	socket_timeout(socket, sockfd->timeout);
	sockfd->socket = socket;
	sockfd->flags |= FD_SOCKET_FLAG_SERVER;
	regs->eax = 0;
}

void connect_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	if (((sockfd->flags & FD_SOCKET_FLAG_SERVER) != 0) || ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) != 0)) {
		regs->eax = -1;
		return;
	}
	// connect to server (as client)
	sockaddr_t * sockaddr = (sockaddr_t *) regs->ecx;
	int port = net_request_ephemeral();
	socket_t * socket = socket_open(SOCKET_UDP, port, 0, 0);
	socket_set_connectionless(socket, sockfd->protocol == SOCK_DGRAM); // connectionless socket if protocol is UDP (to align with linux)
	socket_timeout(socket, sockfd->timeout);
	sockfd->socket = socket;
	sockfd->client = socket_connect(socket, sockaddr->sin_addr, ntohw(sockaddr->sin_port));
	sockfd->flags |= FD_SOCKET_FLAG_CONNECTED;
	regs->eax = 0;
}


void getsockopt_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	regs->eax = 0;
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	if (regs->ecx != SOL_SOCKET) {
		regs->eax = -1;
		return;
	}
	socket_t * socket = sockfd->socket;
	switch (regs->ecx) {
		case SO_RCVTIMEO:
			timeval_t * tv = (timeval_t *) regs->esi;
			tv->seconds = sockfd->timeout;
			return;
	}
}

void setsockopt_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	regs->eax = 0;
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	if (regs->ecx != SOL_SOCKET) {
		regs->eax = -1;
		return;
	}
	socket_t * socket = sockfd->socket;
	switch (regs->edx) {
		case SO_RCVTIMEO:
			timeval_t * tv = (timeval_t *) regs->esi;
			sockfd->timeout = tv->seconds;
			if (socket) {
				socket_timeout(socket, tv->seconds);
			}
			return;
	}
}

void sendto_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	//if (((sockfd->flags & FD_SOCKET_FLAG_SERVER) == 0) || ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) != 0)) {
	//	regs->eax = -1;
	//	return;
	//}
	socket_t * socket = sockfd->socket;
	sockaddr_t * sockaddr = (sockaddr_t *) regs->edi;
	socket_client_t * client = socket_get_client(socket, sockaddr->sin_addr, ntohw(sockaddr->sin_port));
	void * buffer = (void *) regs->ecx;
	size_t size = (size_t) regs->edx;
	socket_write(client, buffer, size);
}

void recvfrom_linux_syscall(registers_t * regs) {
	fd_t * fd = find_fd(regs->ebx);
	if (!fd || fd->type != FD_SOCKET) {
		regs->eax = -1;
		return;
	}
	fd_socket_t * sockfd = (fd_socket_t *) fd;
	socket_t * socket = sockfd->socket;
	socket_packet_t * packet = NULL;
	void * buffer = (void *) regs->ecx;
	size_t size = (size_t) regs->edx;
	sockaddr_t * sockaddr = (sockaddr_t *) regs->edi;
	if (((sockfd->flags & FD_SOCKET_FLAG_SERVER) != 0) && ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) == 0)) {
		packet = socket_any_poll(socket);
	} else if (((sockfd->flags & FD_SOCKET_FLAG_SERVER) == 0) && ((sockfd->flags & FD_SOCKET_FLAG_CONNECTED) != 0)) {
		socket_client_t * client = sockfd->client;
		packet = socket_poll(client);
	}
	if (size > packet->size) {
		size = packet->size;
	}
	if (sockaddr) {
		sockaddr->sin_family = AF_INET;
		sockaddr->sin_addr = packet->ip;
		sockaddr->sin_port = ntohw(packet->port);
	}
	memcpy(buffer, packet->data, size);
	socket_free_packet(packet);
	regs->eax = size;
	return;
}

void prctl_linux_syscall(registers_t * regs) {
	process_t * current_process = *get_current_process();
	switch (regs->ebx) {
		case PR_GET_NAME:
			return;
		case PR_SET_HANDLER: {
			input_callback_t handler = (input_callback_t) regs->edx;
			switch (regs->ecx) {
				default:
					return;
				case PR_HANDLER_GLOBAL:
					current_process->recv_global_event = handler;
					return;
				case PR_HANDLER_PRIVATE:
					current_process->recv_event = handler;
					return;
			}
			return;
		}
		case PR_HOOK_STDOUT: {
			uint64_t pid = ((uint64_t) regs->ecx) | (((uint64_t) regs->edx) << 32);
			stdout_handler_t handler = (stdout_handler_t) regs->esi;
			stdout_constructor_t constructor = (stdout_constructor_t) regs->edi;
			process_t * process = multitasking_get_pid(pid);
			// printf(u"hooking: %s\n", process->name);
			hook_stdout(process, handler, constructor);
			return;
		}
	}
}

void getlocale_linux_syscall(registers_t * regs) {
	regs->eax = 0; // this syscall no longer exists but userland wants it
}

void ipc_linux_syscall(registers_t * regs) {
	uint64_t pid = (uint64_t) regs->ebx | ((uint64_t) regs->ecx << 32);
	ipc_event_t * event;
	process_t * proc = multitasking_get_pid(pid);
	if (!proc) {
		regs->eax = -1;
		return;
	}

	process_t * current_process = *get_current_process();
	event = malloc(sizeof(ipc_event_t));
	event->type = EVENT_IPC;
	event->cmd = regs->edx;
	event->data = (void *) regs->esi;
	event->from = current_process;
	send_event((event_t *) event, proc);
	free(event);
	regs->eax = 0;
	return;
}

void sysinfo_linux_syscall(registers_t * regs) {
	sysinfo_t * sysinfo = (sysinfo_t *) regs->ebx;
	sysinfo->uptime = ticks / pit_freq;
	sysinfo->totalram = ram_size;
	sysinfo->freeram = ram_size - used_memory;
	sysinfo->sharedram = 0;
	sysinfo->bufferram = 0;
	sysinfo->totalswap = 0;
	sysinfo->freeswap = 0;
	sysinfo->procs = proc_count;
	sysinfo->totalhigh = ram_size;
	sysinfo->freehigh = ram_size - used_memory;
	sysinfo->mem_unit = 1;
	regs->eax = 0;
}

void sched_yield_linux_syscall(registers_t * regs) {
	yield();
	regs->eax = 0;
}

void malloc_linux_syscall(registers_t * regs) {
	void * alloc = malloc(regs->ebx);

	// track this so if they never free it doesnt leak
	multitasking_track_allocation(*get_current_process(), alloc);
	regs->eax = (uint32_t) alloc;
}

void free_linux_syscall(registers_t * regs) {
	void * alloc = (void *) regs->ebx;

	// goodbye!
	multitasking_untrack_allocation(*get_current_process(), alloc);
	regs->eax = free(alloc);
}

static personality_t personality = {
	PERSONALITY_LINUX, u"Linux", 23,
	{
		{SYSCALL_LINUX_NOTHING,			do_nothing_linux_syscall}, // non standard
		{SYSCALL_LINUX_EXIT,			exit_linux_syscall},
		{SYSCALL_LINUX_FORK,			fork_linux_syscall},
		{SYSCALL_LINUX_READ,			read_linux_syscall},
		{SYSCALL_LINUX_WRITE,			write_linux_syscall},
		{SYSCALL_LINUX_GETPID,			getpid_linux_syscall},
		{SYSCALL_LINUX_UNAME,			uname_linux_syscall},
		{SYSCALL_LINUX_GETTHERM,		gettherm_linux_syscall}, // non standard
		{SYSCALL_LINUX_IPC,				ipc_linux_syscall}, // non standard
		{SYSCALL_LINUX_SYSINFO,			sysinfo_linux_syscall},
		{SYSCALL_LINUX_CLONE,			clone_linux_syscall},
		{SYSCALL_LINUX_SCHED_YIELD,		sched_yield_linux_syscall},
		{SYSCALL_LINUX_PRCTL,			prctl_linux_syscall},
		{SYSCALL_LINUX_GETLOCALE,		getlocale_linux_syscall},
		{SYSCALL_LINUX_LCTL,			lctl_linux_syscall},
		{SYSCALL_LINUX_SOCKET,			socket_linux_syscall},
		{SYSCALL_LINUX_BIND,			bind_linux_syscall},
		{SYSCALL_LINUX_CONNECT,			connect_linux_syscall},
		{SYSCALL_LINUX_GETSOCKOPT,		getsockopt_linux_syscall},
		{SYSCALL_LINUX_SETSOCKOPT,		setsockopt_linux_syscall},
		{SYSCALL_LINUX_SENDTO,			sendto_linux_syscall},
		{SYSCALL_LINUX_RECVFROM,		recvfrom_linux_syscall},
		{SYSCALL_LINUX_MALLOC,			malloc_linux_syscall},
		{SYSCALL_LINUX_FREE,			free_linux_syscall}
	}
};
