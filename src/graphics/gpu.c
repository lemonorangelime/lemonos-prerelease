#include <graphics/gpu.h>
#include <graphics/graphics.h>
#include <memory.h>
#include <linked.h>
#include <string.h>
#include <math.h>

static linked_t * gpus;
static gpu_t * software_gpu = NULL;
static gpu_t * fastest_gpu = NULL;

gpu_t * gpu_create(uint16_t * name) {
	gpu_t * gpu = malloc(sizeof(gpu_t));
	memset(gpu, 0, sizeof(gpu_t));
	if (software_gpu) {
		gpu->ranking = 1;
	}
	gpu->name = ustrdup(name);
	return gpu;
}

void gpu_register(gpu_t * gpu) {
	if (gpu->ranking > fastest_gpu->ranking) {
		fastest_gpu = gpu;
	}
	gpus = linked_add(gpus, gpu);
}

gpu_t * gpu_get_default() {
	return fastest_gpu;
}

int gpu_software_get_cap(int cap) {
	return 0;
}

void * gpu_alloc(gpu_t * gpu, uint32_t size) {
	if (size == 0) {
		return NULL;
	}
	return gpu->alloc(gpu, size);
}

void * gpu_software_alloc(gpu_t * gpu, uint32_t size) {
	return malloc(size);
}

int gpu_transfer(gpu_t * gpu, void * buffer1, void * buffer2, uint32_t size, int direction) {
	if (size == 0) {
		return -1;
	}
	if (direction >= 2) {
		return -1;
	}
	return gpu->transfer(gpu, buffer1, buffer2, size, direction);
}

int gpu_software_transfer(gpu_t * gpu, void * buffer1, void * buffer2, uint32_t size, int direction) {
	memcpy(buffer1, buffer2, size); // well these are both CPU RAM pointers
	return 0;
}

int gpu_rect_fill(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour) {
	if (bpp > 32 || bpp < 8) {
		return -1;
	}
	if (bpp & 0b111) {
		return -1;
	}
	return gpu->rect_fill(gpu, fb, width, height, bpp, x, y, src_width, src_height, colour);
}

int gpu_software_rect_fill(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour) {
	uint32_t * dest;
	int ry = y; // position !
	int rx = x;
	int bytes = bpp >> 3;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;

	int r1premult_width = src_width * bytes;
	int r2premult_width = width * bytes;

	// clip image if its outside bounds
	if ((ry + src_height) >= height) {
		src_height -= (ry + src_height) - height;
	}
	if ((rx + src_width) >= width) {
		src_width -= (rx + src_width) - width;
	}
	for (int y = starty; y < src_height; y++) {
		dest = fb + ((y + ry) * r2premult_width) + ((startx + rx) * bytes);
		switch (bpp) {
			case 8: memset((void *) dest, colour, src_width - startx); break;
			case 15:
			case 16:
				memset16((void *) dest, colour, src_width - startx);
				break;
			case 24: {
				uint16_t * word = (uint16_t *) dest;
				uint8_t * byte = ((uint8_t *) dest) + 2;
				int size = src_width - startx;
				while (size--) {
					*word = colour & 0xffff;
					*byte = (colour >> 16) & 0xff;
					word = (uint16_t *) (((uint32_t) word) + 3);
					byte = (uint8_t *) (((uint32_t) byte) + 3);
				}
				break;
			}
			case 32: memset32(dest, colour, src_width - startx); break;
		}
	}
	return 1;
}

int gpu_rect_draw(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp) {
	if (bpp > 32 || bpp < 8) {
		return -1;
	}
	if (bpp & 0b111) {
		return -1;
	}
	return gpu->rect_draw(gpu, fb, width, height, src_fb, src_width, src_height, x, y, bpp);
}

int gpu_software_rect_draw(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp) {
	void * dest;
	void * src;
	int ry = y; // position !
	int rx = x;
	int starty = (ry < 0) ? abs32(ry) : 0; // if rect is offscreen, start at first visible pixel
	int startx = (rx < 0) ? abs32(rx) : 0;
	int stride = bpp >> 3;

	int r1premult_width = src_width * stride;
	int r2premult_width = width * stride;

	// clip image if its outside bounds
	if ((ry + src_height) >= height) {
		src_height -= (ry + src_height) - height;
	}
	if ((rx + src_width) >= width) {
		src_width -= (rx + src_width) - width;
	}
	for (int y = starty; y < src_height; y++) {
		src = ((void *) src_fb) + (y * r1premult_width) + (startx * stride);
		dest = ((void *) fb) + ((y + ry) * r2premult_width) + ((startx + rx) * stride);
		for (int x = startx; x < src_width; x++) {
			memcpy(dest, src, stride);
			dest += stride;
			src += stride;
		}
	}
	return 1;
}

int gpu_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour) {
	if (bpp > 32 || bpp < 8) {
		return -1;
	}
	if (bpp & 0b111) {
		return -1;
	}
	return gpu->line_draw(gpu, fb, width, height, bpp, x1, y1, x2, y2, colour);
}

void __safe_memcpy(void * dest, void * src, int size, int x, int width, int y, int height) {
	if ((x < 0) || (y < 0) || (x >= width) || (y >= height)) {
		return;
	}
	memcpy(dest, src, size);
}

int gpu_software_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour) {
	int dx = abs32(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = abs32(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) >> 2;
	int stride = bpp >> 3;

    while (x1 != x2 || y1 != y2) {
        int e2 = err;
		__safe_memcpy(fb + (((y1 * width) + x1) * stride), &colour, stride, x1, width, y1, height);
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 <  dy) { err += dx; y1 += sy; }
    }
}

int gpu_tri_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour) {
	if (bpp > 32 || bpp < 8) {
		return -1;
	}
	if (bpp & 0b111) {
		return -1;
	}
	return gpu->tri_draw(gpu, fb, width, height, bpp, a, b, c, fov, colour);
}

int gpu_software_tri_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour) {
	// cprintf(LEGACY_COLOUR_RED, u"Not rendering triangle in software.\n");
}

int gpu_crunch(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int src_bpp, int bpp) {
	if ((bpp > 32 || bpp < 8) || (src_bpp > 32 || src_bpp < 8)) {
		return -1;
	}
	if ((bpp & 0b111) || (src_bpp & 0b111)) {
		return -1;
	}
	return gpu->crunch(gpu, fb, width, height, src_fb, src_width, src_height, x, y, src_bpp, bpp);
}

uint32_t crunch_pop_pixel(void ** fb, int bpp) {
	uint32_t pixel = 0;
	memcpy(&pixel, *fb, bpp >> 3);
	*fb += bpp >> 3;
	return pixel;
}

void crunch_shift_pixel(void ** fb, uint32_t pixel, int bpp) {
	memcpy(*fb, &pixel, bpp >> 3);
	*fb += bpp >> 3;
}

uint32_t crunch_read_rgba32(void ** fb, int bpp) {
	uint8_t r, g, b;
	uint32_t pixel = crunch_pop_pixel(fb, bpp);
	switch (bpp) {
		case 8: {
			r = ((pixel >> 5) & 0b111) * 36;
			g = ((pixel >> 2) & 0b111) * 36;
			b = (pixel & 0b11) * 85;
			break;
		}
		case 15: {
			r = ((pixel >> 5) & 0b11111) * 8;
			g = ((pixel >> 2) & 0b11111) * 8;
			b = (pixel & 0b11111) * 8;
			break;
		}
		case 16: {
			r = ((pixel >> 5) & 0b11111) * 8;
			g = ((pixel >> 2) & 0b111111) * 4;
			b = (pixel & 0b11111) * 8;
			break;
		}
		case 24:
			return 0xff000000 | pixel;
		case 32:
			return pixel;
	}
	return 0xff000000 | (r << 16) | (g << 8) | b;
}

void crunch_write_pixel(void ** fb, uint32_t pixel, int bpp) {
	if ((bpp == 24) || (bpp == 32)) {
		crunch_shift_pixel(fb, pixel, bpp);
		return;
	}

	uint8_t r = (pixel >> 16) & 0xff;
	uint8_t g = (pixel >> 8) & 0xff;
	uint8_t b = pixel & 0xff;

	switch (bpp) {
		case 8: {
			r /= 32; // 32
			g /= 32;
			b /= 64; // 67
			crunch_shift_pixel(fb, (r << 5) | (g << 2) | b, bpp);
			return;
		}

		case 15: {
			r /= 8;
			g /= 8;
			b /= 8;
			crunch_shift_pixel(fb, (r << 10) | (g << 5) | b, bpp);
			return;
		}

		case 16: {
			r /= 8;
			g /= 4;
			b /= 8;
			crunch_shift_pixel(fb, (r << 11) | (g << 5) | b, bpp);
			return;
		}
	}
}

int gpu_software_crunch(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int src_bpp, int bpp) {
	if (src_bpp == bpp) {
		memcpy(fb, src_fb, width * height * (bpp >> 3));
		return 0;
	}

	int size = width * height;
	while (size--) {
		uint32_t pixel = crunch_read_rgba32(&src_fb, src_bpp);
		crunch_write_pixel(&fb, pixel, bpp);
	}
	return 0;
}

void gpu_init() {
	software_gpu = gpu_create(u"Software");
	software_gpu->get_cap = gpu_software_get_cap;
	software_gpu->alloc = gpu_software_alloc;
	software_gpu->transfer = gpu_software_transfer;
	software_gpu->rect_fill = gpu_software_rect_fill;
	software_gpu->rect_draw = gpu_software_rect_draw;
	software_gpu->line_draw = gpu_software_line_draw;
	software_gpu->tri_draw = gpu_software_tri_draw;
	software_gpu->crunch = gpu_software_crunch;
	software_gpu->ranking = 0;
	fastest_gpu = software_gpu;
	gpu_register(software_gpu);
}
