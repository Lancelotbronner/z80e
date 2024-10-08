//
//  ti/runloop.h
//  z80e
//
//  Created by Christophe Bronner on 2024-09-12.
//

#pragma once

#include <z80e/types.h>

typedef struct {
	int index;
	int after_cycle;
} timer_tick_t;

struct z80_runloop {
	long long last_end;
	int spare_cycles;
	timer_tick_t *ticks;
	int ticks_capacity;
};

void runloop_init(asic_t asic);
