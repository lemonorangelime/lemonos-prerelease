#pragma once

#include <stdint.h>

void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, long val);
void outd(uint16_t port, uint32_t val);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
long inl(uint16_t port);
uint32_t ind(uint16_t port);

void inb_bin(uint16_t port, uint8_t * buffer, size_t length);
void inw_bin(uint16_t port, uint16_t * buffer, size_t length);
void inl_bin(uint16_t port, long * buffer, size_t length);
void ind_bin(uint16_t port, uint32_t * buffer, size_t length);