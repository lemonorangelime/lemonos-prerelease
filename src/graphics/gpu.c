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
		memcpy(gpu, software_gpu, sizeof(gpu_t)); // extend off software gpu
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

int gpu_software_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour) {
	int dx = abs32(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = abs32(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) >> 2;
	int stride = bpp >> 3;

    while (x1 != x2 || y1 != y2) {
        int e2 = err;
		memcpy(fb + (((y1 * width) + x1) * stride), &colour, stride);
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

void gpu_init() {
	software_gpu = gpu_create(u"Software");
	software_gpu->get_cap = gpu_software_get_cap;
	software_gpu->rect_fill = gpu_software_rect_fill;
	software_gpu->rect_draw = gpu_software_rect_draw;
	software_gpu->line_draw = gpu_software_line_draw;
	software_gpu->tri_draw = gpu_software_tri_draw;
	software_gpu->ranking = 0;
	fastest_gpu = software_gpu;
	gpu_register(software_gpu);
}
