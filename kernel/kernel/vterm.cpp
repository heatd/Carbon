/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <carbon/vterm.h>
#include <carbon/framebuffer.h>
#include <carbon/font.h>
#include <carbon/console.h>
#include <carbon/memory.h>

struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct console_cell
{
	uint32_t codepoint;
	struct color bg;
	struct color fg;
	unsigned long dirty;	
};

struct vterm
{
	unsigned int columns;
	unsigned int rows;
	unsigned int cursor_x, cursor_y;
	struct framebuffer *fb;
	struct console_cell *cells;
	struct color fg;
	struct color bg;
};

static inline uint32_t unpack_rgba(struct color color, struct framebuffer *fb)
{
	uint32_t c = 	((color.r << fb->color.red_shift) & fb->color.red_mask) |
			((color.g << fb->color.green_shift) & fb->color.green_mask) |
			((color.b << fb->color.blue_shift) & fb->color.blue_mask);

	return c;
}

static struct color default_fg = {.r = 0xaa, .g = 0xaa, .b = 0xaa, .a = 0x00};
static struct color default_bg = {};

static void draw_char(uint32_t c, unsigned int x, unsigned int y,
	struct framebuffer *fb, struct color fg, struct color bg)
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

		buffer = (volatile char*) (((char *) buffer) + fb->pitch);
	}
}

void vterm_flush_all(struct vterm *vterm)
{
	struct font *f = get_font_data();
	for(unsigned int i = 0; i < vterm->columns; i++)
	{
		for(unsigned int j = 0; j < vterm->rows; j++)
		{
			struct console_cell *cell = &vterm->cells[j * vterm->columns + i];
			draw_char(cell->codepoint, i * f->width, j * f->height,
				vterm->fb, cell->fg, cell->bg);
			cell->dirty = 0;
		}
	}
}

void vterm_scroll(struct framebuffer *fb, struct vterm *vt)
{
	memcpy(vt->cells, vt->cells + vt->columns, sizeof(struct console_cell)
		* (vt->rows-1) * vt->columns);
	
	for(unsigned int i = 0; i < vt->columns; i++)
	{
		struct console_cell *c = &vt->cells[(vt->rows-1) * vt->columns + i];
		c->codepoint = ' ';
		c->bg = default_bg;
		c->fg = default_fg;
	}
	
	vterm_flush_all(vt);
}

void vterm_set_char(char c, unsigned int x, unsigned int y, struct color fg,
	struct color bg, struct vterm *vterm)
{
	struct console_cell *cell = &vterm->cells[y * vterm->columns + x];
	cell->codepoint = c;
	cell->fg = fg;
	cell->bg = bg;
	cell->dirty = 1;
}

void vterm_putc(char c, struct vterm *vt)
{
	struct framebuffer *fb = vt->fb;

	if(c == '\n')
	{
		vt->cursor_x = 0;
		vt->cursor_y++;
	}
	else
	{
		vterm_set_char(c, vt->cursor_x, vt->cursor_y, vt->fg, vt->bg, vt);
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
				color = default_bg;
			else
				color = default_fg;
			
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

		buffer = (volatile char *) (((char *) buffer) + fb->pitch);
	}
}

void update_cursor(struct vterm *vt)
{
	struct framebuffer *fb = vt->fb;
	struct font *f = get_font_data();

	draw_cursor(vt->cursor_x * f->width, vt->cursor_y * f->height, fb);
}

void vterm_flush(struct vterm *vterm);

ssize_t vterm_write(const void *buffer, size_t len, struct console *c)
{
	ssize_t written = 0;
	const char *str = (const char *) buffer;
	for(size_t i = 0; i != len; str++, written++, len--)
	{
		vterm_putc(*str, (struct vterm *) c->priv);
	}
	
	vterm_flush((struct vterm *) c->priv);
	update_cursor((struct vterm *) c->priv);
	return written;
}

void vterm_flush(struct vterm *vterm)
{
	struct font *f = get_font_data();
	for(unsigned int i = 0; i < vterm->columns; i++)
	{
		for(unsigned int j = 0; j < vterm->rows; j++)
		{
			struct console_cell *cell = &vterm->cells[j * vterm->columns + i];

			if(cell->dirty)
			{
				draw_char(cell->codepoint, i * f->width, j * f->height,
					vterm->fb, cell->fg, cell->bg);
				cell->dirty = 0;
			}
		}
	}
}

void vterm_fill_screen(struct vterm *vterm, uint32_t character,
	struct color fg, struct color bg)
{
	for(unsigned int i = 0; i < vterm->columns; i++)
	{
		for(unsigned int j = 0; j < vterm->rows; j++)
		{
			struct console_cell *cell = &vterm->cells[j * vterm->rows + i];
			cell->codepoint = character;
			cell->bg = bg;
			cell->fg = fg;
			cell->dirty = 1;
		}
	}

	vterm_flush(vterm);
}

struct vterm primary_vterm = {};
struct console primary_console =
{
	(char *) "vterm",
	vterm_write,
	nullptr,
	&primary_vterm,
	nullptr
};

static void fb_fill_color(uint32_t color, struct framebuffer *frameb)
{
	volatile uint32_t *fb = (volatile uint32_t *) frameb->framebuffer;

	for(size_t i = 0; i < frameb->height; i++)
	{
		for(size_t j = 0; j < frameb->width; j++)
			fb[j] = color;
		fb = (volatile uint32_t *) ((char*) fb + frameb->pitch);
	}
}
void *efi_allocate_early_boot_mem(size_t size);

void vterm_initialize(void)
{
	struct framebuffer *fb = get_primary_framebuffer();
	struct font *font = get_font_data();
	primary_vterm.columns = fb->width / font->width;
	primary_vterm.rows = fb->height / font->height;
	primary_vterm.fb = fb;
	primary_vterm.cells = (struct console_cell *)
		efi_allocate_early_boot_mem(
		primary_vterm.columns * primary_vterm.rows * sizeof(*primary_vterm.cells));

	primary_vterm.fg = default_fg;
	primary_vterm.bg = default_bg;

	assert(primary_vterm.cells != NULL);

	console_reset(&primary_console);

	vterm_fill_screen(&primary_vterm, ' ', primary_vterm.fg, primary_vterm.bg);

	update_cursor(&primary_vterm);
}
