ENTRY(_start)

SECTIONS
{
	.multiboot : ALIGN(4k)
	{
		*(.multiboot)
	}
	. = 0x100000;
	kernel_start = .;
	_text_start = .;
	.boot :
	{
		*(.boot)
		*(.boot_data)
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

	cpu_phys_base = .;
	. = 0;

	.percpu : AT(cpu_phys_base)
	{
		__percpu_start = .;
		*(.percpu*)
		__percpu_end = .;
	}
	
	. += cpu_phys_base;

	.text : ALIGN(0x1000)
	{
		*(.text*)
		*(.gnu.linkonce.t*)
		*(.bootstrap_stack)
		*(.init)
		*(.fini)
		*(.jcr)
		*(.dtors)
		*(.ctors)
		*vdso_helper.o(*.vdso)
	}

	.rodata : ALIGN(0x1000)
	{
		*(.rodata*)
		*(.gnu.linkonce.r*)
		_driver_init_start = .;
		*(.driver.init*)
		_driver_init_end = .;
	}

	. = ALIGN(0x1000);

	_text_end = .;

	_data_start = .;
	.data : ALIGN(0x1000)
	{
		*(.data)
		*(.gnu.linkonce.d*)
	}

	.bss : ALIGN(0x1000)
	{
		*(COMMON)
		*(.bss)
		*(.gnu.linkonce.b*)
		. = ALIGN(16);
		*(.bootstrap_stack*)
	}

	. = ALIGN(0x1000);
	_data_end = .;
	_vdso_sect_start = .;
	.vdso ALIGN(0x1000) : AT(ADDR(.vdso))
	{
		*vdso_helper.o(*.vdso)
	}

	. = ALIGN(0x1000);
	_vdso_sect_end = .;
	kernel_end = .;

}
