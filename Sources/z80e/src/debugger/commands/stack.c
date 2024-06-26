#include <z80e/debugger/commands.h>
#include <z80e/debugger/debugger.h>
#include <z80e/ti/memory.h>

int command_stack(struct debugger_state *state, int argc, char **argv) {
	if (argc > 1) {
		state->print(state, "%s [count] - print first `count` (or 10) items on the stack\n", argv[0]);
		return 0;
	}

	z80cpu_t *cpu = &state->asic->cpu;
	uint16_t sp = cpu->registers.SP;

	uint16_t i;
	for (i = sp; i != (uint16_t)(sp + 20); i += 2) {
		state->print(state, "0x%04X: 0x%04X\n", i, cpu_read_word(cpu, i));
	}

	return 0;
}
