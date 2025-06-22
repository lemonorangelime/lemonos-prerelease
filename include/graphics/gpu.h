#pragma once

#include <stdint.h>

// lots of this is actually unimplimented

typedef struct gpu gpu_t;
typedef struct gpuvertex gpu_vertex_t;

typedef int (* gpu_cap_t)(int cap);
typedef int (* gpu_rect_fill_t)(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour);
typedef int (* gpu_rect_draw_t)(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp);
typedef int (* gpu_line_draw_t)(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour);
typedef int (* gpu_tri_draw_t)(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour);

typedef struct gpuvertex {
	float x;
	float y;
	float z;
} gpu_vertex_t;

typedef struct gpu {
	uint16_t * name;
	uint64_t ranking;
	gpu_cap_t get_cap;
	gpu_rect_fill_t rect_fill;
	gpu_rect_draw_t rect_draw;
	gpu_line_draw_t line_draw;
	gpu_tri_draw_t tri_draw;
	void * priv;
} gpu_t;

enum {
	GPUCAP_FUNC_LINE_DRAW,
	GPUCAP_FUNC_RECT_FILL,
	GPUCAP_FUNC_VECT_FILL, // complex vector shapes (octogon, pentagon)
	GPUCAP_FUNC_BLIT, // bit blitting
	GPUCAP_FUNC_ALPHA_COMPOSITE,
	GPUCAP_FUNC_FONT_DRAW,
	GPUCAP_FUNC_DITHER,
	GPUCAP_FUNC_MPEG_DECODE,
	GPUCAP_FUNC_SCALE,
	GPUCAP_FUNC_3D_TRI_DRAW,
	GPUCAP_FUNC_3D_QUAD_DRAW,
	GPUCAP_FUNC_3D_PARTICLE_DRAW,
	GPUCAP_FUNC_3D_LINE_DRAW,

	GPUCAP_FEAT_3D_ALPHA,
	GPUCAP_FEAT_3D_FOG,
	GPUCAP_FEAT_3D_TEXTURES,
	GPUCAP_FEAT_3D_REFLECTIONS,
	GPUCAP_FEAT_3D_SHADOWS,
	GPUCAP_FEAT_3D_SHADING,
	GPUCAP_FEAT_3D_LIGHTING,
	GPUCAP_FEAT_3D_LOD,
	GPUCAP_FEAT_3D_ANTI_ALIASING,
	GPUCAP_FEAT_3D_FILTERING, // I despise texture filtering but we will add driver support anyway
	GPUCAP_FEAT_3D_DITHERING,
	GPUCAP_FEAT_3D_STENCILS,
};

gpu_t * gpu_create(uint16_t * name);
gpu_t * gpu_get_default();
int gpu_rect_fill(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour);
int gpu_software_rect_fill(gpu_t * gpu, void * fb, int width, int height, int bpp, int x, int y, int src_width, int src_height, uint32_t colour);
int gpu_rect_draw(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp);
int gpu_software_rect_draw(gpu_t * gpu, void * fb, int width, int height, void * src_fb, int src_width, int src_height, int x, int y, int bpp);
int gpu_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour);
int gpu_software_line_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, int x1, int y1, int x2, int y2, uint32_t colour);
int gpu_tri_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour);
int gpu_software_tri_draw(gpu_t * gpu, void * fb, int width, int height, int bpp, gpu_vertex_t * a, gpu_vertex_t * b, gpu_vertex_t * c, int fov, uint32_t colour);
void gpu_register(gpu_t * gpu);
void gpu_init();
