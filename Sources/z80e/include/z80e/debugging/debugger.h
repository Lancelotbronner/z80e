#pragma once

#include <z80e/types.h>
#include <z80e/debugging/hooks.h>

#include <stdarg.h>

struct debugger_commands {
	int count;
	int capacity;
	debugger_command_t *storage;
};

struct debugger_state {
	int (*print)(debugger_state_t , const char *, ...);
	int (*vprint)(debugger_state_t , const char *, va_list);
	void *state;
	void *interface_state;
	asic_t asic;
	debugger_t debugger;
	debugger_state_t (*create_new_state)(debugger_state_t , const char *command_name);
	void (*close_window)(debugger_state_t );
};

typedef enum {
	DEBUGGER_DISABLED,
	DEBUGGER_ENABLED,
	DEBUGGER_LONG_OPERATION,
	DEBUGGER_LONG_OPERATION_INTERRUPTABLE
} debugger_operation_state;

struct debugger {
	debugger_operation_state state;
	asic_t asic;

	struct debugger_commands commands;

	struct {
		struct hooks_execution before_execution;
		struct hooks_execution after_execution;
	} hook;

	struct {
		bool echo : 1;
		bool echo_reg : 1;
		bool auto_on : 1;
		bool knightos : 1;
		bool nointonstep : 1;
	} flags;
};

int debugger_source_rc(debugger_state_t , const char *rc_name);

void debugger_init(debugger_t debugger, asic_t asic);
void debugger_deinit(debugger_t debugger);

int debugger_find(debugger_t debugger, const char *, debugger_command_t *);
bool debugger_register(debugger_t debugger, const debugger_command_t command, void *data);

char **debugger_parse(const char *, int *);
int debugger_exec(debugger_state_t , const char *);

uint16_t debugger_evaluate(debugger_state_t , const char *);