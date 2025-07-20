#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory/allocators.h>

typedef struct {
	uint32_t primary_stream_ctrl;
	uint32_t chroma_key_ctrl;
	char padding1[8];
	uint32_t secondary_stream_ctrl;
	uint32_t chroma_key_upper;
	uint32_t secondary_stream_scale;
	char padding2[4];
	uint32_t blend_ctrl;
	char padding3[28];
	uint32_t primary_stream_fb0;
	uint32_t primary_stream_fb1;
	uint32_t primary_stream_stride;
	uint32_t double_buffer_support;
	uint32_t secondary_stream_fb0;
	uint32_t secondary_stream_fb1;
	uint32_t secondary_stream_stride;
	uint32_t opaque_overlay_ctrl;
	uint32_t k1_scale_factor;
	uint32_t k2_scale_factor;
	uint32_t dda_init_vertical_accumulator;
	uint32_t stream_fifo_ctrl;
	uint32_t primary_stream_window_coord;
	uint32_t primary_stream_window_size;
	uint32_t secondary_stream_window_coord;
	uint32_t secondary_stream_window_size;
} __attribute__((packed)) s3virge_stream_controller_t;

typedef struct {
	uint32_t fifo_control;
	uint32_t miu_ctrl;
	uint32_t stream_timeouts;
	uint32_t misc_timeouts;
	char padding1[16];
	uint32_t dma_read_base;
	uint32_t dma_read_stride;
} __attribute__((packed)) s3virge_memory_controller_t;

typedef struct {
	uint32_t subsystem_status;
	uint32_t padding1;
	uint32_t advanced_ctrl;
} __attribute__((packed)) s3virge_misc_t;

typedef struct {
	uint32_t video_dma_base;
	uint32_t video_dma_size;
	uint32_t video_dma_start;
	uint32_t padding1;
	uint32_t command_dma_base;
	uint32_t command_dma_write_ptr;
	uint32_t command_dma_read_ptr;
	uint32_t command_dma_start;
} __attribute__((packed)) s3virge_dma_t;

typedef struct {
	uint32_t lpb_mode;
	uint32_t lpb_fifo_status;
	uint32_t lpb_irq_flags;
	uint32_t lpb_fb0_base;
	uint32_t lpb_fb1_base;
	uint32_t lpb_direct_address;
	uint32_t lpb_direct_data;
	uint32_t lpb_io_port;
	uint32_t lpb_serial;
	uint32_t lpb_input_win_size;
	uint32_t lpb_video_data_offsets;
	uint32_t lpb_horizontal_decimation_ctrl;
	uint32_t lpb_vertical_decimation_ctrl;
	uint32_t lpb_line_stride;
} __attribute__((packed)) s3virge_lpb_bus_t;

typedef struct {
	uint32_t src_base;
	uint32_t dest_base;
	uint32_t horizontal_clip;
	uint32_t vertical_clip;
	uint32_t stride;
	uint32_t mono_pattern0;
	uint32_t mono_pattern1;
	uint32_t mono_pattern_background;
	uint32_t mono_pattern_foreground;
	uint32_t src_background;
	uint32_t src_foreground;
	uint32_t cmd_set;
	uint32_t rectangle_size;
	uint32_t rectangle_src_coords;
	uint32_t rectangle_dest_coords;
} __attribute__((packed)) s3virge_2d_controller_t;

typedef struct {
	uint32_t src_base;
	uint32_t dest_base;
	uint32_t horizontal_clip;
	uint32_t vertical_clip;
	uint32_t stride;
	uint32_t mono_pattern0;
	uint32_t mono_pattern1;
	uint32_t mono_pattern_background;
	uint32_t mono_pattern_foreground;
	uint32_t src_background;
	uint32_t src_foreground;
	uint32_t cmd_set;
	uint32_t rectangle_size;
	uint32_t rectangle_src_coords;
	uint32_t rectangle_dest_coords;
	char padding[92];
	uint32_t line_endpoints;
	uint32_t line_xdelta;
	uint32_t line_xstart;
	uint32_t line_ystart;
	uint32_t line_ycount;
} __attribute__((packed)) s3virge_2d_line_controller_t;

typedef struct {
	uint32_t src_base;
	uint32_t dest_base;
	uint32_t horizontal_clip;
	uint32_t vertical_clip;
	uint32_t stride;
	uint32_t mono_pattern0;
	uint32_t mono_pattern1;
	uint32_t mono_pattern_background;
	uint32_t mono_pattern_foreground;
	uint32_t src_background;
	uint32_t src_foreground;
	uint32_t cmd_set;
	uint32_t rectangle_size;
	uint32_t rectangle_src_coords;
	uint32_t rectangle_dest_coords;
	char padding[92];
	uint32_t polygon_right_xdelta;
	uint32_t polygon_right_xstart;
	uint32_t polygon_left_xdelta;
	uint32_t polygon_left_xstart;
	uint32_t polygon_ystart;
	uint32_t polygon_ycount;
} __attribute__((packed)) s3virge_2d_polygon_controller_t;

typedef struct {
	uint32_t z_base_address;
	uint32_t dest_base_address;
	uint32_t horizontal_clip;
	uint32_t vertical_clip;
	uint32_t stride;
	uint32_t z_stride;
	uint32_t texture_base_address;
	uint32_t texture_border_colour;
	uint32_t fog_colour;
	uint32_t colour0;
	uint32_t colour1;
	uint32_t cmd_set;
	uint32_t line_gb_delta;
	uint32_t line_ar_delta;
	uint32_t line_gb_start;
	uint32_t line_ar_start;
	uint32_t padding1;
	uint32_t line_z_delta;
	uint32_t line_z_start;
	char padding2[12];
	uint32_t line_endpoints;
	uint32_t line_x_delta;
	uint32_t line_x_start;
	uint32_t line_y_start;
	char padding3[904];
	uint32_t triangle_v_base;
	uint32_t triangle_u_base;
	uint32_t triangle_wx_delta;
	uint32_t triangle_wy_delta;
	uint32_t triangle_w_start;
	uint32_t triangle_dx_delta;
	uint32_t triangle_vx_delta;
	uint32_t triangle_ux_delta;
	uint32_t triangle_dy_delta;
	uint32_t triangle_vy_delta;
	uint32_t triangle_uy_delta;
	uint32_t triangle_d_start;
	uint32_t triangle_v_start;
	uint32_t triangle_u_start;
	uint32_t triangle_gbx_delta;
	uint32_t triangle_arx_delta;
	uint32_t triangle_gby_delta;
	uint32_t triangle_ary_delta;
	uint32_t triangle_gb_start;
	uint32_t triangle_ar_start;
	uint32_t triangle_zx_delta;
	uint32_t triangle_zy_delta;
	uint32_t triangle_z_start;
	uint32_t triangle_xy12_delta;
	uint32_t triangle_x12_end;
	uint32_t triangle_xy01_delta;
	uint32_t triangle_x01_end;
	uint32_t triangle_xy02_delta;
	uint32_t triangle_x_start;
	uint32_t triangle_y_start;
	uint32_t triangle_y_count;
} __attribute__((packed)) s3virge_3d_controller_t;

typedef struct {
	uint32_t z_base_address;
	uint32_t dest_base_address;
	uint32_t horizontal_clip;
	uint32_t vertical_clip;
	uint32_t stride;
	uint32_t z_stride;
	uint32_t texture_base_address;
	uint32_t texture_border_colour;
	uint32_t fog_colour;
	uint32_t colour0;
	uint32_t colour1;
	uint32_t cmd_set;
	uint32_t triangle_v_base;
	uint32_t triangle_u_base;
	uint32_t triangle_wx_delta;
	uint32_t triangle_wy_delta;
	uint32_t triangle_w_start;
	uint32_t triangle_dx_delta;
	uint32_t triangle_vx_delta;
	uint32_t triangle_ux_delta;
	uint32_t triangle_dy_delta;
	uint32_t triangle_vy_delta;
	uint32_t triangle_uy_delta;
	uint32_t triangle_d_start;
	uint32_t triangle_v_start;
	uint32_t triangle_u_start;
	uint32_t triangle_gbx_delta;
	uint32_t triangle_arx_delta;
	uint32_t triangle_gby_delta;
	uint32_t triangle_ary_delta;
	uint32_t triangle_gb_start;
	uint32_t triangle_ar_start;
	uint32_t triangle_zx_delta;
	uint32_t triangle_zy_delta;
	uint32_t triangle_z_start;
	uint32_t triangle_xy12_delta;
	uint32_t triangle_x12_end;
	uint32_t triangle_xy01_delta;
	uint32_t triangle_x01_end;
	uint32_t triangle_xy02_delta;
	uint32_t triangle_x_start;
	uint32_t triangle_y_start;
	uint32_t triangle_y_count;
} __attribute__((packed)) s3virge_3d_triangle_controller_t;

typedef struct {
	volatile s3virge_stream_controller_t * stream_controller;
	volatile s3virge_memory_controller_t * memory_controller;
	volatile s3virge_misc_t * misc;
	volatile s3virge_dma_t * dma;
	volatile s3virge_lpb_bus_t * lpb_bus;
	volatile s3virge_2d_controller_t * blitter_2d;
	volatile s3virge_2d_line_controller_t * line_2d;
	volatile s3virge_2d_polygon_controller_t * polygon_2d;
	volatile s3virge_3d_triangle_controller_t * triangle_3d;
	allocator_t * allocator;
	uint32_t base;
	uint32_t allocator_base;
} s3virge_t;

enum {
	S3_VIRGE_4MB = 0b000,
	S3_VIRGE_4MB_2 = 0b001,
	S3_VIRGE_4MB_3 = 0b010,
	S3_VIRGE_8MB = 0b011,
	S3_VIRGE_2MB = 0b100,
};


void s3virge_init();