ENTRY(_start)

SECTIONS
{
	.multiboot : ALIGN(4k)
	{
		*(.multiboot)
	}
	. = 0x100000;
	kernel_start = .;
	.boot :
	{
		*(.boot)
		. = ALIGN(4096);
		pml4 = .;
		. += 0x1000;
		pdpt = .;
		. += 0x1000;
		pd = .;
		. += 0x1000;
		pdlower = .;
		. += 0x1000;
		pdptphysical_map = .;
		. += 0x1000;
		. += 0x8000;
	}

	.text : ALIGN(0x1000)
	{
		*(.text)
		*(.gnu.linkonce.t*)
		*(.bootstrap_stack)
		*(.init)
		*(.fini)
		*(.jcr)
		*(.dtors)
		*(.ctors)
		*vdso_helper.o(*.vdso)
	}

	.data : ALIGN(0x1000)
	{
		*(.data)
		*(.gnu.linkonce.d*)
	}

	.rodata : ALIGN(0x1000)
	{
		*(.rodata*)
		*(.gnu.linkonce.r*)
		_driver_init_start = .;
		*(.driver.init*)
		_driver_init_end = .;
	}

	.bss : ALIGN(0x1000)
	{
		*(COMMON)
		*(.bss)
		*(.gnu.linkonce.b*)
	}
	kernel_end = .;

}
