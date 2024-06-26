#pragma once

#include <z80e/debugger/debugger.h>
#include <z80e/ti/memory.h>

int command_hexdump(debugger_state_t *state, int argc, char **argv);
int command_backwards_hexdump(debugger_state_t *state, int argc, char **argv);
int command_disassemble(debugger_state_t *state, int argc, char **argv);
int command_print_registers(debugger_state_t *state, int argc, char **argv);
int command_print_expression(debugger_state_t *state, int argc, char **argv);
int command_stack(debugger_state_t *state, int argc, char **argv);
int command_print_mappings(debugger_state_t *state, int argc, char **argv);
int command_unhalt(debugger_state_t *state, int argc, char **argv);
int command_run(debugger_state_t *state, int argc, char **argv);
int command_step(debugger_state_t *state, int argc, char **argv);
int command_stop(debugger_state_t *state, int argc, char **argv);
int command_on(debugger_state_t *state, int argc, char **argv);
int command_in(debugger_state_t *state, int argc, char **argv);
int command_out(debugger_state_t *state, int argc, char **argv);
int command_break(debugger_state_t *state, int argc, char **argv);
int command_step_over(debugger_state_t *state, int argc, char **argv);
int command_link(debugger_state_t *state, int argc, char **argv);
int command_dump_lcd(debugger_state_t *state, int argc, char **argv);
int command_turn_on(debugger_state_t *state, int argc, char **argv);
int command_press_key(debugger_state_t *state, int argc, char **argv);
int command_release_key(debugger_state_t *state, int argc, char **argv);
int command_load_register(debugger_state_t *state, int argc, char **argv);
int command_tap_key(debugger_state_t *state, int argc, char **argv);
int command_timer(debugger_state_t *state, int argc, char **argv);
uint16_t parse_expression_z80e(debugger_state_t *, const char *);

void init_link(struct debugger_state *state);
