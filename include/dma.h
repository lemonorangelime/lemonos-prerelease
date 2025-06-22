#pragma once

#include <stdint.h>

enum {
	DMA0_CHANNEL0_ADDRESS_REG,
	DMA0_CHANNEL0_COUNT_REG,
	DMA0_CHANNEL1_ADDRESS_REG,
	DMA0_CHANNEL1_COUNT_REG,
	DMA0_CHANNEL2_ADDRESS_REG,
	DMA0_CHANNEL2_COUNT_REG,
	DMA0_CHANNEL3_ADDRESS_REG,
	DMA0_CHANNEL3_COUNT_REG,
	DMA0_STATUS_REG,
	DMA0_COMMAND_REG = 8,
	DMA0_REQUEST_REG,
	DMA0_CHANNEL_MASK_REG,
	DMA0_MODE_REG,
	DMA0_FLIPFLOP_REG,
	DMA0_TEMP_REG = 13,
	DMA0_RESET_REG = 13,
	DMA0_MASK_RESET_REG,
	DMA0_MULTICHANNEL_REG,
};

enum {
	DMA1_CHANNEL4_ADDRESS_REG = 0xc0,
	DMA1_CHANNEL4_COUNT_REG = 0xc2,
	DMA1_CHANNEL5_ADDRESS_REG = 0xc4,
	DMA1_CHANNEL5_COUNT_REG = 0xc6,
	DMA1_CHANNEL6_ADDRESS_REG = 0xc8,
	DMA1_CHANNEL6_COUNT_REG = 0xca,
	DMA1_CHANNEL7_ADDRESS_REG = 0xcc,
	DMA1_CHANNEL7_COUNT_REG = 0xce,
	DMA1_STATUS_REG = 0xd0,
	DMA1_COMMAND_REG = 0xd0,
	DMA1_REQUEST_REG = 0xd2,
	DMA1_CHANNEL_MASK_REG = 0xd4,
	DMA1_MODE_REG = 0xd6,
	DMA1_FLIPFLOP_REG = 0xd8,
	DMA1_TEMP_REG = 0xda,
	DMA1_RESET_REG = 0xda,
	DMA1_MASK_RESET_REG = 0xdc,
	DMA1_MULTICHANNEL_REG = 0xde,
};

enum {
	DMA_CMD_MASK_MEMTOMEM = 1, // huh
	DMA_CMD_MASK_CHANNEL0HOLD = 2,
	DMA_CMD_MASK_ENABLE = 4,
	DMA_CMD_MASK_TIMING = 8,
	DMA_CMD_MASK_PRIORITY = 16,
	DMA_CMD_MASK_SELECT = 32,
	DMA_CMD_MASK_REQ = 64,
	DMA_CMD_MASK_ACK = 128,
};

enum {
	DMA_MODE_MASK_SELECT = 3,
	DMA_MODE_MASK_TRANSFER_TYPE = 0x0c,
	DMA_MODE_SELF_TEST = 0,
	DMA_MODE_READ_TRANSFER = 4,
	DMA_MODE_WRITE_TRANSFER = 8,
	DMA_MODE_MASK_AUTO = 16,
	DMA_MODE_MASK_IDEC = 32,
	DMA_MODE_MASK = 0xc0,
	DMA_MODE_TRANSFER_ONDEMAND = 0,
	DMA_MODE_TRANSFER_SINGLE = 64,
	DMA_MODE_TRANSFER_BLOCK = 128,
	DMA_MODE_TRANSFER_CASCADE = 0xc0,
};

void dma_mask_channel(uint8_t channel);
void dma_unmask_channel(uint8_t channel);
void dma_mask_reset();
void dma_reset();
void dma_flipflip_reset(int dma);
uint16_t dma_channel_to_port(uint8_t channel);
uint16_t dma_channel_to_dma(uint8_t channel);
uint16_t dma_channel_to_channel(uint8_t channel);
void dma_set_address(uint8_t channel, uint8_t low, uint8_t high);
void dma_set_count(uint8_t channel, uint8_t low, uint8_t high);
void dma_set_mode(uint8_t channel, uint8_t mode);
void dma_prepare_read(uint8_t channel);
void dma_prepare_write(uint8_t channel);