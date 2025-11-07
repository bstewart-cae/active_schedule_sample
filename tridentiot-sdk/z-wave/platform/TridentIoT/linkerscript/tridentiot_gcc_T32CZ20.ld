/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

MEMORY
{
    /* Define each memory region. */
    FLASH (rx)  : ORIGIN = 0x10008000, LENGTH = 0x91000  /* 580 KB */
    RAM (rwx)   : ORIGIN = 0x30000000, LENGTH = 0x30000
    MFG_TOKENS_PRODUCTION (rx) : ORIGIN = 0x100FF000, LENGTH = 0x1000  /* 4KB for chip production data */
    MFG_TOKENS_ZW (rx)         : ORIGIN = 0x100FE000, LENGTH = 0x800  /* 2KB for Z-Wave stack tokens */
    MFG_TOKENS_APP (rx)        : ORIGIN = 0x100FE800, LENGTH = 0x800  /* 2KB for application tokens */
}

__btl_start_address = ORIGIN(RAM);
__flash_max_size__ = LENGTH(FLASH) - NVM_STORAGE_SIZE;
__fw_max_size__ = __flash_max_size__ - 0x200;  /*we need to reserve 0x200 bytes for the image signature*/

/* Entry point. */
ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector Table Section. */
    .text :
    {
        KEEP(*(.vectors))
        __Vectors_End = .;
        __Vectors_Size = __Vectors_End - __Vectors;
        __end__ = .;

        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))


        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
         *crtbegin.o(.dtors)
         *crtbegin?.o(.dtors)
         *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
         *(SORT(.dtors.*))
         *(.dtors)

        *(.rodata*)

        KEEP(*(.eh_frame*))
    } > FLASH

    .fw_properties :
    {
      . = ALIGN(4);
      KEEP(*(.fw_properties*))
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    /* To copy multiple ROM to RAM sections,
     * define etext2/data2_start/data2_end and
     * define __STARTUP_COPY_MULTIPLE in startup_cmsdk_mps2_an519_bl2.S */
    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__etext)
        LONG (__data_start__)
        LONG ((__data_end__ - __data_start__) / 4)
        LONG (DEFINED(__etext2) ? __etext2 : 0)
        LONG (DEFINED(__data2_start__) ? __data2_start__ : 0)
        LONG (DEFINED(__data2_start__) ? ((__data2_end__ - __data2_start__) / 4) : 0)
        __copy_table_end__ = .;
    } > FLASH

    /* To clear multiple BSS sections,
     * uncomment .zero.table section and,
     * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_cmsdk_mps2_an519_bl2.S */
    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (__bss_start__)
        LONG ((__bss_end__ - __bss_start__) / 4)
        LONG (DEFINED(__bss2_start__) ? __bss2_start__ : 0)
        LONG (DEFINED(__bss2_start__) ? ((__bss2_end__ - __bss2_start__) / 4) : 0)
        __zero_table_end__ = .;
    } > FLASH

    __etext = ALIGN(4);

    .btl_mailbox __btl_start_address (NOLOAD):
    {
        __btl_mailbox_start__ = .;
        *(.btl_mailbox*)
        __btl_mailbox_end__ = .;

    } > RAM


    .data : AT (__etext)
    {
        __data_start__ = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > RAM
    Image$$ER_DATA$$Base = ADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    bss_size = __bss_end__ - __bss_start__;

    /* Reserve space and place heap in memory map. */


    .heap (ORIGIN(RAM) + LENGTH(RAM) - __HEAP_SIZE -__STACK_SIZE - __RET_SRAM_SIZE) :  ALIGN(8)
    {
        __end__ = .;
        PROVIDE(end = .);
        __HeapBase = .;
        . += __HEAP_SIZE;
        __HeapLimit = .;
        __heap_limit = .; /* Add for _sbrk */
    } > RAM

    Image$$ARM_LIB_HEAP$$ZI$$Limit = ADDR(.heap) + SIZEOF(.heap);

    /* Reserve space for stack in memory. */
    .stack :
    {
        __StackLimit = .;
        . +=  __STACK_SIZE - 0x8;
        __StackTop = .;
    } > RAM

    Image$$ARM_LIB_STACK$$ZI$$Base = ADDR(.stack);
    Image$$ARM_LIB_STACK$$ZI$$Limit = ADDR(.stack)+SIZEOF(.stack);

    .stack_seal_res :
    {
        . += 0x8;
    } > RAM

    __StackSeal = ADDR(.stack_seal_res);


    PROVIDE(__stack = Image$$ARM_LIB_STACK$$ZI$$Limit);
    /* reserve space for retention sram*/
    .ret_sram (NOLOAD) :
    {
        . = ALIGN(4);
        __ret_sram_start__ = .;
        *(.ret_sram*)
        . = ALIGN(4);
        __ret_sram_end__ = .;
    } > RAM

    /* Section in retention RAM for reset counter. */
    .ret_counter (NOLOAD) :
    {
        *(.ret_counter*)
    } > RAM

    /* Provide basic symbols giving location and size of main text block,
     * including initial values of RW data sections. Note that these will need
     * extending to give a complete picture with complex images
     * (e.g multiple Flash banks). */
    _image_start    = LOADADDR(.text);
    _image_end      = LOADADDR(.data) + SIZEOF(.data);
    _image_size     = _image_end - _image_start;

    __app_end__ = ORIGIN(FLASH) + SIZEOF(.data) + SIZEOF(.text);

    __nvm_storage_start__ = ORIGIN(FLASH) + __flash_max_size__;

    .nvm_storage __nvm_storage_start__ (NOLOAD) : {
      KEEP(*(.nvm_storage*))
    } > FLASH

    __nvmStorageBase = __nvm_storage_start__;

    /*this definition is used in the C code*/
    __mfg_tokens_region_start = ORIGIN(MFG_TOKENS_ZW);
    .mfg_tokens_storage (NOLOAD) : {
      KEEP(*(.mfg_tokens_storage*))
    } > MFG_TOKENS_ZW

    __mfg_tokens_app_region_start = ORIGIN(MFG_TOKENS_APP);
    .mfg_tokens_app_storage (NOLOAD) : {
      KEEP(*(.mfg_tokens_app_storage*))
    } > MFG_TOKENS_APP

    __mfg_tokens_production_region_start = ORIGIN(MFG_TOKENS_PRODUCTION);
    .mfg_tokens_app_storage (NOLOAD) : {
      KEEP(*(.mfg_tokens_production_storage*))
    } > MFG_TOKENS_PRODUCTION

    ASSERT(  _image_size  <= __fw_max_size__, "fw too big !")
    ASSERT( __nvm_storage_start__  >= __app_end__, "nvm storage too big !")
}
