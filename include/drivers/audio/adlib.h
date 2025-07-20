#pragma once

#include <stdint.h>

typedef struct {
	uint16_t packed_channel;
	uint16_t freq;
	uint8_t feedback;
	uint8_t modulator_ctrl;
	uint8_t waveform;

	uint8_t attack;
	uint8_t release;
} adlib_priv_t;

typedef struct {
	uint8_t music_ctrl;
} adlib_synth_priv_t;

enum {
	REG_WAVEFORM_CTRL = 0x01,
	REG_TIMER0 = 0x02,
	REG_TIMER1 = 0x03,
	REG_TIMER_CTRL = 0x04,
	REG_SPEECH_MODE = 0x08,
	REG_MODULATOR_CTRL = 0x20,
	REG_KEY_SCALER = 0x40,
	REG_DECAY_RATE = 0x60,
	REG_RELEASE_RATE = 0x80,
	REG_NOTE_LOW = 0xa0,
	REG_NOTE_HIGH = 0xb0,
	REG_FEEDBACK = 0xc0,
	REG_MUSIC_CTRL = 0xbd,
	REG_WAVEFORM = 0xe0,
};

enum {
	TIMER_CTRL_ENABLE_TIMER0	= 0b00000001,
	TIMER_CTRL_ENABLE_TIMER1	= 0b00000010,
	TIMER_CTRL_DISABLE_TIMER0	= 0b00100000,
	TIMER_CTRL_DISABLE_TIMER1	= 0b01000000,
	TIMER_CTRL_IRQ_RESET		= 0b10000000,
};

#define ADLIB_LEFT_SPEAKER 0x220
#define ADLIB_RIGHT_SPEAKER 0x222
#define ADLIB_MONO 0x388

void adlib_init();