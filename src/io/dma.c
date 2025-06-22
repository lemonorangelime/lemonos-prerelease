#include <stdint.h>
#include <dma.h>
#include <ports.h>

// doesnt work

void dma_mask_channel(uint8_t channel) {
	if (channel <= 4) {
		outb(DMA0_CHANNEL_MASK_REG, 1 << (channel - 1));
		return;
	}
	outb(DMA1_CHANNEL_MASK_REG, 1 << (channel - 4));
}

void dma_unmask_channel(uint8_t channel) {
	if (channel <= 4) {
		outb(DMA0_CHANNEL_MASK_REG, channel);
		return;
	}
	outb(DMA1_CHANNEL_MASK_REG, channel);
}

void dma_mask_reset() {
	outb(DMA1_MASK_RESET_REG, 1);
}

void dma_reset() {
	outb(DMA0_RESET_REG, 1);
}

void dma_flipflip_reset(int dma) {
	outb(dma ? DMA1_FLIPFLOP_REG : DMA0_FLIPFLOP_REG, 1);
}

uint16_t dma_channel_to_port(uint8_t channel) {
	if (channel <= 4) {
		return DMA0_CHANNEL0_ADDRESS_REG + (channel * 2);
	}
	return DMA1_CHANNEL4_ADDRESS_REG + (channel * 4);
}

uint16_t dma_channel_to_count_port(uint8_t channel) {
	if (channel <= 4) {
		return DMA0_CHANNEL0_COUNT_REG + (channel * 2);
	}
	return DMA1_CHANNEL4_COUNT_REG + (channel * 4);
}

uint16_t dma_channel_to_dma(uint8_t channel) {
	return channel > 4;
}

// shit name
uint16_t dma_channel_to_channel(uint8_t channel) {
	return channel - ((channel > 4) * 4);
}

void dma_set_address(uint8_t channel, uint8_t low, uint8_t high) {
	uint16_t port = dma_channel_to_port(channel);
	outb(port, low);
	outb(port, high);
}

void dma_set_count(uint8_t channel, uint8_t low, uint8_t high) {
	uint16_t port = dma_channel_to_count_port(channel);
	outb(port, low);
	outb(port, high);
}

void dma_set_mode(uint8_t channel, uint8_t mode) {
	int dma = dma_channel_to_dma(channel);
	int channel2 = dma_channel_to_channel(channel);
	dma_mask_channel(channel);
	outb(dma ? DMA1_MODE_REG : DMA0_MODE_REG, channel2 | mode);
	dma_mask_reset();
}

void dma_prepare_read(uint8_t channel) {
	dma_set_mode(channel, DMA_MODE_READ_TRANSFER | DMA_MODE_TRANSFER_SINGLE);
}

void dma_prepare_write(uint8_t channel) {
	dma_set_mode(channel, DMA_MODE_WRITE_TRANSFER | DMA_MODE_TRANSFER_SINGLE);
}
