/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2024 NXP
 * Generated linker script file for MCXN947
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.9.0_prc2 [Build 2107] [2023-11-24] on Apr 11, 2024, 3:29:29 PM
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH0 (rx) : ORIGIN = 0x0, LENGTH = 0x100000 /* 1M bytes (alias Flash) */  
  PROGRAM_FLASH1 (rx) : ORIGIN = 0x100000, LENGTH = 0x100000 /* 1M bytes (alias Flash2) */  
  SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x66000 /* 408K bytes (alias RAM) */  
  SRAMX (rwx) : ORIGIN = 0x4000000, LENGTH = 0x18000 /* 96K bytes (alias RAM2) */  
  SRAMH (rwx) : ORIGIN = 0x20066000, LENGTH = 0x1000 /* 4K bytes (alias RAM3) */  
  USB_RAM (rwx) : ORIGIN = 0x400ba000, LENGTH = 0x1000 /* 4K bytes (alias RAM4) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH0 = 0x0  ; /* PROGRAM_FLASH0 */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_PROGRAM_FLASH0 = 0x0 + 0x100000 ; /* 1M bytes */  
  __top_Flash = 0x0 + 0x100000 ; /* 1M bytes */  
  __base_PROGRAM_FLASH1 = 0x100000  ; /* PROGRAM_FLASH1 */  
  __base_Flash2 = 0x100000 ; /* Flash2 */  
  __top_PROGRAM_FLASH1 = 0x100000 + 0x100000 ; /* 1M bytes */  
  __top_Flash2 = 0x100000 + 0x100000 ; /* 1M bytes */  
  __base_SRAM = 0x20000000  ; /* SRAM */  
  __base_RAM = 0x20000000 ; /* RAM */  
  __top_SRAM = 0x20000000 + 0x66000 ; /* 408K bytes */  
  __top_RAM = 0x20000000 + 0x66000 ; /* 408K bytes */  
  __base_SRAMX = 0x4000000  ; /* SRAMX */  
  __base_RAM2 = 0x4000000 ; /* RAM2 */  
  __top_SRAMX = 0x4000000 + 0x18000 ; /* 96K bytes */  
  __top_RAM2 = 0x4000000 + 0x18000 ; /* 96K bytes */  
  __base_SRAMH = 0x20066000  ; /* SRAMH */  
  __base_RAM3 = 0x20066000 ; /* RAM3 */  
  __top_SRAMH = 0x20066000 + 0x1000 ; /* 4K bytes */  
  __top_RAM3 = 0x20066000 + 0x1000 ; /* 4K bytes */  
  __base_USB_RAM = 0x400ba000  ; /* USB_RAM */  
  __base_RAM4 = 0x400ba000 ; /* RAM4 */  
  __top_USB_RAM = 0x400ba000 + 0x1000 ; /* 4K bytes */  
  __top_RAM4 = 0x400ba000 + 0x1000 ; /* 4K bytes */  
