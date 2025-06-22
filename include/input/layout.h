#pragma once

#include <stdint.h>

extern uint16_t us_qwerty_kbd[][98];
extern uint16_t el_qwerty_kbd[][98];
extern uint16_t ru_ytsuken_kbd[][98];
extern uint16_t fr_bepo_kbd[][98];

extern uint16_t (* layer0_kbd_layout)[][98];
extern uint16_t (* layer1_kbd_layout)[][98];
extern uint16_t (* layer2_kbd_layout)[][98];

uint16_t (*layout_get_layer(int layer))[][98];
void layout_init();