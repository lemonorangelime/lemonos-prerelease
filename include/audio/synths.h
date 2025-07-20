#pragma once

#include <stdint.h>
#include <linked.h>

typedef struct fm_synthesiser_channel fm_synthesiser_channel_t;
typedef struct fm_synthesiser fm_synthesiser_t;

typedef void (* fm_synthesiser_play_t)(fm_synthesiser_channel_t * channel, uint16_t freq);
typedef void (* fm_synthesiser_wavectrl_t)(fm_synthesiser_channel_t * channel, int waveform);
typedef void (* fm_synthesiser_harmonicsctrl_t)(fm_synthesiser_channel_t * channel, int octave, int vibrato, int modulation);
typedef void (* fm_synthesiser_fxctrl_t)(fm_synthesiser_channel_t * channel, int feedback);
typedef void (* fm_synthesiser_volume_t)(fm_synthesiser_channel_t * channel, int volume, int freq_log);
typedef void (* fm_synthesiser_envelope_t)(fm_synthesiser_channel_t * channel, int attack, int decay, int sustain, int release);
typedef int (* fm_synthesiser_stereo_select_t)(fm_synthesiser_channel_t * channel, int speaker);
typedef void (* fm_synthesiser_pause_t)(fm_synthesiser_channel_t * channel);
typedef void (* fm_synthesiser_reset_channel_t)(fm_synthesiser_channel_t * channel);

typedef struct fm_synthesiser_channel {
	fm_synthesiser_t * synthesiser;
	fm_synthesiser_play_t play;
	fm_synthesiser_wavectrl_t wavectrl;
	fm_synthesiser_harmonicsctrl_t harmonicsctrl;
	fm_synthesiser_fxctrl_t fxctrl;
	fm_synthesiser_volume_t volume;
	fm_synthesiser_envelope_t envelope;
	fm_synthesiser_stereo_select_t select;
	fm_synthesiser_pause_t pause;
	fm_synthesiser_reset_channel_t reset;
	int playing;
	int channel;
	int speaker;
	int taken;
	void * priv;
} fm_synthesiser_channel_t;

typedef void (* fm_synthesiser_initialiser_t)(fm_synthesiser_t * synthesiser, fm_synthesiser_channel_t * channel, int id);
typedef int (* fm_synthesiser_supports_t)(fm_synthesiser_t * synthesiser, int feature);
typedef int (* fm_synthesiser_can_play_t)(fm_synthesiser_t * synthesiser, uint16_t freq);
typedef uint32_t (* fm_synthesiser_freq_range_t)(fm_synthesiser_t * synthesiser);
typedef void (* fm_synthesiser_reset_t)(fm_synthesiser_t * synthesiser);

typedef struct fm_synthesiser {
	fm_synthesiser_supports_t supports;
	fm_synthesiser_can_play_t can_play;
	fm_synthesiser_freq_range_t range;
	fm_synthesiser_reset_t reset;

	uint16_t * name;
	linked_t * channel_list;
	int opened_channels;
	int channels;
	void * priv;
} fm_synthesiser_t;

enum {
	SYNTH_SINEWAVE,
	SYNTH_HALF_SINEWAVE,
	SYNTH_RECTIFIED_SINEWAVE,
	SYNTH_NEGATIVE_SINEWAVE,

	SYNTH_SQUARE,
	SYNTH_HALF_SQUARE,
	SYNTH_RECTIFIED_SQUARE, // ?
	SYNTH_NEGATIVE_SQUARE,

	SYNTH_TRIANGLE,
	SYNTH_HALF_TRIANGLE,
	SYNTH_RECTIFIED_TRIANGLE,
	SYNTH_NEGATIVE_TRIANGLE,

	SYNTH_SAWTOOTH,
	SYNTH_HALF_SAWTOOTH,
	SYNTH_RECTIFIED_SAWTOOTH,
	SYNTH_NEGATIVE_SAWTOOTH,
};



fm_synthesiser_t * synth_create(uint16_t * name, fm_synthesiser_supports_t supports);
void synth_init_channels(fm_synthesiser_t * synthesiser, fm_synthesiser_initialiser_t initialiser, int channel);
void synth_register(fm_synthesiser_t * synthesiser);
fm_synthesiser_t * synth_open(uint16_t * name);
fm_synthesiser_channel_t * synth_open_channel(fm_synthesiser_t * synthesiser);
void synth_close_channel(fm_synthesiser_channel_t * channel);
int synth_supports(fm_synthesiser_t * synthesiser, int feature);
int synth_can_play(fm_synthesiser_t * synthesiser, uint16_t freq);
uint32_t synth_freq_range(fm_synthesiser_t * synthesiser);
void synth_reset(fm_synthesiser_t * synthesiser);

void synth_play(fm_synthesiser_channel_t * channel, uint16_t freq);
void synth_waveform_ctrl(fm_synthesiser_channel_t * channel, int waveform);
void synth_harmonics_ctrl(fm_synthesiser_channel_t * channel, int octave, int vibrato, int modulation);
void synth_fx_ctrl(fm_synthesiser_channel_t * channel, int feedback);
void synth_envelope_ctrl(fm_synthesiser_channel_t * channel, int attack, int decay, int sustain, int release);
void synth_pause_channel(fm_synthesiser_channel_t * channel);
void synth_reset_channel(fm_synthesiser_channel_t * channel);
void synth_init();