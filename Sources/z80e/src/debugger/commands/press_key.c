#include <strings.h>
#include <z80e/debugger/commands.h>
#include <z80e/debugger/debugger.h>
#include <z80e/devices/keyboard.h>
#include <z80e/debugger/keys.h>

int command_press_key(debugger_state_t *state, int argc, char **argv) {
	if (argc != 2) {
		state->print(state, "%s - Depress the specified key code, in hex or by name.\n", argv[0]);
		return 0;
	}
	uint8_t key;

	int i, found = 0;
	for (i = 0; i < sizeof(key_strings) / sizeof(key_string_t); ++i) {
		if (strcasecmp(key_strings[i].key, argv[1]) == 0) {
			key = key_strings[i].value;
			found = 1;
			break;
		}
	}
	if (!found) {
		key = parse_expression_z80e(state, argv[1]);
	}

	keyboard_press(state->asic->cpu.devices[0x01].data, key);
	return 0;
}
