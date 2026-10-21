#pragma once

#include <stdbool.h>
#include <stdint.h>

// Probe AXP2101 at the address in HARDWARE_REFERENCE.md.
// Chip-id register 0x03 / value 0x4A is what XPowersLib checks in the
// Waveshare AXP2101 Arduino example's library. Charge settings are left
// at the chip default.
bool pmu_probe(void);
