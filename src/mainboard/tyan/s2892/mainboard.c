#include <device/device.h>
#include "chip.h"

#if CONFIG_CHIP_NAME == 1
struct chip_operations mainboard_tyan_s2892_ops = {
	CHIP_NAME("Tyan S2892 mainboard")
};
#endif

