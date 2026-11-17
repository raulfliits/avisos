OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY {
  rom   : ORIGIN = 0x08000000, LENGTH = 32M
  iwram : ORIGIN = 0x03000000, LENGTH = 0x7E00
  ewram : ORIGIN = 0x02000000, LENGTH = 256K
}
SECTIONS {
  .text : { KEEP(*(.crt0)) *(.text .text.* .glue_7 .glue_7t .gnu.linkonce.t.*) KEEP(*(.init)) KEEP(*(.fini)) } >rom
  .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) . = ALIGN(4); } >rom
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >rom
  .ARM.exidx : { __exidx_start = .; *(.ARM.exidx* .gnu.linkonce.armexidx.*) __exidx_end = .; } >rom
  .init_array : { KEEP(*(.init_array*)) } >rom
  .fini_array : { KEEP(*(.fini_array*)) } >rom
  . = ALIGN(4);
  __iwram_lma = .;
  .iwram : AT(__iwram_lma) { __iwram_start = .; *(.iwram .iwram.*) *(.data .data.* .gnu.linkonce.d.*) . = ALIGN(4); __iwram_end = .; } >iwram
  .bss (NOLOAD) : { __bss_start = .; *(.bss .bss.* COMMON .gnu.linkonce.b.*) . = ALIGN(4); __bss_end = .; end = .; _end = .; } >iwram
  .ewram (NOLOAD) : { __ewram_bss_start = .; *(.ewram .ewram.*) . = ALIGN(4); __ewram_bss_end = .; } >ewram
  /DISCARD/ : { *(.comment) }
}
