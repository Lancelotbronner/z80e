#include <z80e/disassembler/disassemble.h>

#include <z80e/debugging/commands.h>
#include <z80e/debugging/debugger.h>
#include <z80e/ti/asic.h>

#include <stdio.h>

//MARK: - Disassemble Command

struct mmu_disassemble_memory {
	struct disassemble_memory mem;
	z80_cpu_t cpu;
	debugger_t debugger;
};

int disassemble_print(struct disassemble_memory *s, const char *format, ...) {
	struct mmu_disassemble_memory *m = (struct mmu_disassemble_memory *)s;

	char buffer[50];

	va_list list;
	va_start(list, format);

	vsnprintf(buffer, 50, format, list);

	return debugger_print(m->debugger, "%s", buffer);
}

uint8_t disassemble_read(struct disassemble_memory *s, uint16_t pointer) {
	struct mmu_disassemble_memory *m = (struct mmu_disassemble_memory *)s;
	return m->cpu->memory_read(m->cpu->memory, pointer);
}

static int __command_disassemble(debugger_t debugger, void *data, int argc, char **argv) {
	if (argc > 3) {
		debugger_print(debugger, "%s `start` `count` - Print the disassembled commands\n"
				" Prints `count` disassembled commands starting in memory from `start`.\n", argv[0]);
		return 0;
	}

	z80_cpu_t cpu = &debugger->asic->cpu;

	uint16_t start = debugger->asic->cpu.registers.PC;
	uint16_t count = 10;

	if (argc > 1)
		start = debugger_evaluate(debugger, argv[1]);
	if (argc > 2)
		count = debugger_evaluate(debugger, argv[2]);

	uint16_t i = 0;

	struct mmu_disassemble_memory str = { { disassemble_read, start }, cpu, debugger };

	for (i = 0; i < count; i++) {
		debugger_print(debugger, "0x%04X: ", str.mem.current);
		parse_instruction(&(str.mem), disassemble_print, debugger->flags.knightos);
		debugger_print(debugger, "\n");
	}

	return 0;
}

const struct debugger_command DisassembleCommand = {
	.name = "disassemble",
	.usage = "<start> <count>",
	.summary = "Print the disassembled commands",
	.description = "Prints `count` disassembled commands starting in memory from `start`.",
	.callback = __command_disassemble,
};
