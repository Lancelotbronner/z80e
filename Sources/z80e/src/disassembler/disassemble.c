#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <ScasKit/list.h>

#include <z80e/disassembler/disassemble.h>

struct pcall {
	char *name;
	uint16_t addr;
};

struct {
	list_t pcalls;
	list_t objects;
} globals;

void parse_kernel_inc(FILE *file) {
	int _;
	while (!feof(file)) {
		char *line = read_line(file);
		line = strip_whitespace(line, &_);
		if (strncmp(".equ ", line, 5) == 0) {
			struct pcall *item = calloc(1, sizeof(struct pcall));
			size_t len = strlen(line) - strlen(".equ ") - strlen(" 0x0000");
			item->name = malloc(len + 1);
			strncpy(item->name, line + strlen(".equ "), len);
			item->name[len] = '\0';
			item->addr = (uint16_t)strtol(line + strlen(".equ  0x") + strlen(item->name), NULL, 16);
			list_add(globals.pcalls, item);
		}
		free(line);
	}
	fclose(file);
}

void disassembler_init() {
	FILE *kernel_inc = fopen(".knightos/include/kernel.inc", "r");
	if (kernel_inc) {
		globals.pcalls = create_list();
		parse_kernel_inc(kernel_inc);
	}
	globals.objects = create_list();
}

void disassembler_load_object(const char *path) {
	FILE *file = fopen(path, "r");
	if (file) {
		object_t *object = freadobj(file, path);
		list_add(globals.objects, object);
		printf("Loaded object from %s\n", path);
	} else {
		printf("Unable to open file for reading: %s\n", path);
	}
}

const char *pcall_name(uint16_t addr) {
	if (addr < 0x4000 && addr > 0x4000 - (256 * 3)) { // Optimized page 0 pcall
		addr = (0x4000 - addr) / 3 - 1;
		addr <<= 8;
	}

	if (!globals.pcalls) {
		return NULL;
	} else {
		int i;
		for (i = 0; i < globals.pcalls->length; ++i) {
			struct pcall *item = globals.pcalls->items[i];
			if (item->addr == addr) {
				return item->name;
			}
		}
		return NULL;
	}
}

#define KNIGHTOS_CURRENT_THREAD 0x8200
#define KNIGHTOS_ACTIVE_THREADS 0x8201
#define KNIGHTOS_THREAD_TABLE 	0x8000
#define KNIGHTOS_THREAD_LEN 	8

int get_symbol(struct disassemble_memory *memory, const char *name, uint16_t *val) {
	int i;
	uint16_t current = memory->current;
	uint16_t offset = 0;
	if (current > 0x8000) {
		// Userspace - parse thread table
		uint8_t threads = memory->read_byte(memory, KNIGHTOS_ACTIVE_THREADS);
		for (i = 0; i < threads; ++i) {
			uint16_t offs = KNIGHTOS_THREAD_TABLE + i * KNIGHTOS_THREAD_LEN;

			uint8_t first = memory->read_byte(memory, offs + 1);
			uint8_t second = memory->read_byte(memory, offs + 2);
			uint16_t addr = first | second << 8;

			first = memory->read_byte(memory, addr - 2);
			second = memory->read_byte(memory, addr - 1);
			uint16_t len = first | second << 8;

			if (current >= addr && current < addr + len) {
				offset = addr;
				break;
			}
		}
	}

	for (i = 0; i < globals.objects->length; ++i) {
		object_t *obj = globals.objects->items[i];
		int j;
		for (j = 0; j < obj->areas->length; ++j) {
			area_t *area = obj->areas->items[j];
			int k;
			for (k = 0; k < area->symbols->length; ++k) {
				symbol_t *sym = area->symbols->items[k];
				if (strncasecmp(sym->name, name, strlen(sym->name)) == 0) {
					*val = sym->value + offset;
					return (int)strlen(sym->name);
				}
			}
		}
	}
	return 0;
}

bool try_from_sourcemap(struct disassemble_memory *memory, source_map_t **map, source_map_entry_t **entry) {
	int i;
	uint16_t current = memory->current;
	uint16_t offset = 0;
	if (current > 0x8000) {
		// Userspace - parse thread table
		uint8_t threads = memory->read_byte(memory, KNIGHTOS_ACTIVE_THREADS);
		for (i = 0; i < threads; ++i) {
			uint16_t offs = KNIGHTOS_THREAD_TABLE + i * KNIGHTOS_THREAD_LEN;

			uint8_t first = memory->read_byte(memory, offs + 1);
			uint8_t second = memory->read_byte(memory, offs + 2);
			uint16_t addr = first | second << 8;

			first = memory->read_byte(memory, addr - 2);
			second = memory->read_byte(memory, addr - 1);
			uint16_t len = first | second << 8;

			if (current >= addr && current < addr + len) {
				offset = addr;
				break;
			}
		}
	}

	for (i = 0; i < globals.objects->length; ++i) {
		object_t *obj = globals.objects->items[i];
		int j;
		for (j = 0; j < obj->areas->length; ++j) {
			area_t *area = obj->areas->items[j];
			int k;
			for (k = 0; k < area->source_map->length; ++k) {
				*map = area->source_map->items[k];
				int l;
				for (l = 0; l < (*map)->entries->length; ++l) {
					*entry = (*map)->entries->items[l];
					if ((*entry)->address + offset == current) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

struct context {
	uint16_t prefix;
	struct disassemble_memory *memory;
	union {
		uint8_t opcode;
		struct {
			uint8_t z : 3;
			uint8_t y : 3;
			uint8_t x : 2;
		};
		struct {
			uint8_t   : 3;
			uint8_t q : 1;
			uint8_t p : 2;
		};
	};
	write_pointer_t write;
};

void parse_n(struct context *context) {
	context->write(context->memory, "0x%02X", context->memory->read_byte(context->memory, context->memory->current++));
}

void parse_d(struct context *context) {
	context->write(context->memory, "%d", (int8_t)context->memory->read_byte(context->memory, context->memory->current++));
}

void parse_d_relative(struct context *context) {
	uint16_t address = context->memory->current + 1;
	context->write(context->memory, "0x%02X", (int8_t)context->memory->read_byte(context->memory, context->memory->current++) + address);
}

uint16_t parse_nn(struct context *context) {
	uint8_t first = context->memory->read_byte(context->memory, context->memory->current++);
	uint8_t second = context->memory->read_byte(context->memory, context->memory->current++);
	uint16_t total = first | second << 8;
	return total;
}

void parse_HorIHw(struct context *context) {
	switch (context->prefix >> 8) {
	case 0xDD: context->write(context->memory, "IXH"); break;
	case 0xFD: context->write(context->memory, "IYH"); break;
	default: context->write(context->memory, "H"); break;
	}
}

void parse_LorILw(struct context *context) {
	switch (context->prefix >> 8) {
	case 0xDD: context->write(context->memory, "IXL"); break;
	case 0xFD: context->write(context->memory, "IYL"); break;
	default: context->write(context->memory, "L"); break;
	}
}

void parse_HLorIr(struct context *context) {
	switch (context->prefix >> 8) {
	case 0xDD: context->write(context->memory, "IX"); break;
	case 0xFD: context->write(context->memory, "IY"); break;
	default: context->write(context->memory, "HL"); break;
	}
}

void parse_r(struct context *context, uint8_t part) {
	switch (part) {
	case 0: context->write(context->memory, "B"); break;
	case 1: context->write(context->memory, "C"); break;
	case 2: context->write(context->memory, "D"); break;
	case 3: context->write(context->memory, "E"); break;
	case 4: parse_HorIHw(context);
		break;
	case 5: parse_LorILw(context);
		break;
	case 6:
		if (context->prefix >> 8 == 0xDD) {
			uint8_t d = context->memory->read_byte(context->memory, context->memory->current++);
			context->write(context->memory, "(IX + 0x%X)", d);
		} else if (context->prefix >> 8 == 0xFD) {
			uint8_t d = context->memory->read_byte(context->memory, context->memory->current++);
			context->write(context->memory, "(IY + 0x%X)", d);
		} else {
			context->write(context->memory, "(HL)");
		}
		break;
	case 7: context->write(context->memory, "A"); break;
	}
}

void parse_rw_r(struct context *context, int write, int read) {
	uint16_t old_prefix = context->prefix;
	if (write != 6) {
		context->prefix &= 0xFF;
	}
	parse_r(context, write);
	if (write == 6) {
		context->prefix &= 0xFF;
	} else {
		context->prefix = old_prefix;
	}
	context->write(context->memory, ", ");
	parse_r(context, read);
}

const char *parse_cc(int i, struct context *context) {
	switch (i) {
	case 0: return "NZ";
	case 1: return "Z";
	case 2: return "NC";
	case 3: return "C";
	case 4: return "PO";
	case 5: return "PE";
	case 6: return "P";
	case 7: return "M";
	}
	return NULL;
}

void parse_rp(int i, struct context *context) {
	switch (i) {
	case 0: context->write(context->memory, "BC"); break;
	case 1: context->write(context->memory, "DE"); break;
	case 2: parse_HLorIr(context); break;
	case 3: context->write(context->memory, "SP"); break;
	}
}

void parse_rp2(int i, struct context *context) {
	switch (i) {
	case 0: context->write(context->memory, "BC"); break;
	case 1: context->write(context->memory, "DE"); break;
	case 2: parse_HLorIr(context); break;
	case 3: context->write(context->memory, "AF"); break;
	}
}

void parse_rot(int y, struct context *context) {
	switch (y) {
	case 0: // RLC r[z]
		context->write(context->memory, "RLC ");
		break;
	case 1: // RRC r[z]
		context->write(context->memory, "RRC ");
		break;
	case 2: // RL r[z]
		context->write(context->memory, "RL ");
		break;
	case 3: // RR r[z]
		context->write(context->memory, "RR ");
		break;
	case 4: // SLA r[z]
		context->write(context->memory, "SLA ");
		break;
	case 5: // SRA r[z]
		context->write(context->memory, "SRA ");
		break;
	case 6: // SLL r[z]
		context->write(context->memory, "SLL ");
		break;
	case 7: // SRL r[z]
		context->write(context->memory, "SRL ");
		break;
	}
}

void parse_im(int y, struct context *context) {
	switch (y) {
	case 0: context->write(context->memory, "IM 0"); break;
	case 1: context->write(context->memory, "IM 0"); break;
	case 2: context->write(context->memory, "IM 1"); break;
	case 3: context->write(context->memory, "IM 2"); break;
	case 4: context->write(context->memory, "IM 0"); break;
	case 5: context->write(context->memory, "IM 0"); break;
	case 6: context->write(context->memory, "IM 1"); break;
	case 7: context->write(context->memory, "IM 2"); break;
	}
}

void parse_alu(int i, struct context *context) {
	switch (i) {
	case 0: // ADD A, v
		context->write(context->memory, "ADD A, ");
		break;
	case 1: // ADC A, v
		context->write(context->memory, "ADC A, ");
		break;
	case 2: // SUB v
		context->write(context->memory, "SUB ");
		break;
	case 3: // SBC v
		context->write(context->memory, "SBC ");
		break;
	case 4: // AND v
		context->write(context->memory, "AND ");
		break;
	case 5: // XOR v
		context->write(context->memory, "XOR ");
		break;
	case 6: // OR v
		context->write(context->memory, "OR ");
		break;
	case 7: // CP v
		context->write(context->memory, "CP ");
		break;
	}
}

void parse_bli(int y, int z, struct context *context) {
	switch (y) {
	case 4:
		switch (z) {
		case 0: // LDI
			context->write(context->memory, "LDI");
			break;
		case 1: // CPI
			context->write(context->memory, "CPI");
			break;
		case 2: // INI
			context->write(context->memory, "INI");
			break;
		case 3: // OUTI
			context->write(context->memory, "OUTI");
			break;
		}
		break;
	case 5:
		switch (z) {
		case 0: // LDD
			context->write(context->memory, "LDD");
			break;
		case 1: // CPD
			context->write(context->memory, "CPD");
			break;
		case 2: // IND
			context->write(context->memory, "IND");
			break;
		case 3: // OUTD
			context->write(context->memory, "OUTD");
			break;
		}
		break;
	case 6:
		switch (z) {
		case 0: // LDIR
			context->write(context->memory, "LDIR");
			break;
		case 1: // CPIR
			context->write(context->memory, "CPIR");
			break;
		case 2: // INIR
			context->write(context->memory, "INIR");
			break;
		case 3: // OTIR
			context->write(context->memory, "OTIR");
			break;
		}
		break;
	case 7:
		switch (z) {
		case 0: // LDDR
			context->write(context->memory, "LDDR");
			break;
		case 1: // CPDR
			context->write(context->memory, "CPDR");
			break;
		case 2: // INDR
			context->write(context->memory, "INDR");
			break;
		case 3: // OTDR
			context->write(context->memory, "OTDR");
			break;
		}
		break;
	}
}

uint16_t parse_instruction(struct disassemble_memory *memory, write_pointer_t write_p, bool knightos) {
	source_map_t *map;
	source_map_entry_t *entry;
	if (try_from_sourcemap(memory, &map, &entry)) {
		char *code = entry->source_code;
		while (*code && isspace(*code)) code++;
		write_p(memory, "%s:%d: %s", map->file_name, entry->line_number, code);
		memory->current += entry->length;
		return entry->length;
	}

	uint16_t start_mem = memory->current;
	struct context context;
	context.prefix = 0;
	uint8_t data = memory->read_byte(memory, memory->current);
	if (data == 0xDD || data == 0xFD) {
		context.prefix &= 0x00FF;
		context.prefix |= memory->read_byte(memory, memory->current++) << 8;
		data = memory->read_byte(memory, memory->current);
	}
	if (data == 0xCB || data == 0xED) {
		context.prefix &= 0xFF00;
		context.prefix |= memory->read_byte(memory, memory->current++);
		data = memory->read_byte(memory, memory->current);
	}

	if (context.prefix >> 8 && context.prefix & 0xFF) {
		data = memory->read_byte(memory, memory->current-- + 1);
	}

	context.opcode = data;
	memory->current++;
	context.memory = memory;
	write_pointer_t write = context.write = write_p;

	if (context.prefix & 0xFF) {
		switch (context.prefix & 0xFF) {
		case 0xCB:
			switch (context.x) {
			case 0: // rot[y] r[z]
				parse_rot(context.y, &context);
				parse_r(&context, context.z);
				break;
			case 1: // BIT y, r[z]
				write(memory, "BIT 0x%X, ", context.y);
				parse_r(&context, context.z);
				break;
			case 2: // RES y, r[z]
				write(memory, "RES 0x%X, ", context.y);
				parse_r(&context, context.z);
				break;
			case 3: // SET y, r[z]
				write(memory, "SET 0x%X, ", context.y);
				parse_r(&context, context.z);
				break;
			}
			break;
		case 0xED:
			switch (context.x) {
			case 1:
				switch (context.z) {
				case 0:
					if (context.y == 6) { // IN (C)
						write(memory, "IN (C)");
					} else { // IN r[y], (C)
						write(memory, "IN ");
						parse_r(&context, context.y);
						write(memory, ", (C)");
					}
					break;
				case 1:
					if (context.y == 6) { // OUT (C), 0
						write(memory, "OUT (C), 0");
					} else { // OUT (C), r[y]
						write(memory, "OUT (C), ");
						parse_r(&context, context.y);
					}
					break;
				case 2:
					if (context.q == 0) { // SBC HL, rp[p]
						write(memory, "SBC HL, ");
						parse_rp(context.p, &context);
					} else { // ADC HL, rp[p]
						write(memory, "ADC HL, ");
						parse_rp(context.p, &context);
					}
					break;
				case 3:
					if (context.q == 0) { // LD (nn), rp[p]
						write(memory, "LD (0x%04X), ", parse_nn(&context));
						parse_rp(context.p, &context);
					} else { // LD rp[p], (nn)
						write(memory, "LD ");
						parse_rp(context.p, &context);
						write(memory, ", (0x%04X)", parse_nn(&context));
					}
					break;
				case 4: // NEG
					write(memory, "NEG");
					break;
				case 5:
					if (context.y == 1) { // RETI
						write(memory, "RETI");
						break;
					} else { // RETN
						write(memory, "RETN");
						break;
					}
					break;
				case 6: // IM im[y]
					parse_im(context.y, &context);
					break;
				case 7:
					switch (context.y) {
					case 0: // LD I, A
						write(memory, "LD I, A");
						break;
					case 1: // LD R, A
						write(memory, "LD R, A");
						break;
					case 2: // LD A, I
						write(memory, "LD A, I");
						break;
					case 3: // LD A, R
						write(memory, "LD A, R");
						break;
					case 4: // RRD
						write(memory, "RRD");
						break;
					case 5: // RLD
						write(memory, "RLD");
						break;
					default: // NOP (invalid instruction)
						write(memory, "*INVALID* 0x%X", context.opcode);
						break;
					}
					break;
				}
				break;
			case 2:
				if (context.y >= 4) { // bli[y,z]
					parse_bli(context.y, context.z, &context);
				} else { // NONI (invalid instruction)
					write(memory, "*INVALID* 0x%X", context.opcode);
				}
				break;
			default: // NONI (invalid instruction)
				write(memory, "*INVALID* 0x%X", context.opcode);
				break;
			}
			break;
		}
	} else {
		switch (context.x) {
		case 0:
			switch (context.z) {
			case 0:
				switch (context.y) {
				case 0: // NOP
					write(memory, "NOP");
					break;
				case 1: // EX AF, AF'
					write(memory, "EX AF, AF'");
					break;
				case 2: // DJNZ d
					write(memory, "DJNZ ");
					parse_d_relative(&context);
					break;
				case 3: // JR d
					write(memory, "JR ");
					parse_d_relative(&context);
					break;
				case 4:
				case 5:
				case 6:
				case 7: // JR cc[y-4], d
					write(memory, "JR %s, ", parse_cc(context.y - 4, &context));
					parse_d_relative(&context);
					break;
				}
				break;
			case 1:
				switch (context.q) {
				case 0: // LD rp[p], nn
					write(memory, "LD ");
					parse_rp(context.p, &context);
					write(memory, ", 0x%04X", parse_nn(&context));
					break;
				case 1: // ADD HL, rp[p]
					write(memory, "ADD ");
					parse_HLorIr(&context);
					write(memory, ", ");
					parse_rp(context.p, &context);
					break;
				}
				break;
			case 2:
				switch (context.q) {
				case 0:
					switch (context.p) {
					case 0: // LD (BC), A
						write(memory, "LD (BC), A");
						break;
					case 1: // LD (DE), A
						write(memory, "LD (DE), A");
						break;
					case 2: // LD (nn), HL
						write(memory, "LD (0x%04X), ", parse_nn(&context));
						parse_HLorIr(&context);
						break;
					case 3: // LD (nn), A
						write(memory, "LD (0x%04X), A", parse_nn(&context));
						break;
					}
					break;
				case 1:
					switch (context.p) {
					case 0: // LD A, (BC)
						write(memory, "LD A, (BC)");
						break;
					case 1: // LD A, (DE)
						write(memory, "LD A, (DE)");
						break;
					case 2: // LD HL, (nn)
						write(memory, "LD ");
						parse_HLorIr(&context);
						write(memory, ", (0x%04X)", parse_nn(&context));
						break;
					case 3: // LD A, (nn)
						write(memory, "LD A, (0x%04X)", parse_nn(&context));
						break;
					}
					break;
				}
				break;
			case 3:
				switch (context.q) {
				case 0: // INC rp[p]
					write(memory, "INC ");
					parse_rp(context.p, &context);
					break;
				case 1: // DEC rp[p]
					write(memory, "DEC ");
					parse_rp(context.p, &context);
					break;
				}
				break;
			case 4: // INC r[y]
				write(memory, "INC ");
				parse_r(&context, context.y);
				break;
			case 5: // DEC r[y]
				write(memory, "DEC ");
				parse_r(&context, context.y);
				break;
			case 6: // LD r[y], n
				write(memory, "LD ");
				parse_r(&context, context.y);
				write(memory, ", ");
				parse_n(&context);
				break;
			case 7:
				switch (context.y) {
				case 0: // RLCA
					write(memory, "RLCA");
					break;
				case 1: // RRCA
					write(memory, "RRCA");
					break;
				case 2: // RLA
					write(memory, "RLA");
					break;
				case 3: // RRA
					write(memory, "RRA");
					break;
				case 4: // DAA
					write(memory, "DAA");
					break;
				case 5: // CPL
					write(memory, "CPL");
					break;
				case 6: // SCF
					write(memory, "SCF");
					break;
				case 7: // CCF
					write(memory, "CCF");
					break;
				}
				break;
			}
			break;
		case 1:
			if (context.z == 6 && context.y == 6) { // HALT
				write(memory, "HALT");
			} else { // LD r[y], r[z]
				write(memory, "LD ");
				parse_rw_r(&context, context.y, context.z);
			}
			break;
		case 2: // ALU[y] r[z]
			parse_alu(context.y, &context);
			parse_r(&context, context.z);
			break;
		case 3:
			switch (context.z) {
			case 0: // RET cc[y]
				write(memory, "RET %s", parse_cc(context.y, &context));
				break;
			case 1:
				switch (context.q) {
				case 0: // POP rp2[p]
					write(memory, "POP ");
					parse_rp2(context.p,&context);
					break;
				case 1:
					switch (context.p) {
					case 0: // RET
						write(memory, "RET");
						break;
					case 1: // EXX
						write(memory, "EXX");
						break;
					case 2: // JP HL
						write(memory, "JP ");
						parse_HLorIr(&context);
						break;
					case 3: // LD SP, HL
						write(memory, "LD SP, ");
						parse_HLorIr(&context);
						break;
					}
					break;
				}
				break;
			case 2: // JP cc[y], nn
				write(memory, "JP %s", parse_cc(context.y, &context));
				write(memory, ", 0x%04X", parse_nn(&context));
				break;
			case 3:
				switch (context.y) {
				case 0: // JP nn
					write(memory, "JP 0x%04X", parse_nn(&context));
					break;
				case 1: // 0xCB prefixed opcodes
					// Handled before!
					break;
				case 2: // OUT (n), A
					write(memory, "OUT (");
					parse_n(&context);
					write(memory, "), A");
					break;
				case 3: // IN A, (n)
					write(memory, "IN A, (");
					parse_n(&context);
					write(memory, ")");
					break;
				case 4: // EX (SP), HL
					write(memory, "EX (SP), ");
					parse_HLorIr(&context);
					break;
				case 5: // EX DE, HL
					write(memory, "EX DE, HL");
					break;
				case 6: // DI
					write(memory, "DI");
					break;
				case 7: // EI
					write(memory, "EI");
					break;
				}
				break;
			case 4: // CALL cc[y], nn
			{
				const char *cc = parse_cc(context.y, &context);
				uint16_t nn = parse_nn(&context);
				// Check if this is an optimized page 0 pcall
				if (nn < 0x4000 && nn > 0x4000 - (256 * 3)) {
					const char *pcall = pcall_name(nn);
					if (pcall) {
						write(memory, "PCALL %s, %s", cc, pcall);
					} else {
						write(memory, "CALL %s, 0x%04X", cc, nn);
					}
				} else {
					write(memory, "CALL %s, 0x%04X", cc, nn);
				}
				break;
			}
			case 5:
				switch (context.q) {
				case 0: // PUSH r2p[p]
					write(memory, "PUSH ");
					parse_rp2(context.p, &context);
					break;
				case 1:
					switch (context.p) {
					case 0: // CALL nn
					{
						uint16_t nn = parse_nn(&context);
						if (nn < 0x4000 && nn > 0x4000 - (256 * 3)) {
							const char *pcall = pcall_name(nn);
							if (pcall) {
								write(memory, "PCALL %s", pcall);
							} else {
								write(memory, "CALL 0x%04X", nn);
							}
						} else {
							write(memory, "CALL 0x%04X", nn);
						}
						break;
					}
					case 1: // 0xDD prefixed opcodes
						// Handled before
						break;
					case 2: // 0xED prefixed opcodes
						// Handled before
						break;
					case 3: // 0xFD prefixed opcodes
						// Handled before
						break;
					}
					break;
				}
				break;
			case 6: // alu[y] n
				parse_alu(context.y, &context);
				parse_n(&context);
				break;
			case 7: // RST y*8
				if (knightos) {
					if (context.y * 8 == 0x20) {
						uint16_t nn = parse_nn(&context);
						const char *pcall = pcall_name(nn);
						if (pcall) {
							write(memory, "PCALL %s", pcall);
						} else {
							write(memory, "PCALL 0x%04X", nn);
						}
					} else if (context.y * 8 == 0x08) {
						write(memory, "K");
						parse_instruction(memory, write_p, knightos);
					} else {
						write(memory, "RST 0x%X", context.y * 8);
					}
				} else {
					write(memory, "RST 0x%X", context.y * 8);
				}
				break;
			}
			break;
		}
	}

	if (context.prefix >> 8 && context.prefix & 0xFF) {
		memory->current++;
	}
	return memory->current - start_mem;
}
