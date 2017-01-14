/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Linker script for ST STM32F429IDISCOVERY (STM32F429ZI, 2048K flash, 256K RAM). */

/* Define memory regions. */
MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
	ram (rwx) : ORIGIN = 0x10000000, LENGTH = 64K /* (kbob) CCM is faster */
	system_ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
        /* SDRAM is 8 MB, has four banks of 2 MB. */
        sdram_0 (rwx) : ORIGIN = 0xD0000000, LENGTH = 2M
        sdram_1 (rwx) : ORIGIN = 0xD0200000, LENGTH = 2M
        sdram_2 (rwx) : ORIGIN = 0xD0400000, LENGTH = 2M
        sdram_3 (rwx) : ORIGIN = 0xD0600000, LENGTH = 2M
}

SECTIONS
{
	.system_ram (NOLOAD) : {
		*(.system_ram)
	} >system_ram
	.sdram_0 (NOLOAD) : {
		*(.sdram_bank_0)
	} >sdram_0
	.sdram_1 (NOLOAD) : {
		*(.sdram_bank_1)
	} >sdram_1
	.sdram_2 (NOLOAD) : {
		*(.sdram_bank_2)
	} >sdram_2
	.sdram_3 (NOLOAD) : {
		*(.sdram_bank_3)
	} >sdram_3
}
/* Include the common ld script. */
INCLUDE libopencm3_stm32f4.ld
