#include <audio/synths.h>
#include <stdint.h>
#include <string.h>
#include <linked.h>
#include <memory.h>

static linked_t * synthesisers = NULL;

int synth_dummy_function() {
	return 0;
}

int synth_dummy_true_function() {
	return 1;
}

uint32_t synth_dummy_range_function(fm_synthesiser_t * synthesiser) {
	return 0x0000ffff;
}

fm_synthesiser_t * synth_create(uint16_t * name, fm_synthesiser_supports_t supports) {
	fm_synthesiser_t * synthesiser = malloc(sizeof(fm_synthesiser_t));
	synthesiser->supports = supports;
	synthesiser->range = synth_dummy_range_function;
	synthesiser->can_play = (fm_synthesiser_can_play_t) synth_dummy_true_function;
	synthesiser->name = ustrdup(name);
	synthesiser->priv = NULL;
	synthesiser->channel_list = NULL;
	synthesiser->channels = 0;
	return synthesiser;
}

void synth_init_channels(fm_synthesiser_t * synthesiser, fm_synthesiser_initialiser_t initialiser, int channels) {
	synthesiser->channels = channels;

	while (channels--) {
		fm_synthesiser_channel_t * channel = malloc(sizeof(fm_synthesiser_channel_t));
		channel->synthesiser = synthesiser;
		channel->play = (fm_synthesiser_play_t) synth_dummy_function;
		channel->wavectrl = (fm_synthesiser_wavectrl_t) synth_dummy_function;
		channel->harmonicsctrl = (fm_synthesiser_harmonicsctrl_t) synth_dummy_function;
		channel->fxctrl = (fm_synthesiser_fxctrl_t) synth_dummy_function;
		channel->volume = (fm_synthesiser_volume_t) synth_dummy_function;
		channel->envelope = (fm_synthesiser_envelope_t) synth_dummy_function;
		channel->pause = (fm_synthesiser_pause_t) synth_dummy_function;
		channel->reset = (fm_synthesiser_reset_channel_t) synth_dummy_function;
		channel->channel = channels;
		channel->playing = 0;
		channel->speaker = 0;
		channel->taken = 0;
		channel->priv = NULL;
		initialiser(synthesiser, channel, channels);

		synthesiser->channel_list = linked_add(synthesiser->channel_list, channel);
	}
}

fm_synthesiser_channel_t * synth_open_channel(fm_synthesiser_t * synthesiser) {
	if (synthesiser->opened_channels >= synthesiser->channels) {
		return NULL;
	}

	linked_iterator_t iterator = {synthesiser->channel_list};
	linked_t * node = synthesiser->channel_list;
	while (node) {
		fm_synthesiser_channel_t * channel = node->p;
		if (channel->taken == 0) {
			channel->taken = 1;
			synthesiser->opened_channels += 1;
			return channel;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

void synth_close_channel(fm_synthesiser_channel_t * channel) {
	fm_synthesiser_t * synthesiser = channel->synthesiser;
	if (synthesiser->opened_channels >= synthesiser->channels) {
		return;
	}

	synth_reset_channel(channel);
	channel->taken = 0;
	synthesiser->opened_channels -= 1;
}

void synth_register(fm_synthesiser_t * synthesiser) {
	synthesisers = linked_add(synthesisers, synthesiser);
}

fm_synthesiser_t * synth_open(uint16_t * name) {
	linked_iterator_t iterator = {synthesisers};
	linked_t * node = synthesisers;
	if (name == NULL) {
		if (!node) {
			return NULL;
		}
		return node->p;
	}
	while (node) {
		fm_synthesiser_t * synthesiser = node->p;
		if (ustrcmp(synthesiser->name, name) == 0) {
			return synthesiser;
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

int synth_supports(fm_synthesiser_t * synthesiser, int feature) {
	if (!synthesiser) {
		return -1;
	}
	return synthesiser->supports(synthesiser, feature);
}

int synth_can_play(fm_synthesiser_t * synthesiser, uint16_t freq) {
	if (!synthesiser) {
		return 0;
	}
	return synthesiser->can_play(synthesiser, freq);
}

uint32_t synth_freq_range(fm_synthesiser_t * synthesiser) {
	if (!synthesiser) {
		return 0;
	}
	return synthesiser->range(synthesiser);
}

void synth_reset(fm_synthesiser_t * synthesiser) {
	if (!synthesiser) {
		return;
	}
	return synthesiser->reset(synthesiser);
}

void synth_play(fm_synthesiser_channel_t * channel, uint16_t freq) {
	if (!channel) {
		return;
	}
	channel->playing = 1;
	return channel->play(channel, freq);
}

void synth_waveform_ctrl(fm_synthesiser_channel_t * channel, int waveform) {
	if (!channel) {
		return;
	}
	return channel->wavectrl(channel, waveform);
}

void synth_harmonics_ctrl(fm_synthesiser_channel_t * channel, int octave, int vibrato, int modulation) {
	if (!channel) {
		return;
	}
	return channel->harmonicsctrl(channel, octave, vibrato, modulation);
}

void synth_fx_ctrl(fm_synthesiser_channel_t * channel, int feedback) {
	if (!channel) {
		return;
	}
	return channel->fxctrl(channel, feedback);
}

void synth_envelope_ctrl(fm_synthesiser_channel_t * channel, int attack, int decay, int sustain, int release) {
	if (!channel) {
		return;
	}
	return channel->envelope(channel, attack, decay, sustain, release);
}

int synth_stereo_select(fm_synthesiser_channel_t * channel, int speaker) {
	if (!channel) {
		return -1;
	}
	int ret = channel->select(channel, speaker);
	if (ret == -1) {
		return -1;
	}
	channel->speaker = speaker;
	return ret;
}

void synth_pause_channel(fm_synthesiser_channel_t * channel) {
	if (!channel) {
		return;
	}
	channel->playing = 0;
	return channel->pause(channel);
}

void synth_reset_channel(fm_synthesiser_channel_t * channel) {
	if (!channel) {
		return;
	}
	channel->playing = 0;
	return channel->reset(channel);
}

void synth_init() {
	
}