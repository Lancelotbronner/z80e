#include <z80e/devices/status.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <z80e/ti/asic.h>
#include <z80e/cpu/z80.h>
#include <z80e/ti/hardware/interrupts.h>

static uint8_t __status_read(device_t device) {
	asic_t *asic = device->data;
	uint8_t value = 0x00;

	if (asic->battery_remove_check) {
		if (asic->battery != BATTERIES_REMOVED)
			value |= 0x01;
	} else if (asic->battery == BATTERIES_GOOD)
		value |= 0x01;

	value |= asic->device != TI73 ? 0x02 : 0x00;

	if (asic->mmu.flash_unlocked)
		value |= 0x04;

	if (asic->device != TI73 && asic->device != TI83p)
		value |= 0x80;

	if (asic->device != TI73 && asic->device != TI83p)
		value |= 0x40; // link assist available

	if (asic->device == TI84p || asic->device == TI84pSE || asic->device == TI84pCSE)
		value |= 0x20;

	return value;
}

static void __status_write(device_t device, uint8_t value) {
	asic_t *asic = device->data;
	if (asic->device == TI83p || asic->device == TI73)
		return;
	write_acknowledged_interrupts(asic->interrupts, value);
}

void device_status(device_t device, asic_t *asic) {
	device->data = asic;
	device->read = __status_read;
	device->write = __status_write;
}
