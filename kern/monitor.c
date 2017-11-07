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
#include <kern/trap.h>

#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "showmappings", "Display the physical page mappings and corresponding perm bits", showmappings },
	{ "setp", "Explicitly set, clear, or change the permissions of any mapping in the current address space.", setp },
	{ "dumpmem", "Dump the contents of a range of memory given either a virtual or physical address range. Be sure the dump code behaves correctly when the range extends across page boundaries!", dumpmem },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Please see nox interface rather than qemu interface!!!\n\n\033[0m\033[48;5;188m   \033[48;5;188m\033[38;5;143m▄\033[48;5;188m\033[38;5;179m▄\033[48;5;188m      \033[48;5;187m \033[48;5;187m\033[38;5;186m▄\033[48;5;180m \033[48;5;179m\033[38;5;223m▄\033[48;5;179m\033[38;5;186m▄\033[48;5;186m    \033[0m\n\033[48;5;186m\033[38;5;186m\033[48;5;188m  \033[48;5;224m\033[38;5;188m▄\033[48;5;137m \033[48;5;180m\033[38;5;137m▄\033[48;5;180m\033[38;5;143m▄\033[48;5;188m\033[38;5;179m▄\033[48;5;188m\033[38;5;223m▄\033[48;5;224m\033[38;5;187m▄\033[48;5;188m\033[38;5;186m▄\033[48;5;188m\033[38;5;180m▄\033[48;5;187m\033[38;5;179m▄\033[48;5;179m\033[38;5;180m▄\033[48;5;180m\033[38;5;137m▄\033[48;5;222m\033[38;5;180m▄\033[48;5;223m\033[38;5;179m▄\033[48;5;223m\033[38;5;186m▄\033[48;5;222m\033[38;5;186m▄\033[48;5;222m \033[48;5;186m\033[38;5;223m▄\033[0m\n\033[48;5;186m\033[38;5;223m\033[48;5;188m  \033[48;5;188m\033[38;5;187m▄\033[48;5;137m \033[48;5;95m\033[38;5;101m▄\033[48;5;137m\033[38;5;58m▄\033[48;5;137m \033[48;5;137m\033[38;5;180m▄\033[48;5;180m  \033[48;5;179m\033[38;5;180m▄ \033[48;5;180m \033[48;5;186m\033[38;5;187m▄\033[48;5;180m\033[38;5;186m▄\033[48;5;180m\033[38;5;222m▄\033[48;5;143m\033[38;5;186m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;143m  \033[0m\n\033[48;5;143m\033[38;5;143m\033[48;5;188m\033[38;5;224m▄\033[48;5;224m\033[38;5;188m▄\033[48;5;223m\033[38;5;187m▄\033[48;5;143m\033[38;5;137m▄\033[48;5;179m \033[48;5;179m\033[38;5;186m▄\033[48;5;180m \033[48;5;180m\033[38;5;95m▄\033[48;5;180m\033[38;5;16m▄ \033[48;5;187m\033[38;5;16m▄\033[48;5;180m\033[38;5;16m▄\033[48;5;179m\033[38;5;16m▄\033[48;5;187m\033[38;5;0m▄\033[48;5;144m\033[38;5;102m▄\033[48;5;16m  \033[48;5;16m\033[38;5;187m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;144m\033[38;5;187m▄\033[0m\n\033[48;5;144m\033[38;5;187m\033[48;5;188m \033[48;5;224m\033[38;5;188m▄\033[48;5;186m\033[38;5;180m▄\033[48;5;94m\033[38;5;179m▄\033[48;5;180m \033[48;5;180m\033[38;5;187m▄\033[48;5;16m\033[38;5;187m▄ \033[48;5;16m  \033[48;5;16m\033[38;5;138m▄\033[48;5;16m\033[38;5;180m▄\033[48;5;186m \033[48;5;137m\033[38;5;187m▄\033[48;5;186m\033[38;5;187m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;224m\033[38;5;230m▄\033[48;5;224m \033[48;5;187m  \033[0m\n\033[48;5;187m\033[38;5;187m\033[48;5;224m\033[38;5;188m▄\033[48;5;188m\033[38;5;187m▄\033[48;5;180m \033[48;5;180m\033[38;5;179m▄\033[48;5;187m\033[38;5;180m▄\033[48;5;187m      \033[48;5;180m \033[48;5;187m\033[38;5;181m▄\033[48;5;187m \033[48;5;59m \033[48;5;58m\033[38;5;59m▄\033[48;5;59m \033[48;5;187m   \033[0m\n\033[48;5;187m\033[38;5;187m \033[48;5;186m\033[38;5;180m▄\033[48;5;179m \033[48;5;143m\033[38;5;179m▄\033[48;5;180m \033[48;5;187m \033[48;5;180m\033[38;5;187m▄\033[48;5;187m\033[38;5;180m▄\033[48;5;181m\033[38;5;187m▄\033[48;5;187m\033[38;5;180m▄ \033[48;5;180m\033[38;5;58m▄\033[48;5;180m\033[38;5;138m▄\033[48;5;145m\033[38;5;101m▄\033[48;5;59m \033[48;5;95m\033[38;5;59m▄\033[48;5;102m\033[38;5;144m▄\033[48;5;187m  \033[48;5;181m\033[38;5;187m▄\033[0m\n\033[48;5;181m\033[38;5;187m\033[48;5;224m\033[38;5;223m▄\033[48;5;180m\033[38;5;179m▄\033[48;5;137m\033[38;5;179m▄\033[48;5;179m\033[38;5;137m▄\033[48;5;179m \033[48;5;180m \033[48;5;186m\033[38;5;180m▄\033[48;5;187m  \033[48;5;180m\033[38;5;181m▄\033[48;5;180m \033[48;5;180m\033[38;5;144m▄\033[48;5;144m\033[38;5;180m▄  \033[48;5;137m\033[38;5;187m▄\033[48;5;187m \033[48;5;223m\033[38;5;187m▄\033[48;5;224m\033[38;5;223m▄ \033[0m\n\033[48;5;224m\033[38;5;223m\033[48;5;187m\033[38;5;223m▄\033[48;5;180m\033[38;5;186m▄\033[48;5;180m \033[48;5;179m\033[38;5;180m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;180m  \033[48;5;180m\033[38;5;186m▄\033[48;5;187m\033[38;5;180m▄ \033[48;5;180m    \033[48;5;186m \033[48;5;187m\033[38;5;180m▄\033[48;5;186m \033[48;5;187m \033[48;5;223m\033[38;5;187m▄ \033[0m\n\033[48;5;223m\033[38;5;187m\033[48;5;187m \033[48;5;186m\033[38;5;180m▄\033[48;5;187m   \033[48;5;180m\033[38;5;187m▄ \033[48;5;180m\033[38;5;144m▄\033[48;5;180m\033[38;5;187m▄\033[48;5;187m \033[48;5;180m\033[38;5;181m▄\033[48;5;144m\033[38;5;180m▄\033[48;5;143m\033[38;5;180m▄\033[48;5;180m  \033[48;5;186m\033[38;5;187m▄\033[48;5;187m \033[48;5;187m\033[38;5;223m▄\033[48;5;187m \033[0m\n", mon_backtrace },
};



uint32_t xtoi(char* buf) {
	uint32_t res = 0;
	buf += 2;
	while (*buf) { 
		if (*buf >= 'a') *buf = *buf-'a'+'0'+10;
		res = res*16 + *buf - '0';
		++buf;
	}
	return res;
}

void pprint(pte_t *pte) {
	cprintf("PTE_P: %x, PTE_W: %x, PTE_U: %x\n", 
		*pte&PTE_P, *pte&PTE_W, *pte&PTE_U);
}

int
showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if (argc == 1) {
		cprintf("Usage: showmappings 0xbegin_addr 0xend_addr\n");
		return 0;
	}
	uint32_t begin = xtoi(argv[1]), end = xtoi(argv[2]);
	for (; begin <= end; begin += PGSIZE) {
		pte_t *pte = pgdir_walk(kern_pgdir, (void *) begin, 1);
		if (!pte) 
			panic("boot_map_region panic, out of memory");
		if (*pte & PTE_P) {
			cprintf("page %x with ", begin);
			pprint(pte);
		} 
		else 
			cprintf("page not exist: %x\n", begin);
	}
	return 0;
}

int 
setp(int argc, char **argv, struct Trapframe *tf) {
	if (argc == 1) {
		cprintf("Usage: setp0xaddr [c|s :clear or set] [P|W|U]\n");
		return 0;
	}
	uint32_t addr = xtoi(argv[1]);
	pte_t *pte = pgdir_walk(kern_pgdir, (void *)addr, 1);
	cprintf("%x before setp: ", addr);
	pprint(pte);
	uint32_t perm = 0;
	if (argv[3][0] == 'P') 
		perm = PTE_P;
	else if (argv[3][0] == 'W') 
		perm = PTE_W;
	else if (argv[3][0] == 'U') 
		perm = PTE_U;
	if (argv[2][0] == 'c')
		*pte = *pte & ~perm;
	else if(argv[2][0] == 's')
		*pte = *pte | perm;
	else{
		cprintf("Usage: setp0xaddr [c|s :clear or set] [P|W|U]\n");
		return 0;
	}
	cprintf("%x after  setp: ", addr);
	pprint(pte);
	return 0;
}


int 
dumpmem(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 4) {
		cprintf("Usage: dumpmem [v|p : virtual address or physical address] [LOWER_ADDR] [DWORD]    \n");
		return 0;
	}
	uint32_t la, ua;
	int i;
	if (argv[1][0] == 'p') {
		la = strtol(argv[2], 0, 0) + KERNBASE;
		ua = la + strtol(argv[3], 0, 0) * 4;
	}
	else if (argv[1][0] == 'v') {
		la = strtol(argv[2], 0, 0);
		ua = la + strtol(argv[3], 0, 0) * 4;
	}
	else {
		cprintf("Invalid ADDR TYPE!\n");
		return 0;
	}
	if (la >= ua ||
		la != ROUNDUP(la, 4) ||
		ua != ROUNDUP(ua, 4)) {
		cprintf("Invalid ADDR (0x%x, 0x%x)!\n", la, ua);
		return 0;
	}
	for (i = 0; la < ua; la += 4) {
		if (!(i % 4))
			cprintf("\n0x%x: ", la);
		cprintf("0x%x\t", *((uint32_t *)(la)));
		i++;
	}
	cprintf("\n");
	return 0;
}



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

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
