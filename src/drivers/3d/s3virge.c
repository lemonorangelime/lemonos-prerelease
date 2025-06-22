#include <bus/pci.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <pit.h>
#include <drivers/3d/s3virge.h>
#include <drivers/vga/vga.h>
#include <graphics/displays.h>
#include <graphics/gpu.h>

// WARNING: this is *VERY* unfinished

uint32_t s3virge_format_size(int width, int height) {
	return height | ((width - 1) << 16);
}

void s3virge_toggle_card(uint16_t mode) {
	uint8_t cr67 = vga_crtc_inb(0x67);
	cr67 &= 0b11110011;
	cr67 |= mode;
	vga_crtc_outb(0x67, cr67);
}

void s3virge_set_bits(uint8_t reg, uint8_t bits) {
	uint8_t v = vga_crtc_inb(reg);
	v |= bits;
	vga_crtc_outb(reg, v);
}

uint8_t s3virge_c67_format_bpp(int bpp) {
	switch (bpp) {
		case 8:
			return 0b0000;
		case 15:
			return 0b0011;
		case 16:
			return 0b0101;
		case 24:
		case 32:
			return 0b1101;
	}
}

uint8_t s3virge_ctrl_format_bpp(int bpp) {
	switch (bpp) {
		case 8:
			return 0b000;
		case 15:
			return 0b011;
		case 16:
			return 0b101;
		case 24:
			return 0b110;
		case 32:
			return 0b111;
	}
}

int s3virge_modeset(s3virge_t * virge, int width, int height, int bpp) {
	if (bpp == 4) {
		return 1;
	}
	uint8_t cr67 = vga_crtc_inb(0x67);
	cr67 &= 0b00000011;
	cr67 |= 0b00001100 | (s3virge_c67_format_bpp(bpp) << 4);
	vga_crtc_outb(0x67, cr67);

	virge->memory_controller->fifo_control = 0x0000c000; // disable
	virge->stream_controller->primary_stream_ctrl = s3virge_ctrl_format_bpp(bpp) << 24;
	virge->stream_controller->chroma_key_ctrl = 0;
	virge->stream_controller->secondary_stream_ctrl = 0x03000000;
	virge->stream_controller->chroma_key_upper = 0;
	virge->stream_controller->secondary_stream_scale = 0;
	virge->stream_controller->blend_ctrl = 0x01000000;
	virge->stream_controller->primary_stream_fb0 = 0;
	virge->stream_controller->primary_stream_fb1 = 0;
	virge->stream_controller->primary_stream_stride = width * ((bpp + 7) / 8);
	virge->stream_controller->double_buffer_support = 0;
	virge->stream_controller->secondary_stream_fb0 = 0;
	virge->stream_controller->secondary_stream_fb1 = 0;
	virge->stream_controller->secondary_stream_stride = 1;
	virge->stream_controller->opaque_overlay_ctrl = 0x40000000;
	virge->stream_controller->k1_scale_factor = 0;
	virge->stream_controller->k2_scale_factor = 0;
	virge->stream_controller->dda_init_vertical_accumulator = 0;
	virge->stream_controller->primary_stream_window_coord = 0x00010001;
	virge->stream_controller->primary_stream_window_size = s3virge_format_size(width, height);
	virge->stream_controller->secondary_stream_window_coord = 0x07ff07ff;
	virge->stream_controller->secondary_stream_window_size = 0x00010001;
	virge->memory_controller->fifo_control = 0x00006088; // enable
}

void s3virge_resize(display_t * display, int width, int height) {
	s3virge_t * virge = display->priv;
	if (s3virge_modeset(virge, width, height, display->bpp)) {
		return;
	}
	display->width = width;
	display->height = height;
}

void s3virge_crunch(display_t * display, int bpp) {
	s3virge_t * virge = display->priv;
	if (s3virge_modeset(virge, display->width, display->height, bpp)) {
		return;
	}
	display->bpp = bpp;
}

s3virge_t * s3virge_make_controller(pci_t * pci) {
	s3virge_t * virge = malloc(sizeof(s3virge_t));
	uint32_t base = pci->bar0;
	virge->stream_controller = (void *) (base + 0x1008180);
	virge->memory_controller = (void *) (base + 0x1008200);
	virge->misc = (void *) (base + 0x1008504);
	virge->dma = (void *) (base + 0x1008580);
	virge->lpb_bus = (void *) (base + 0x100ff00);
	virge->blitter_2d = (void *) (base + 0x100a4d4);
	virge->line_2d = (void *) (base + 0x100a8d4);
	virge->polygon_2d = (void *) (base + 0x100acd4);
	virge->triangle_3d = (void *) (base + 0x100b4d4);
	virge->base = base;
	return virge;
}

int s3virge_gpu_get_cap(int cap) {
	return 0;
}

// check wiki

int s3virge_gpu_rect_fill(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour) {
	int gpubpp = (bpp == 32) ? 24 : bpp;
	s3virge_t * virge = gpu->priv;
	int stride = width * (gpubpp >> 3);
	uint8_t format = gpubpp >> 3;
	virge->blitter_2d->dest_base = 0x12c000;
	virge->blitter_2d->mono_pattern_foreground = colour;
	virge->blitter_2d->stride = (stride << 16) | stride;
	virge->blitter_2d->rectangle_size = (width << 16) | height;
	virge->blitter_2d->rectangle_dest_coords = (x << 16) | y;
	virge->blitter_2d->cmd_set = (0xf0 << 17) | (1 << 8) | ((format - 1) << 2) | (0b11 << 25) | (2 << 27);

	uint64_t target = ticks + 5;
	while (ticks < target) {}

	if (bpp == 32) {
		uint32_t * dest = fb;
		void * src = (void *) (virge->base + 0x12c000);
		int size = width * height;
		while (size--) {
			*dest = 0xff000000;
			memcpy(dest, src, 3);
			dest++;
			src += 3;
		}
		return 0;
	}

	memcpy(fb, (void *) (virge->base + 0x12c000), width * height * (gpubpp >> 3));
	return 0;
}

int s3virge_gpu_rect_draw(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp) {
	int gpubpp = (bpp == 32) ? 24 : bpp;
	s3virge_t * virge = gpu->priv;
	int stride = width * (gpubpp >> 3);
	int src_stride = src_width * (gpubpp >> 3);
	uint8_t format = gpubpp >> 3;

	if (bpp == 32) {
		void * dest = (void *) (virge->base + 0x258000);
		uint32_t * src = src_fb;
		int size = width * height;
		while (size--) {
			memcpy(dest, src, 3);
			dest += 3;
			src++;
		}
	} else {
		memcpy((void *) (virge->base + 0x258000), src_fb, src_width * src_height * format);
	}
	virge->blitter_2d->src_base = 0x258000;
	virge->blitter_2d->dest_base = 0x12c000;
	virge->blitter_2d->stride = (stride << 16) | src_stride;
	virge->blitter_2d->rectangle_size = (src_width << 16) | src_height;
	virge->blitter_2d->rectangle_dest_coords = (x << 16) | y;
	virge->blitter_2d->cmd_set = (0xcc << 17) | (1 << 8) | ((format - 1) << 2) | (0b11 << 25) | (0 << 27);

	uint64_t target = ticks + 5;
	while (ticks < target) {}

	if (bpp == 32) {
		uint32_t * dest = fb;
		void * src = (void *) (virge->base + 0x12c000);
		int size = width * height;
		while (size--) {
			*dest = 0xff000000;
			memcpy(dest, src, 3);
			dest++;
			src += 3;
		}
		return 0;
	}

	memcpy(fb, (void *) (virge->base + 0x12c000), width * height * format);
	return 0;
}

int s3virge_gpu_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour) {
	int gpubpp = (bpp == 32) ? 24 : bpp;
	s3virge_t * virge = gpu->priv;
	int stride = width * (gpubpp >> 3);
	uint8_t format = gpubpp >> 3;
	if (y2 > y1) {
		int xtemp = x1;
		int ytemp = y1;
		x1 = x2;
		y1 = y2;
		x2 = xtemp;
		y2 = ytemp;
	}
	int32_t dx = x2 - x1;
	int32_t dy = y2 - y1;
	uint8_t major = (-dy > abs32(dx)) ? 1 : 0;
	uint8_t xdir = dx >= 0;
	int32_t xdelta = (dy == 0) ? 0 : (dx << 20) / -dy;
	int32_t xstart = x1 << 20;
	if (!major) {
		xstart -= xdelta / 2;
		if (xdelta < 0) {
			xstart += (1 << 20) - 1;
		}
	}
	virge->line_2d->dest_base = 0x12c000;
	virge->line_2d->stride = (stride << 16) | 0;
	virge->line_2d->rectangle_size = (width << 16) | height;
	virge->blitter_2d->mono_pattern_foreground = colour;
	virge->line_2d->line_endpoints = (x1 << 16) | x2;
	virge->line_2d->line_xdelta = xdelta;
	virge->line_2d->line_xstart = xstart;
	virge->line_2d->line_ystart = y1;
	virge->line_2d->line_ycount = (xdir << 31) | (-dy + 1);
	virge->line_2d->cmd_set = (0xf0 << 17) | (1 << 8) | ((format - 1) << 2) | (1 << 25) | (3 << 27);

	uint64_t target = ticks + 5;
	while (ticks < target) {}

	if (bpp == 32) {
		uint32_t * dest = fb;
		void * src = (void *) (virge->base + 0x12c000);
		int size = width * height;
		while (size--) {
			*dest = 0xff000000;
			memcpy(dest, src, 3);
			dest++;
			src += 3;
		}
		return 0;
	}

	memcpy(fb, (void *) (virge->base + 0x12c000), width * height * (gpubpp >> 3));
	return 0;
}

void s3virge_gpu_swap_vertex(gpu_vertex_t ** x, gpu_vertex_t ** y) {
	gpu_vertex_t * temp = *x;
	*x = *y;
	*y = temp;
}

int16_t s3virge_s87(int16_t v) {
	return (v << 7);
}
int32_t s3virge_s11_20(int32_t v) {
	return (v << 20);
}
int32_t s3virge_s11_20f(float v) {
	return v * (1 << 20);
}
int32_t s3virge_s16_15(int32_t v) {
	return (v << 15);
}

int16_t s3virge_interp(uint16_t v0, uint16_t v1, int16_t d, uint16_t dy01, uint16_t dy, int16_t _dx) {
	uint16_t v = v0 + d * dy01 / dy;
	return v1 - v;
}

int s3virge_gpu_tri_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour) {
	int gpubpp = (bpp == 32) ? 24 : bpp;
	s3virge_t * virge = gpu->priv;
	int stride = width * (gpubpp >> 3);
	uint8_t format = gpubpp >> 3;

	if (b->y > a->y) {
		s3virge_gpu_swap_vertex(&a, &b);
	}
	if (c->y > a->y) {
		s3virge_gpu_swap_vertex(&a, &c);
	}
	if (c->y > b->y) {
		s3virge_gpu_swap_vertex(&c, &b);
	}

	int16_t dx = b->x - a->x;
	int16_t dx2 = c->x - a->x;
	int16_t dy = a->y - c->y + 1;
	int16_t dy12 = b->x - c->x + 1;
	uint16_t dy01 = dy - dy12;
	int16_t dz = c->z - a->z;

	// dunno what this is but says to do it
	int16_t _dx = a->x + dx2 * dy01 / dy;
	_dx = b->x - _dx;

	uint8_t xdir = _dx >= 0; // are we drawing right to left or left to right
	_dx = _dx != 0 ? (_dx > 0 ? _dx : -_dx) : 1;
	virge->triangle_3d->triangle_gb_start = (s3virge_s87(colour >> 8 & 0xff) << 16) | s3virge_s87(colour & 0xff);
	virge->triangle_3d->triangle_ar_start = s3virge_s87(colour >> 16 & 0xff);
	virge->triangle_3d->triangle_gby_delta = 0;
	virge->triangle_3d->triangle_ary_delta = 0;
	virge->triangle_3d->triangle_gbx_delta = 0;
	virge->triangle_3d->triangle_arx_delta = 0;
	virge->triangle_3d->triangle_z_start = s3virge_s16_15(a->z);
	virge->triangle_3d->triangle_zy_delta = s3virge_s16_15(dz) / dy;
	virge->triangle_3d->triangle_zx_delta = s3virge_s16_15(s3virge_interp(a->z, b->z, dz, dy01, dy, _dx)) / _dx;
	virge->triangle_3d->dest_base_address = 0x12c000; // dynamically allocate this in the future
	virge->triangle_3d->stride = stride << 16;
	virge->triangle_3d->triangle_y_start = a->y;
	virge->triangle_3d->triangle_x_start = s3virge_s11_20(a->x + a->y * dx2 / dy);
	virge->triangle_3d->triangle_xy12_delta = dy12 ? s3virge_s11_20(c->x - b->x) / dy12 : 0;
	virge->triangle_3d->triangle_x12_end = s3virge_s11_20(b->x);
	virge->triangle_3d->triangle_xy01_delta = dy01 ? s3virge_s11_20(dx) / dy01 : 0;
	virge->triangle_3d->triangle_x01_end = s3virge_s11_20(a->x + (dy01 ? a->y * dx / dy01 : 0));
	virge->triangle_3d->triangle_xy02_delta = s3virge_s11_20(dx2) / dy;
	virge->triangle_3d->triangle_y_count = (xdir << 31) | (dy01 << 16) | dy12;
	virge->triangle_3d->horizontal_clip = width - 1;
	virge->triangle_3d->vertical_clip = height - 1;
	virge->triangle_3d->cmd_set = (1 << 31) | (0 << 27) | (0b11 << 24) | (0b111 << 20) | ((format - 1) << 2) | (1 << 1);

	uint64_t target = ticks + (pit_freq * 4);
	while (ticks < target) {}

	if (bpp == 32) {
		uint32_t * dest = fb;
		void * src = (void *) (virge->base + 0x12c000);
		int size = width * height;
		while (size--) {
			*dest = 0xff000000;
			memcpy(dest, src, 3);
			dest++;
			src += 3;
		}
		return 0;
	}

	memcpy(fb, (void *) (virge->base + 0x12c000), width * height * (gpubpp >> 3));
	return 0;
}

void s3virge_reset(pci_t * pci) {
	s3virge_t * virge = s3virge_make_controller(pci);
	display_t * display = display_create(DISPLAY_VGA, s3virge_resize, s3virge_crunch);
	display->bpp = root_window.bpp;
	display->fb = root_window.fb;
	display->height = root_window.size.height;
	display->width = root_window.size.width;
	display->pci = pci;
	display->priv = virge;
	display_register(display);
	display_listen(display, gfx_resize_callback, NULL);

	gpu_t * gpu = gpu_create(u"S3 ViRGE");
	gpu->get_cap = s3virge_gpu_get_cap;
	gpu->rect_fill = s3virge_gpu_rect_fill;
	gpu->rect_draw = s3virge_gpu_rect_draw;
	gpu->line_draw = s3virge_gpu_line_draw;
	gpu->tri_draw = s3virge_gpu_tri_draw;
	gpu->priv = virge;
	gpu_register(gpu);

	vga_crtc_outb(0x38, 0x48);
	vga_crtc_outb(0x39, 0xa5);

	s3virge_set_bits(0x03, 0x80);

	s3virge_set_bits(0x40, 0x01);
	s3virge_set_bits(0x53, 0x08);
	vga_crtc_outb(0x58, 0x13);

	s3virge_resize(display, display->width, display->height);
}

int s3virge_found = 0;
int s3virge_dev_init(pci_t * pci) {
	s3virge_found++;
	if (s3virge_found != 1) {
		return 0;
	}

	pci_cmd_set_flags(pci, PCI_CMD_MASTER | PCI_CMD_MEM | PCI_CMD_IO | PCI_CMD_WRITE | PCI_CMD_SNOOPY); // enable dma (master mode)
	s3virge_reset(pci);
}

int s3virge_check(pci_t * pci) {
	if (pci->vendor == 0x5333 && pci->device == 0x5631) {
		return 1; // S3 ViRGE GPU
	}
	if (pci->vendor == 0x5333 && pci->device == 0x8a10) {
		return 1; // S3 ViRGE GX GPU
	}
	return 0;
}

void s3virge_init() {
	pci_add_handler(s3virge_dev_init, s3virge_check);
}
