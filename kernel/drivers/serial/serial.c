/*
* Copyright (c) 2018 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <carbon/x86/portio.h>
#include <carbon/memory.h>
#include <carbon/console.h>

struct serial_port
{
	uint16_t io_port;
	int com_nr;
};

bool serial_is_transmit_empty(struct serial_port *port)
{
	return inb(port->io_port + 5) & 0x20;
}

bool serial_recieved(struct serial_port *port)
{
	return inb(port->io_port + 5) & 1;
}

static void write_byte(char c, struct serial_port *port)
{
	while(!serial_is_transmit_empty(port));

	outb(port->io_port, c);
	if(c == '\n')
		write_byte('\r', port);
}

static void serial_write(const char *s, size_t size, struct serial_port *port)
{
	for(size_t i = 0; i < size; i++)
	{
		write_byte(*(s + i), port);
	}
}

static char read_byte(struct serial_port *port)
{
	while(!serial_recieved(port));

	return inb(port->io_port);
}

void serial_port_init(struct serial_port *port)
{
	outb(port->io_port + 1, 0x00);    // Disable all interrupts
	outb(port->io_port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(port->io_port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port->io_port + 1, 0x00);    //                  (hi byte)
	outb(port->io_port + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(port->io_port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port->io_port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

struct serial_port *serial_port_create(int comx, uint16_t io_port)
{
	return NULL;
}

struct serial_port *com1 = NULL;

ssize_t serial_console_write(const void *buffer, size_t len, struct console *c)
{
	struct serial_port *p = c->priv;
	serial_write(buffer, len, p);

	return len;
}
ssize_t serial_console_read(void *buffer, size_t len, struct console *c)
{
	//struct serial_port *p = c->priv;
	return -1;
}

struct console serial_console = 
{
	.name = "com1",
	.write = serial_console_write,
	.read = serial_console_read,
};

void x86_serial_init(void)
{
	com1 = ksbrk(sizeof(struct serial_port));
	
	com1->io_port = 0x3f8;
	com1->com_nr = 0;
	serial_port_init(com1);
	serial_console.priv = com1;

	console_add(&serial_console);
}
