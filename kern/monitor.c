// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Please see nox interface rather than qemu interface!!!\n\n\033[0m\033[48;5;188m   \033[48;5;188m\033[38;5;143m▄\033[48;5;188m\033[38;5;179m▄\033[48;5;188m      \033[48;5;187m \033[48;5;187m\033[38;5;186m▄\033[48;5;180m \033[48;5;179m\033[38;5;223m▄\033[48;5;179m\033[38;5;186m▄\033[48;5;186m    \033[0m\n\033[48;5;186m\033[38;5;186m\033[48;5;188m  \033[48;5;224m\033[38;5;188m▄\033[48;5;137m \033[48;5;180m\033[38;5;137m▄\033[48;5;180m\033[38;5;143m▄\033[48;5;188m\033[38;5;179m▄\033[48;5;188m\033[38;5;223m▄\033[48;5;224m\033[38;5;187m▄\033[48;5;188m\033[38;5;186m▄\033[48;5;188m\033[38;5;180m▄\033[48;5;187m\033[38;5;179m▄\033[48;5;179m\033[38;5;180m▄\033[48;5;180m\033[38;5;137m▄\033[48;5;222m\033[38;5;180m▄\033[48;5;223m\033[38;5;179m▄\033[48;5;223m\033[38;5;186m▄\033[48;5;222m\033[38;5;186m▄\033[48;5;222m \033[48;5;186m\033[38;5;223m▄\033[0m\n\033[48;5;186m\033[38;5;223m\033[48;5;188m  \033[48;5;188m\033[38;5;187m▄\033[48;5;137m \033[48;5;95m\033[38;5;101m▄\033[48;5;137m\033[38;5;58m▄\033[48;5;137m \033[48;5;137m\033[38;5;180m▄\033[48;5;180m  \033[48;5;179m\033[38;5;180m▄ \033[48;5;180m \033[48;5;186m\033[38;5;187m▄\033[48;5;180m\033[38;5;186m▄\033[48;5;180m\033[38;5;222m▄\033[48;5;143m\033[38;5;186m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;143m  \033[0m\n\033[48;5;143m\033[38;5;143m\033[48;5;188m\033[38;5;224m▄\033[48;5;224m\033[38;5;188m▄\033[48;5;223m\033[38;5;187m▄\033[48;5;143m\033[38;5;137m▄\033[48;5;179m \033[48;5;179m\033[38;5;186m▄\033[48;5;180m \033[48;5;180m\033[38;5;95m▄\033[48;5;180m\033[38;5;16m▄ \033[48;5;187m\033[38;5;16m▄\033[48;5;180m\033[38;5;16m▄\033[48;5;179m\033[38;5;16m▄\033[48;5;187m\033[38;5;0m▄\033[48;5;144m\033[38;5;102m▄\033[48;5;16m  \033[48;5;16m\033[38;5;187m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;144m\033[38;5;187m▄\033[0m\n\033[48;5;144m\033[38;5;187m\033[48;5;188m \033[48;5;224m\033[38;5;188m▄\033[48;5;186m\033[38;5;180m▄\033[48;5;94m\033[38;5;179m▄\033[48;5;180m \033[48;5;180m\033[38;5;187m▄\033[48;5;16m\033[38;5;187m▄ \033[48;5;16m  \033[48;5;16m\033[38;5;138m▄\033[48;5;16m\033[38;5;180m▄\033[48;5;186m \033[48;5;137m\033[38;5;187m▄\033[48;5;186m\033[38;5;187m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;224m\033[38;5;230m▄\033[48;5;224m \033[48;5;187m  \033[0m\n\033[48;5;187m\033[38;5;187m\033[48;5;224m\033[38;5;188m▄\033[48;5;188m\033[38;5;187m▄\033[48;5;180m \033[48;5;180m\033[38;5;179m▄\033[48;5;187m\033[38;5;180m▄\033[48;5;187m      \033[48;5;180m \033[48;5;187m\033[38;5;181m▄\033[48;5;187m \033[48;5;59m \033[48;5;58m\033[38;5;59m▄\033[48;5;59m \033[48;5;187m   \033[0m\n\033[48;5;187m\033[38;5;187m \033[48;5;186m\033[38;5;180m▄\033[48;5;179m \033[48;5;143m\033[38;5;179m▄\033[48;5;180m \033[48;5;187m \033[48;5;180m\033[38;5;187m▄\033[48;5;187m\033[38;5;180m▄\033[48;5;181m\033[38;5;187m▄\033[48;5;187m\033[38;5;180m▄ \033[48;5;180m\033[38;5;58m▄\033[48;5;180m\033[38;5;138m▄\033[48;5;145m\033[38;5;101m▄\033[48;5;59m \033[48;5;95m\033[38;5;59m▄\033[48;5;102m\033[38;5;144m▄\033[48;5;187m  \033[48;5;181m\033[38;5;187m▄\033[0m\n\033[48;5;181m\033[38;5;187m\033[48;5;224m\033[38;5;223m▄\033[48;5;180m\033[38;5;179m▄\033[48;5;137m\033[38;5;179m▄\033[48;5;179m\033[38;5;137m▄\033[48;5;179m \033[48;5;180m \033[48;5;186m\033[38;5;180m▄\033[48;5;187m  \033[48;5;180m\033[38;5;181m▄\033[48;5;180m \033[48;5;180m\033[38;5;144m▄\033[48;5;144m\033[38;5;180m▄  \033[48;5;137m\033[38;5;187m▄\033[48;5;187m \033[48;5;223m\033[38;5;187m▄\033[48;5;224m\033[38;5;223m▄ \033[0m\n\033[48;5;224m\033[38;5;223m\033[48;5;187m\033[38;5;223m▄\033[48;5;180m\033[38;5;186m▄\033[48;5;180m \033[48;5;179m\033[38;5;180m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;180m  \033[48;5;180m\033[38;5;186m▄\033[48;5;187m\033[38;5;180m▄ \033[48;5;180m    \033[48;5;186m \033[48;5;187m\033[38;5;180m▄\033[48;5;186m \033[48;5;187m \033[48;5;223m\033[38;5;187m▄ \033[0m\n\033[48;5;223m\033[38;5;187m\033[48;5;187m \033[48;5;186m\033[38;5;180m▄\033[48;5;187m   \033[48;5;180m\033[38;5;187m▄ \033[48;5;180m\033[38;5;144m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;187m \033[48;5;180m\033[38;5;181m▄\033[48;5;144m\033[38;5;180m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;180m  \033[48;5;186m\033[38;5;187m▄\033[48;5;187m \033[48;5;187m\033[38;5;223m▄\033[48;5;187m \033[0m\n", mon_backtrace },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Stack backtrace:\n");
	uint32_t ebp = read_ebp(), eip = *((uint32_t *)ebp + 1);
	struct Eipdebuginfo eipinfo;
	for (; ebp; ebp = *(uint32_t *)ebp) {
		eip = *((uint32_t *)ebp + 1);
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", \
			ebp, \
			eip, \
			*((uint32_t *)ebp + 2), \
			*((uint32_t *)ebp + 3), \
			*((uint32_t *)ebp + 4), \
			*((uint32_t *)ebp + 5), \
			*((uint32_t *)ebp + 6));
		
		debuginfo_eip(eip, &eipinfo);
		cprintf("         %s:%d: %.*s+%d\n", \
			eipinfo.eip_file, \
			eipinfo.eip_line, \
			eipinfo.eip_fn_namelen, \
			eipinfo.eip_fn_name, \
			eip - eipinfo.eip_fn_addr);
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");
	

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
