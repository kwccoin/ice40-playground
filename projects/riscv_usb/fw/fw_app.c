/*
 * fw_app.c
 *
 * Copyright (C) 2019 Sylvain Munaut
 * All rights reserved.
 *
 * LGPL v3+, see LICENSE.lgpl3
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "console.h"
#include "icepick.h"
#include "led.h"
#include "mini-printf.h"
#include "spi.h"
#include "usb.h"
#include "usb_dfu_rt.h"
#include "utils.h"


extern const struct usb_stack_descriptors app_stack_desc;



static void
serial_no_init()
{
	uint8_t buf[8];
	char *id, *desc;
	int i;

	flash_manuf_id(buf);
	printf("Flash Manufacturer : %s\n", hexstr(buf, 3, true));

	flash_unique_id(buf);
	printf("Flash Unique ID    : %s\n", hexstr(buf, 8, true));

	/* Overwrite descriptor string */
		/* In theory in rodata ... but nothing is ro here */
	id = hexstr(buf, 8, false);
	desc = (char*)app_stack_desc.str[1];
	for (i=0; i<16; i++)
		desc[2 + (i << 1)] = id[i];
}

static void
boot_dfu(void)
{
	/* Force re-enumeration */
	usb_disconnect();

	/* Boot firmware */
	volatile uint32_t *boot = (void*)0x80000000;
	*boot = (1 << 2) | (1 << 0);
}

void
usb_dfu_rt_cb_reboot(void)
{
        boot_dfu();
}

void main()
{
	int cmd = 0;

	/* Init console IO */
	console_init();
	puts("Booting App image testing ..\n");

	/* LED */
	led_init();
	led_color(56, 5, 72);
	led_blink(true, 250, 1250);
	led_breathe(true, 1000, 750);
	led_state(true);

	/* SPI */
	spi_init();

	/* Enable USB directly */
	serial_no_init();
	usb_init(&app_stack_desc);
	usb_dfu_rt_init();
	usb_cdc_acm_init();
	icepick_init();
	usb_connect();

	/* IO */
	icepick_set_vio(0xc00);

	/* Main loop */
	while (1)
	{
		/* Prompt ? */
		if (cmd >= 0)
			printf("Command> ");

		/* Poll for command */
		cmd = getchar_nowait();

		if (cmd >= 0) {
			if (cmd > 32 && cmd < 127) {
				putchar(cmd);
			}
			putchar('\r');
			putchar('\n');

			switch (cmd)
			{
			case 'p':
				usb_debug_print();
				break;
			case 'b':
				boot_dfu();
				break;
			case 'c':
				icepick_test_clk();
				break;
			case 'i':
				icepick_test_io();
				break;
			case 's':
				icepick_test_sense();
				break;
			default:
				break;
			}
		}

		/* USB poll */
		usb_poll();
	}
}
