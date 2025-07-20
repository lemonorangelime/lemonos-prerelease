#include <drivers/audio/adlib.h>
#include <audio/synths.h>
#include <stdio.h>
#include <cpuspeed.h>
#include <ports.h>
#include <util.h>
#include <pit.h>
#include <multitasking.h>
#include <memory.h>

uint16_t adlib_format_packed_channel(int channel) {
	switch (channel) {
		case 0:
			return 0x0300;
		case 1:
			return 0x0401;
		case 2:
			return 0x0502;
		case 3:
			return 0x0b08;
		case 4:
			return 0x0c09;
		case 5:
			return 0x0d0a;
		case 6:
			return 0x1310;
		case 7:
			return 0x1411;
		case 8:
			return 0x1512;
	}
}

uint8_t adlib_stat(uint16_t channel) {
	return inb(channel);
}

void adlib_outb(uint16_t channel, uint8_t reg, uint8_t data) {
	outb(channel, reg);
	cpuspeed_wait_tsc(cpu_hz / 100000);
	outb(channel + 1, data);
	cpuspeed_wait_tsc(cpu_hz / 100000);
}

int adlib_detect() {
	adlib_outb(ADLIB_MONO, REG_TIMER_CTRL, TIMER_CTRL_DISABLE_TIMER0 | TIMER_CTRL_DISABLE_TIMER1);
	adlib_outb(ADLIB_MONO, REG_TIMER_CTRL, TIMER_CTRL_IRQ_RESET);
	cpuspeed_wait_tsc(cpu_hz / 100);
	uint8_t stat0 = adlib_stat(ADLIB_MONO) & 0xe0;
	adlib_outb(ADLIB_MONO, REG_TIMER0, 0xff);
	adlib_outb(ADLIB_MONO, REG_TIMER_CTRL, TIMER_CTRL_ENABLE_TIMER0 | TIMER_CTRL_DISABLE_TIMER0);
	cpuspeed_wait_tsc(cpu_hz / 100);
	uint8_t stat1 = adlib_stat(ADLIB_MONO) & 0xe0;
	adlib_outb(ADLIB_MONO, REG_TIMER_CTRL, TIMER_CTRL_DISABLE_TIMER0 | TIMER_CTRL_DISABLE_TIMER1);
	adlib_outb(ADLIB_MONO, REG_TIMER_CTRL, TIMER_CTRL_IRQ_RESET);
	if ((stat0 != 0x00) || (stat1 != 0xc0)) {
		return 0;
	}
	adlib_outb(ADLIB_MONO, REG_WAVEFORM_CTRL, 0b100000);
	return 1;
}

uint16_t adlib_freq2note(uint16_t freq) {
	return (freq) + (freq >> 2) + (freq >> 4); // evil math hack
}

uint8_t adlib_percent2nibble(int percent) {
	return (percent >> 3) + (percent >> 5);
}

void adlib_play(uint8_t modulator, uint8_t carrier, uint8_t waveform, uint8_t modulator_ctrl, uint8_t feedback, uint8_t attack, uint8_t release, uint16_t freq) {
	adlib_outb(ADLIB_MONO, REG_NOTE_LOW + modulator, freq & 0xff);
	adlib_outb(ADLIB_MONO, REG_MODULATOR_CTRL + modulator, modulator_ctrl);
	adlib_outb(ADLIB_MONO, REG_KEY_SCALER + modulator, 0x10);
	adlib_outb(ADLIB_MONO, REG_DECAY_RATE + modulator, attack);
	adlib_outb(ADLIB_MONO, REG_RELEASE_RATE + modulator, release);
	adlib_outb(ADLIB_MONO, REG_FEEDBACK + modulator, feedback);
	adlib_outb(ADLIB_MONO, REG_WAVEFORM + modulator, waveform);

	adlib_outb(ADLIB_MONO, REG_NOTE_LOW + carrier, freq & 0xff);
	adlib_outb(ADLIB_MONO, REG_MODULATOR_CTRL + carrier, modulator_ctrl);
	adlib_outb(ADLIB_MONO, REG_KEY_SCALER + carrier, 0);
	adlib_outb(ADLIB_MONO, REG_DECAY_RATE + carrier, attack);
	adlib_outb(ADLIB_MONO, REG_RELEASE_RATE + carrier, release);
	adlib_outb(ADLIB_MONO, REG_FEEDBACK + carrier, feedback);
	adlib_outb(ADLIB_MONO, REG_WAVEFORM + carrier, waveform);

	adlib_outb(ADLIB_MONO, REG_NOTE_HIGH + modulator, 0x30 | ((freq >> 8) & 0xff));
}

void adlib_update_player(fm_synthesiser_channel_t * channel, adlib_priv_t * priv) {
	uint8_t modulator = priv->packed_channel & 0xff;
	uint8_t carrier = (priv->packed_channel >> 8) & 0xff;
	if (!channel->playing) {
		return;
	}
	adlib_play(modulator, carrier, priv->waveform, priv->modulator_ctrl, priv->feedback, priv->attack, priv->release, adlib_freq2note(priv->freq));
}

void adlib_channel_play(fm_synthesiser_channel_t * channel, uint16_t freq) {
	adlib_priv_t * priv = channel->priv;
	priv->freq = freq;
	adlib_update_player(channel, priv);
}

void adlib_waveform_ctrl(fm_synthesiser_channel_t * channel, int waveform_id) {
	adlib_priv_t * priv = channel->priv;
	uint8_t modulator = priv->packed_channel & 0xff;
	uint8_t carrier = (priv->packed_channel >> 8) & 0xff;
	uint8_t waveform = priv->waveform;
	switch (waveform_id) {
		case SYNTH_SINEWAVE:
			waveform = 0b00;
			break;
		case SYNTH_HALF_SINEWAVE:
			waveform = 0b01;
			break;
		case SYNTH_RECTIFIED_SINEWAVE:
			waveform = 0b10;
			break;
		case SYNTH_HALF_SAWTOOTH:
			waveform = 0b11;
			break;
	}
	if (waveform == priv->waveform) {
		return;
	}
	priv->waveform = waveform;
	adlib_update_player(channel, priv);
}

void adlib_harmonics_ctrl(fm_synthesiser_channel_t * channel, int octave, int vibrato, int modulation) {
	fm_synthesiser_t * synthesiser = channel->synthesiser;
	adlib_synth_priv_t * synth_priv = synthesiser->priv;
	adlib_priv_t * priv = channel->priv;

	priv->modulator_ctrl |= (vibrato != 0) << 6;
	priv->modulator_ctrl |= (modulation != 0) << 7;
	if ((priv->modulator_ctrl & 0b11000000) == 0) {
		return;
	}

	uint8_t vibrato_bit = vibrato >= 10;
	uint8_t modulation_bit = modulation >= 2;
	synth_priv->music_ctrl |= (modulation_bit << 7) | vibrato_bit << 6;
	adlib_outb(ADLIB_MONO, REG_MUSIC_CTRL, synth_priv->music_ctrl);
	adlib_update_player(channel, priv);
}

void adlib_fx_ctrl(fm_synthesiser_channel_t * channel, int feedback) {
	adlib_priv_t * priv = channel->priv;
	priv->feedback = (feedback / 14) << 1;
	adlib_update_player(channel, priv);
}

void adlib_envelope_ctrl(fm_synthesiser_channel_t * channel, int attack, int decay, int sustain, int release) {
	adlib_priv_t * priv = channel->priv;
	priv->attack = (adlib_percent2nibble(attack) << 4) | adlib_percent2nibble(decay);
	priv->release = (adlib_percent2nibble(sustain) << 4) | adlib_percent2nibble(release);
	adlib_update_player(channel, priv);
}

void adlib_pause(fm_synthesiser_channel_t * channel) {
	adlib_priv_t * priv = channel->priv;
	uint8_t modulator = priv->packed_channel & 0xff;
	adlib_outb(ADLIB_MONO, REG_NOTE_HIGH + modulator, 0);
}

void adlib_reset(fm_synthesiser_t * synthesiser) {
}

void adlib_channel_initialiser(fm_synthesiser_t * synthesiser, fm_synthesiser_channel_t * channel, int id) {
	adlib_priv_t * priv = malloc(sizeof(adlib_priv_t));
	priv->freq = 0;
	priv->packed_channel = adlib_format_packed_channel(id);
	priv->feedback = 0;
	priv->modulator_ctrl = 1;
	priv->waveform = 0;
	priv->attack = 0xf0;
	priv->release = 0x77;

	channel->priv = priv;
	channel->play = adlib_channel_play;
	channel->wavectrl = adlib_waveform_ctrl;
	channel->harmonicsctrl = adlib_harmonics_ctrl;
	channel->envelope = adlib_envelope_ctrl;
	channel->fxctrl = adlib_fx_ctrl;
	channel->pause = adlib_pause;
	channel->reset = adlib_pause;
}

int adlib_supports(fm_synthesiser_t * synthesiser, int feature) {
	switch (feature) {
		case SYNTH_SINEWAVE:
			return 1;
		case SYNTH_HALF_SINEWAVE:
			return 1;
		case SYNTH_RECTIFIED_SINEWAVE:
			return 1;
		case SYNTH_HALF_SAWTOOTH:
			return 1;
	}
	return 0;
}

int adlib_can_play(fm_synthesiser_t * synthesiser, uint16_t freq) {
	return freq < 585;
}

uint32_t adlib_range(fm_synthesiser_t * synthesiser) {
	return 585;
}

void adlib_init_synthesiser(fm_synthesiser_t * synthesiser) {
	adlib_synth_priv_t * priv = synthesiser->priv;
	priv->music_ctrl = 0;
}

void adlib_init() {
	if (!adlib_detect()) {
		return;
	}

	printf(u"ADLIB: detected Adlib synthesiser\n");

	fm_synthesiser_t * synthesiser = synth_create(u"adlib", adlib_supports);
	synthesiser->can_play = adlib_can_play;
	synthesiser->range = adlib_range;
	synthesiser->reset = adlib_reset;
	synthesiser->priv = malloc(sizeof(adlib_synth_priv_t));
	adlib_init_synthesiser(synthesiser);
	synth_init_channels(synthesiser, adlib_channel_initialiser, 1); // 3 channels
	synth_register(synthesiser);

	fm_synthesiser_t * adlib = synth_open(u"adlib");
	fm_synthesiser_channel_t * channel = synth_open_channel(adlib); // grab a channel
	//synth_waveform_ctrl(channel, SYNTH_HALF_SAWTOOTH); // half sawtooth wave
	//synth_harmonics_ctrl(channel, 0, 0, 0); // 14 cent vibrato and 5 dB modulation
	//synth_envelope_ctrl(channel, 50, 0, 50, 50);
	synth_play(channel, 55);
	sleep_seconds(1); // play that noise for 1 seconds

	int i = 100; // software vibrato
	int direction = 1;
	int delta = 0;
	int freq = 55;
	int c = 0;
	while (i--) {
		int target = 55 + (delta * direction);
		while (freq != target) {
			synth_play(channel, freq);
			cpuspeed_wait_tsc(cpu_hz / 10000);
			freq += direction;
		}
		c += 1;
		if (c == 6) {
			c = 0;
			delta += 1;
		}
		direction = -direction;
	}
	
	// clean up
	synth_close_channel(channel);
}