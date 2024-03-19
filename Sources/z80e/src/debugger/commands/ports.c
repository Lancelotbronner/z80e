#include <z80e/debugger/debugger.h>
#include <z80e/debugger/commands.h>

int command_in(struct debugger_state *state, int argc, char **argv) {
	if(argc != 2) {
		state->print(state, "%s `port` - read from port `port`.\n", argv[0]);
		return 0;
	}

	uint8_t port = parse_expression_z80e(state, argv[1]) & 0xFF;
	uint8_t val = 0;
	if (state->asic->cpu.devices[port].read) {
		val = device_read(state->asic->cpu.devices[port]);
		val = state->asic->cpu.devices[port].read(state->asic->cpu.devices[port].data);
	}
	state->print(state, "port 0x%02X -> 0x%02X\n", port, val);
	return 0;
}

int command_out(struct debugger_state *state, int argc, char **argv) {
	if(argc != 3) {
		state->print(state, "%s `port` `value` - write `value` into port `port`.\n", argv[0]);
		return 0;
	}

	uint8_t port = parse_expression_z80e(state, argv[1]) & 0xFF;
	uint8_t val = parse_expression_z80e(state, argv[2]) & 0xFF;
	if (state->asic->cpu.devices[port].write != NULL) {
		state->asic->cpu.devices[port].write(state->asic->cpu.devices[port].data, val);
	}
	state->print(state, "port 0x%02X <- 0x%02X\n", port, val);
	return 0;
}
