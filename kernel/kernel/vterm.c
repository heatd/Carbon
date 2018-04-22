/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <carbon/vterm.h>
#include <carbon/framebuffer.h>
#include <carbon/font.h>
#include <carbon/console.h>

struct vterm
{
	unsigned int columns;
	unsigned int rows;
	unsigned int cursor_x, cursor_y;
	struct framebuffer *fb;
};

struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static inline uint32_t unpack_rgba(struct color color, struct framebuffer *fb)
{
	uint32_t c = 	((color.r << fb->color.red_shift) & fb->color.red_mask) |
			((color.g << fb->color.green_shift) & fb->color.green_mask) |
			((color.b << fb->color.blue_shift) & fb->color.blue_mask);

	return c;
}

static struct color fg = {.r = 0xaa, .g = 0xaa, .b = 0xaa};
static struct color bg = {0};

static void draw_char(char c, unsigned int x, unsigned int y, struct framebuffer *fb)
{
	struct font *font = get_font_data();
	volatile char *buffer = (volatile char *) fb->framebuffer;

	buffer += y * fb->pitch + x * (fb->bpp / 8);

	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			struct color color;
			unsigned char f = font->font_bitmap[c * font->height + i];

			if(f & font->mask[j])
				color = fg;
			else
				color = bg;
			
			uint32_t c = unpack_rgba(color, fb);
			volatile uint32_t *b = (volatile uint32_t *) ((uint32_t *) buffer + j);
			
			/* If the bpp is 32 bits, we can just blit it out */
			if(fb->bpp == 32)
				*b = c;
			else
			{
				volatile unsigned char *buf =
					(volatile unsigned char *)(buffer + j);
				int bytes = fb->bpp / 8;
				for(int i = 0; i < bytes; i++)
				{
					buf[i] = c;
					c >>= 8;
				}
			}
		}

		buffer = (void*) (((char *) buffer) + fb->pitch);
	}
}

void vterm_scroll(struct framebuffer *fb, struct vterm *vt)
{
	struct font *font = get_font_data();

	unsigned int font_height = font->height;
	unsigned int row_stride = font_height * fb->pitch;

	memmove(fb->framebuffer, fb->framebuffer + row_stride,
		fb->framebuffer_size - row_stride);
	memset(fb->framebuffer + (vt->rows-1) * row_stride, 0,
		row_stride);
}

void vterm_putc(char c, struct vterm *vt)
{
	struct framebuffer *fb = vt->fb;
	struct font *f = get_font_data();

	if(c == '\n')
	{
		vt->cursor_x = 0;
		vt->cursor_y++;
	}
	else
	{
		draw_char(c, vt->cursor_x * f->width, vt->cursor_y * f->height, fb);
		vt->cursor_x++;
	}

	if(vt->cursor_x > vt->columns)
	{
		/* Forced newline */
		vt->cursor_x = 0;
		vt->cursor_y++;
	}

	if(vt->cursor_y == vt->rows)
	{
		vterm_scroll(fb, vt);
		vt->cursor_y--;
	}
}

void draw_cursor(int x, int y, struct framebuffer *fb)
{
	struct font *font = get_font_data();
	volatile char *buffer = (volatile char *) fb->framebuffer;

	buffer += y * fb->pitch + x * (fb->bpp / 8);

	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			struct color color;
			unsigned char f = font->cursor_bitmap[i];

			if(f & font->mask[j])
				color = bg;
			else
				color = fg;
			
			uint32_t c = unpack_rgba(color, fb);
			volatile uint32_t *b = (volatile uint32_t *) ((uint32_t *) buffer + j);
			
			/* If the bpp is 32 bits, we can just blit it out */
			if(fb->bpp == 32)
				*b = c;
			else
			{
				volatile unsigned char *buf =
					(volatile unsigned char *)(buffer + j);
				int bytes = fb->bpp / 8;
				for(int i = 0; i < bytes; i++)
				{
					buf[i] = c;
					c >>= 8;
				}
			}
		}

		buffer = (void*) (((char *) buffer) + fb->pitch);
	}
}

void update_cursor(struct vterm *vt)
{
	struct framebuffer *fb = vt->fb;
	struct font *f = get_font_data();

	draw_cursor(vt->cursor_x * f->width, vt->cursor_y * f->height, fb);
}

ssize_t vterm_write(const void *buffer, size_t len, struct console *c)
{
	ssize_t written = 0;
	const char *str = buffer;
	for(; *str; str++, written++)
	{
		vterm_putc(*str, c->priv);
	}
	
	update_cursor(c->priv);
	return written;
}

struct vterm primary_vterm = {0};
struct console primary_console =
{
	.name = "vterm",
	.write = vterm_write,
	.priv = &primary_vterm
};

void vterm_initialize(void)
{
	struct framebuffer *fb = get_primary_framebuffer();
	struct font *font = get_font_data();
	primary_vterm.columns = fb->width / font->width;
	primary_vterm.rows = fb->height / font->height;
	primary_vterm.fb = fb;

	console_reset(&primary_console);
	
	update_cursor(&primary_vterm);
}
