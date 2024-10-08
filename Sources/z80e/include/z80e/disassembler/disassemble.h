#pragma once

#include <stdint.h>

struct disassemble_memory {
	uint8_t (*read_byte)(struct disassemble_memory *, uint16_t);
	uint16_t current;
	void *extra_data;
};


typedef int (*write_pointer_t)(struct disassemble_memory *, const char *, ...);

uint16_t parse_instruction(struct disassemble_memory *, write_pointer_t, bool);

void disassembler_init();

void disassembler_load_object(const char *path);

int get_symbol(struct disassemble_memory *memory, const char *name, uint16_t *val);
