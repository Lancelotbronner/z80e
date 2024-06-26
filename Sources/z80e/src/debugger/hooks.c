#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <z80e/debugger/hooks.h>
#include <z80e/cpu/z80.h>
#include <z80e/ti/asic.h>
#include <z80e/ti/memory.h>

enum hook_flags : unsigned char {
	HOOK_RESERVED_1 = 0x1,
};

typedef struct {
	void *data;
	hook_memory_callback_t callback;
	uint16_t range_start;
	uint16_t range_end;
	enum hook_flags flags;
} memory_hook_callback_t;

typedef struct {
	int capacity;
	memory_hook_callback_t *callbacks;
} hook_memory_array_t;

typedef struct {
	void *data;
	hook_register_callback_t callback;
	enum z80_registers registers;
	enum hook_flags flags;
} register_hook_callback_t;

typedef struct {
	int capacity;
	register_hook_callback_t *callbacks;
} hook_register_array_t;

typedef struct {
	void *data;
	hook_port_callback_t callback;
	uint8_t range_start;
	uint8_t range_end;
	int flags;
} port_hook_callback_t;

typedef struct {
	int capacity;
	port_hook_callback_t *callbacks;
} hook_port_array_t;

typedef struct {
	void *data;
	hook_execution_callback_t callback;
	int flags;
} execution_hook_callback_t;

typedef struct {
	int capacity;
	execution_hook_callback_t *callbacks;
} hook_execution_array_t;

typedef struct {
	void *data;
	hook_lcd_update_callback_t callback;
	int flags;
} lcd_update_callback_t;

typedef struct {
	int capacity;
	lcd_update_callback_t *callbacks;
} hook_lcd_update_array_t;

struct hook_info {
	z80cpu_t *cpu;
	ti_mmu_t mmu;

	hook_memory_array_t *on_memory_read;
	hook_memory_array_t *on_memory_write;

	hook_register_array_t *on_register_read;
	hook_register_array_t *on_register_write;

	hook_port_array_t *on_port_in;
	hook_port_array_t *on_port_out;

	hook_execution_array_t *on_before_execution;
	hook_execution_array_t *on_after_execution;

	hook_lcd_update_array_t *on_lcd_update;
};

hook_info_t *create_hook_set(asic_t asic) {
	hook_info_t *info = malloc(sizeof(hook_info_t));
	info->cpu = &asic->cpu;
	info->mmu = &asic->mmu;

	info->cpu->hook = info;
	info->mmu->hook = info;

	//TODO: Only allocate the first time we add a hook
	// then the on_* would be able to skip iteration
	info->on_memory_read = malloc(sizeof(hook_memory_array_t));
	info->on_memory_read->capacity = 10;
	info->on_memory_read->callbacks = calloc(10, sizeof(memory_hook_callback_t));

	info->on_memory_write = malloc(sizeof(hook_memory_array_t));
	info->on_memory_write->capacity = 10;
	info->on_memory_write->callbacks = calloc(10, sizeof(memory_hook_callback_t));

	info->on_register_read = malloc(sizeof(hook_register_array_t));
	info->on_register_read->capacity = 10;
	info->on_register_read->callbacks = calloc(10, sizeof(register_hook_callback_t));

	info->on_register_write = malloc(sizeof(hook_register_array_t));
	info->on_register_write->capacity = 10;
	info->on_register_write->callbacks = calloc(10, sizeof(register_hook_callback_t));

	info->on_port_in = malloc(sizeof(hook_port_array_t));
	info->on_port_in->capacity = 10;
	info->on_port_in->callbacks = calloc(10, sizeof(port_hook_callback_t));

	info->on_port_out = malloc(sizeof(hook_port_array_t));
	info->on_port_out->capacity = 10;
	info->on_port_out->callbacks = calloc(10, sizeof(port_hook_callback_t));

	info->on_before_execution = malloc(sizeof(hook_execution_array_t));
	info->on_before_execution->capacity = 10;
	info->on_before_execution->callbacks = calloc(10, sizeof(execution_hook_callback_t));

	info->on_after_execution = malloc(sizeof(hook_execution_array_t));
	info->on_after_execution->capacity = 10;
	info->on_after_execution->callbacks = calloc(10, sizeof(execution_hook_callback_t));

	info->on_lcd_update = malloc(sizeof(hook_lcd_update_array_t));
	info->on_lcd_update->capacity = 10;
	info->on_lcd_update->callbacks = calloc(10, sizeof(lcd_update_callback_t));

	return info;
}

uint8_t hook_on_memory_read(hook_info_t *info, uint16_t address, uint8_t value) {
	int i = 0;
	for (i = 0; i < info->on_memory_read->capacity; i++) {
		memory_hook_callback_t *cb = &info->on_memory_read->callbacks[i];
		if (cb->callback && address >= cb->range_start && address <= cb->range_end)
			value = cb->callback(cb->data, address, value);
	}
	return value;
}

uint8_t hook_on_memory_write(hook_info_t *info, uint16_t address, uint8_t value) {
	int i = 0;
	for (i = 0; i < info->on_memory_write->capacity; i++) {
		memory_hook_callback_t *cb = &info->on_memory_write->callbacks[i];
		if (cb->callback && address >= cb->range_start && address <= cb->range_end)
			value = cb->callback(cb->data, address, value);
	}
	return value;
}

int hook_add_to_memory_array(hook_memory_array_t *hook, uint16_t address_start, uint16_t address_end, void *data, hook_memory_callback_t callback) {
	int x = 0;

	for (; x < hook->capacity; x++) {
		if (!hook->callbacks[x].callback)
			break;

		if (x == hook->capacity - 1) {
			hook->capacity += 10;
			void *n = realloc(hook->callbacks, sizeof(memory_hook_callback_t) * hook->capacity);
			if (n == NULL) {
				return -1;
			}

			hook->callbacks = n;
			memset(n + sizeof(memory_hook_callback_t) * (hook->capacity - 10), 0, sizeof(memory_hook_callback_t) * 10);
		}
	}
	memory_hook_callback_t *cb = &hook->callbacks[x];

	cb->data = data;
	cb->range_start = address_start;
	cb->range_end = address_end;
	cb->callback = callback;
	cb->flags = 0;

	return x;
}

void hook_remove_memory_read(hook_info_t *info, int index) {
	info->on_memory_read->callbacks[index].callback = 0;
}

int hook_add_memory_read(hook_info_t *info, uint16_t address_start, uint16_t address_end, void *data, hook_memory_callback_t callback) {
	return hook_add_to_memory_array(info->on_memory_read, address_start, address_end, data, callback);
}

void hook_remove_memory_write(hook_info_t *info, int index) {
	info->on_memory_write->callbacks[index].callback = 0;
}

int hook_add_memory_write(hook_info_t *info, uint16_t address_start, uint16_t address_end, void *data, hook_memory_callback_t callback) {
	return hook_add_to_memory_array(info->on_memory_write, address_start, address_end, data, callback);
}

uint16_t hook_on_register_read(hook_info_t *info, enum z80_registers flags, uint16_t value) {
	int i = 0;
	for (i = 0; i < info->on_register_read->capacity; i++) {
		register_hook_callback_t *cb = &info->on_register_read->callbacks[i];
		if (cb->callback && cb->registers & flags)
			value = cb->callback(cb->data, flags, value);
	}
	return value;
}

uint16_t hook_on_register_write(hook_info_t *info, enum z80_registers flags, uint16_t value) {
	int i = 0;
	for (i = 0; i < info->on_register_write->capacity; i++) {
		register_hook_callback_t *cb = &info->on_register_write->callbacks[i];
		if (cb->callback && cb->registers & flags)
			value = cb->callback(cb->data, flags, value);
	}
	return value;
}

int hook_add_to_register_array(hook_register_array_t *hook, enum z80_registers flags, void *data, hook_register_callback_t callback) {
	int x = 0;

	for (; x < hook->capacity; x++) {
		if (!hook->callbacks[x].callback)
			break;

		if (x == hook->capacity - 1) {
			hook->capacity += 10;
			void *n = realloc(hook->callbacks, sizeof(register_hook_callback_t) * hook->capacity);
			if (n == NULL) {
				return -1;
			}

			hook->callbacks = n;
			memset(n + sizeof(register_hook_callback_t) * (hook->capacity - 10), 0, sizeof(memory_hook_callback_t) * 10);
		}
	}
	register_hook_callback_t *cb = &hook->callbacks[x];

	cb->data = data;
	cb->callback = callback;
	cb->registers = flags;
	cb->flags = 0;

	return x;
}

void hook_remove_register_read(hook_info_t *info, int index) {
	info->on_register_read->callbacks[index].callback = 0;
}

int hook_add_register_read(hook_info_t *info, enum z80_registers flags, void *data, hook_register_callback_t callback) {
	return hook_add_to_register_array(info->on_register_read, flags, data, callback);
}

void hook_remove_register_write(hook_info_t *info, int index) {
	info->on_register_write->callbacks[index].callback = 0;
}

int hook_add_register_write(hook_info_t *info, enum z80_registers flags, void *data, hook_register_callback_t callback) {
	return hook_add_to_register_array(info->on_register_write, flags, data, callback);
}

uint8_t hook_on_port_in(hook_info_t *info, uint8_t port, uint8_t value) {
	int i = 0;
	for (i = 0; i < info->on_port_in->capacity; i++) {
		port_hook_callback_t *cb = &info->on_port_in->callbacks[i];
		if (cb->callback && port >= cb->range_start && port <= cb->range_end)
			value = cb->callback(cb->data, port, value);
	}
	return value;
}

uint8_t hook_on_port_out(hook_info_t *info, uint8_t port, uint8_t value) {
	int i = 0;
	for (i = 0; i < info->on_port_out->capacity; i++) {
		port_hook_callback_t *cb = &info->on_port_out->callbacks[i];
		if (cb->callback && port >= cb->range_start && port <= cb->range_end)
			value = cb->callback(cb->data, port, value);
	}
	return value;
}

int hook_add_to_port_array(hook_port_array_t *hook, uint8_t range_start, uint8_t range_end, void *data, hook_port_callback_t callback) {
	int x = 0;

	for (; x < hook->capacity; x++) {
		if (!hook->callbacks[x].callback)
			break;

		if (x == hook->capacity - 1) {
			hook->capacity += 10;
			void *n = realloc(hook->callbacks, sizeof(port_hook_callback_t) * hook->capacity);
			if (n == NULL) {
				return -1;
			}

			hook->callbacks = n;
			memset(n + sizeof(port_hook_callback_t) * (hook->capacity - 10), 0, sizeof(port_hook_callback_t) * 10);
		}
	}
	port_hook_callback_t *cb = &hook->callbacks[x];

	cb->data = data;
	cb->range_start = range_start;
	cb->range_end = range_end;
	cb->callback = callback;
	cb->flags = 0;

	return x;
}

void hook_remove_port_in(hook_info_t *info, int index) {
	info->on_port_in->callbacks[index].callback = 0;
}

int hook_add_port_in(hook_info_t *info, uint8_t range_start, uint8_t range_end, void *data, hook_port_callback_t callback) {
	return hook_add_to_port_array(info->on_port_in, range_start, range_end, data, callback);
}

void hook_remove_port_out(hook_info_t *info, int index) {
	info->on_port_out->callbacks[index].callback = 0;
}

int hook_add_port_out(hook_info_t *info, uint8_t range_start, uint8_t range_end, void *data, hook_port_callback_t callback) {
	return hook_add_to_port_array(info->on_port_out, range_start, range_end, data, callback);
}

void hook_on_before_execution(hook_info_t *info, uint16_t address) {
	int i = 0;
	for (i = 0; i < info->on_before_execution->capacity; i++) {
		execution_hook_callback_t *cb = &info->on_before_execution->callbacks[i];
		if (cb->callback)
			cb->callback(cb->data, address);
	}
}

void hook_on_after_execution(hook_info_t *info, uint16_t address) {
	int i = 0;
	for (i = 0; i < info->on_after_execution->capacity; i++) {
		execution_hook_callback_t *cb = &info->on_after_execution->callbacks[i];
		if (cb->callback)
			cb->callback(cb->data, address);
	}
}

int hook_add_to_execution_array(hook_execution_array_t *hook, void *data, hook_execution_callback_t callback) {
	int x = 0;

	for (; x < hook->capacity; x++) {
		if (!hook->callbacks[x].callback)
			break;

		if (x == hook->capacity - 1) {
			hook->capacity += 10;
			void *n = realloc(hook->callbacks, sizeof(execution_hook_callback_t) * hook->capacity);
			if (n == NULL) {
				return -1;
			}

			hook->callbacks = n;
			memset(n, 0, sizeof(execution_hook_callback_t) * 10);
		}
	}
	execution_hook_callback_t *cb = &hook->callbacks[x];

	cb->data = data;
	cb->callback = callback;
	cb->flags = 0;

	return x;
}

void hook_remove_before_execution(hook_info_t *info, int index) {
	info->on_before_execution->callbacks[index].callback = 0;
}

int hook_add_before_execution(hook_info_t *info, void *data, hook_execution_callback_t callback) {
	return hook_add_to_execution_array(info->on_before_execution, data, callback);
}

void hook_remove_after_execution(hook_info_t *info, int index) {
	info->on_after_execution->callbacks[index].callback = 0;
}

int hook_add_after_execution(hook_info_t *info, void *data, hook_execution_callback_t callback) {
	return hook_add_to_execution_array(info->on_after_execution, data, callback);
}

void hook_on_lcd_update(hook_info_t *info, ti_bw_lcd_t *lcd) {
	int i = 0;
	for (i = 0; i < info->on_lcd_update->capacity; i++) {
		lcd_update_callback_t *cb = &info->on_lcd_update->callbacks[i];
		if (cb->callback)
			cb->callback(cb->data, lcd);
	}
}

void hook_remove_lcd_update(hook_info_t *info, int index) {
	info->on_lcd_update->callbacks[index].callback = 0;
}

int hook_add_lcd_update(hook_info_t *info, void *data, hook_lcd_update_callback_t callback) {
	hook_lcd_update_array_t *hook = info->on_lcd_update;
	int x = 0;

	for (; x < hook->capacity; x++) {
		if (!hook->callbacks[x].callback)
			break;

		if (x == hook->capacity - 1) {
			hook->capacity += 10;
			void *n = realloc(hook->callbacks, sizeof(lcd_update_callback_t) * hook->capacity);
			if (n == NULL) {
				return -1;
			}

			hook->callbacks = n;
			memset(n + sizeof(lcd_update_callback_t) * (hook->capacity - 10), 0, sizeof(lcd_update_callback_t) * 10);
		}
	}
	lcd_update_callback_t *cb = &hook->callbacks[x];

	cb->data = data;
	cb->callback = callback;
	cb->flags = 0;

	return x;
}
