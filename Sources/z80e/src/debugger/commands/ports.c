#include <z80e/debugger/debugger.h>
#include <z80e/debugger/commands.h>

int command_in(struct debugger_state *state, int argc, char **argv) {
	if(argc != 2) {
		state->print(state, "%s `port` - read from port `port`.\n", argv[0]);
		return 0;
	}

	uint8_t port = parse_expression_z80e(state, argv[1]) & 0xFF;
	uint8_t val = device_read(&state->asic->cpu.devices[port]);
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
	device_write(&state->asic->cpu.devices[port], val);
	state->print(state, "port 0x%02X <- 0x%02X\n", port, val);
	return 0;
}
