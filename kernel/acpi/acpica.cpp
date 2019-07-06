/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
/* File: acpica.cpp. It's here as the OS layer for ACPICA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <carbon/panic.h>
#include <acpica/acpi.h>
#include <carbon/lock.h>
#include <carbon/x86/portio.h>
#include <carbon/vm.h>
#include <carbon/acpi.h>
#include <carbon/memory.h>
#include <carbon/irq.h>
#include <carbon/list.h>

extern "C"
ACPI_STATUS AcpiOsInitialize()
{
	printf("ACPI initializing!\n");
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsShutdown()
{
	return AE_OK;
}

extern "C"
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
	return (ACPI_PHYSICAL_ADDRESS) Acpi::GetRsdp();
}

extern "C"
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue)
{
	*NewValue = NULL;
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
	*NewTable = NULL;
	return AE_OK;
}

extern "C"
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
	void *addrl = phys_to_virt(PhysicalAddress);
	return addrl;
}

extern "C"
void AcpiOsUnmapMemory(void *where, ACPI_SIZE Length)
{
	(void) where;
	(void) Length;
}

extern "C"
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	panic("implement");
	//*PhysicalAddress = (ACPI_PHYSICAL_ADDRESS) virtual2phys(LogicalAddress);
	return AE_OK;
}

extern "C"
void *AcpiOsAllocate(ACPI_SIZE Size)
{	
	void *ptr = malloc(Size);
	if(!ptr)
		printf("Allocation failed with size %lu\n", Size);
	return ptr;
}

extern "C"
void AcpiOsFree(void *Memory)
{
	free(Memory);
}

// On the OSDev wiki it says it's never used, so I don't need to implement this
// right now(all memory should be readable anyway)
extern "C"
BOOLEAN AcpiOsReadable(void * Memory, ACPI_SIZE Length)
{
	return 1;
}
// On the OSDev wiki it says it's never used, so I don't need to implement this
// right now(all memory should be writable anyway)
extern "C"
BOOLEAN AcpiOsWritable(void * Memory, ACPI_SIZE Length)
{
	return 1;
}

extern "C"
ACPI_THREAD_ID AcpiOsGetThreadId()
{
	return 1;
}

extern "C"
ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void * Context)
{
	panic("unimpl");
	return AE_OK;
}

extern "C"
void AcpiOsSleep(UINT64 Milliseconds)
{
	panic("unimpl");
}
void AcpiOsStall(UINT32 Microseconds)
{
	panic("unimpl");
}

extern "C"
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
{
	*OutHandle = (struct spinlock *) AcpiOsAllocateZeroed(sizeof(struct spinlock));
	if(*OutHandle == nullptr)	return AE_NO_MEMORY;
	return AE_OK;
}

extern "C"
void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
	free(Handle);
}
// TODO: Implement Timeout
extern "C"
ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
	spin_lock(Handle);
	return AE_OK;
}

extern "C"
void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
	spin_unlock(Handle);
}

// TODO: Implement Semaphores (should be pretty simple)
extern "C"
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE * OutHandle)
{
	*OutHandle = (struct spinlock *) AcpiOsAllocateZeroed(sizeof(struct spinlock));
	if(*OutHandle == NULL) return AE_NO_MEMORY;
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	free(Handle);
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
	panic("todo");
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	panic("todo");
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
	*OutHandle = (struct spinlock *) AcpiOsAllocateZeroed(sizeof(struct spinlock));
	if(*OutHandle == NULL) return AE_NO_MEMORY;
	return AE_OK;
}

extern "C"
void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	free(Handle);
}

extern "C"
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	spin_lock(Handle);
	return 0;
}

extern "C"
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
	spin_unlock(Handle);
}

/*
ACPI_OSD_HANDLER ServiceRout;

irqstatus_t acpi_sci_irq(struct irq_context *ctx, void *cookie)
{
	ServiceRout(cookie);
	return IRQ_HANDLED;
}

struct driver acpi_driver =
{
	.name = "acpi"
};

static struct device dev =
{
	.name = "acpi_sci",
	.driver = &acpi_driver
};
*/


Driver AcpiDriver("Acpi Driver");

Device SciDevice("SCI", &AcpiDriver);

struct AcpiIrqContext
{
	ACPI_OSD_HANDLER Handler;
	void *Context;
};

bool AcpiSciGpeIrq(struct Irq::IrqContext& context)
{
	AcpiIrqContext *con = (AcpiIrqContext *) context.context;
	
	auto result = con->Handler(con->Context);

	return (result == ACPI_INTERRUPT_HANDLED);
}

static LinkedList<Irq::IrqHandler *> IrqHandlerList;

extern "C"
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{
	AcpiIrqContext *IrqContext = new AcpiIrqContext;
	if(!IrqContext)
		return AE_NO_MEMORY;

	IrqContext->Context = Context;
	IrqContext->Handler = Handler;
	Irq::IrqHandler *handler = new Irq::IrqHandler(&SciDevice, &AcpiDriver,
					AcpiSciGpeIrq, InterruptLevel, IrqContext);
	if(!handler)
	{
		delete IrqContext;
		return AE_NO_MEMORY;
	}

	if(!IrqHandlerList.Add(handler))
	{
		delete handler;
		delete IrqContext;
		return AE_NO_MEMORY;
	}

	if(!handler->Install())
		return AE_IO_ERROR;

	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
	for(auto it = IrqHandlerList.begin(); it != IrqHandlerList.end(); ++it)
	{
		auto handler = *it;
		auto context = (AcpiIrqContext *) handler->GetContext();
		if(context->Handler == Handler &&
		   handler->GetLine() == InterruptNumber)
		{
			assert(IrqHandlerList.Remove(handler, it) == true);

			handler->Free();

			delete context;
			delete handler;

			return AE_OK;
		}
	}

	return AE_NOT_EXIST;
}

extern "C"
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
	void *ptr;
	ptr = AcpiOsMapMemory(Address, 4096);
	*Value = *(UINT64*) ptr;
	if(Width == 8)
		*Value &= 0xFF;
	else if(Width == 16)
		*Value &= 0xFFFF;
	else if(Width == 32)
		*Value &= 0xFFFFFFFF;
	AcpiOsUnmapMemory(ptr, 4096);
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	UINT64 *ptr;
	ptr = (UINT64*) AcpiOsMapMemory(Address, 4096);
	if(Width == 8)
		*ptr = Value & 0xFF;
	else if(Width == 16)
		*ptr = Value & 0xFFFF;
	else if(Width == 32)
		*ptr = Value & 0xFFFFFFFF;
	else
		*ptr = Value;
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsReadPort (ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
	if(Width == 8)
		*Value = inb(Address);
	else if(Width == 16)
		*Value = inw(Address);
	else if(Width == 32)
		*Value = inl(Address);
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsWritePort (ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	if(Width == 8)
		outb(Address, (uint8_t) Value);
	else if(Width == 16)
		outw(Address, (uint16_t) Value);
	else if(Width == 32)
		outl(Address, Value);
	return AE_OK;
}

struct pci_device
{
	uint8_t bus, slot, func;
};

static Spinlock pci_lock;
const uint16_t CONFIG_ADDRESS = 0xCF8;
const uint16_t CONFIG_DATA = 0xCFC;

uint32_t __pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint32_t tmp = 0;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
                     (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	ScopedSpinlock l(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	tmp = inl(CONFIG_DATA);
	return tmp;
}

__attribute__((no_sanitize_undefined))
uint16_t __pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
        union { uint8_t bytes[4]; uint32_t val;} data;
	data.val = __pci_config_read_dword(bus, slot, func, offset);
	return data.bytes[(offset & 0x3)] | (data.bytes[(offset & 3) + 1] << 8);
}

void __pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 0x80000000));
	
	ScopedSpinlock l(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	outl(CONFIG_DATA, data);
}

void __pci_write_word_aligned(uint8_t bus, uint8_t slot, uint8_t func,
			      uint8_t offset, uint16_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 0x80000000));
	
	ScopedSpinlock l(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	outw(CONFIG_DATA, data);
}

void __pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data)
{
	uint8_t aligned_offset = offset & -4;

	if(aligned_offset == offset)
	{
		/* NOTE: Check pcie.c's __pcie_config_write_word
		 * commentary on why this is needed
		*/
		__pci_write_word_aligned(bus, slot, func, offset, data);
		return;
	}

	uint8_t bshift = offset - aligned_offset;
	uint32_t byte_mask = 0xffff << (bshift * 8);
	uint32_t dword = __pci_config_read_dword(bus, slot, func, aligned_offset);
	dword = (dword & ~byte_mask) | (data << (bshift * 8));
	__pci_write_dword(bus, slot, func, aligned_offset, dword);
}

void __pci_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data)
{
	uint8_t aligned_offset = offset & -4;
	uint8_t byte_shift = offset - aligned_offset;
	uint32_t byte_mask = 0xff << (byte_shift * 8);
	uint32_t dword = __pci_config_read_dword(bus, slot, func, aligned_offset);
	dword = (dword & ~byte_mask) | (data << (byte_shift * 8));
	__pci_write_dword(bus, slot, func, aligned_offset, dword);
}

uint8_t __pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint8_t aligned_offset = offset & -4;
	uint8_t byte_shift = offset - aligned_offset;
	uint32_t dword = __pci_config_read_dword(bus, slot, func, aligned_offset);
	
	return ((dword >> (byte_shift * 8)) & 0xff);
}

void __pci_write_qword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint64_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	ScopedSpinlock l(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* Write out the lower half of the data */
	outl(CONFIG_DATA, data & 0xFFFFFFFF);
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | ((offset+4) & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	outl(CONFIG_DATA, data & 0xFFFFFFFF00000000);
}


ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width)
{
	if(Width == 8)
		*Value = __pci_read_byte(PciId->Bus, PciId->Device, PciId->Function, Register);
	if(Width == 16)
		*Value = __pci_config_read_word(PciId->Bus, PciId->Device, PciId->Function, Register);
	if(Width == 32)
		*Value = __pci_config_read_dword(PciId->Bus, PciId->Device, PciId->Function, Register);
	if(Width == 64)
		panic("nooooooooooooooooooo");
	return AE_OK;
}

extern "C"
ACPI_STATUS AcpiOsWritePciConfiguration (ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width)
{
	if(Width == 8)
		__pci_write_byte(PciId->Bus, PciId->Device, PciId->Function, Register, (uint8_t)Value);
	if(Width == 16)
		__pci_write_word(PciId->Bus, PciId->Device, PciId->Function, Register, (uint16_t)Value);
	if(Width == 32)
		__pci_write_dword(PciId->Bus, PciId->Device, PciId->Function, Register, (uint32_t)Value);
	if(Width == 64)
		__pci_write_qword(PciId->Bus, PciId->Device, PciId->Function, Register, Value);

	return AE_OK;
}

extern "C"
ACPI_STATUS
AcpiOsPhysicalTableOverride (
ACPI_TABLE_HEADER * ExistingTable,
ACPI_PHYSICAL_ADDRESS *NewAddress,
UINT32 * NewTableLength)
{
	*NewAddress = 0;
	return AE_OK;
}

extern "C"
void AcpiOsPrintf (
const char *Format, ...)
{
	va_list params;
	va_start(params, Format);
	vprintf(Format, params);
	va_end(params);
}

extern "C"
void
AcpiOsWaitEventsComplete (
void)
{
	return;
}

extern "C"
ACPI_STATUS
AcpiOsSignal (
UINT32 Function,
void *Info)
{
	panic("Acpi Signal called!");
	return AE_OK;
}

extern "C"
UINT64
AcpiOsGetTimer (
void)
{
	panic("implement");
	return 0;
}

extern "C"
ACPI_STATUS
AcpiOsTerminate()
{
	return AE_OK;
}

extern "C"
int isprint(int ch)
{
	return 1;
}

extern "C"
void
AcpiOsVprintf(const char *Fmt, va_list Args)
{
	vprintf(Fmt, Args);
}

extern "C"
ACPI_STATUS
AcpiOsEnterSleep (
    UINT8                   SleepState,
    UINT32                  RegaValue,
    UINT32                  RegbValue)
    {
	    return AE_OK;
    }

#if 0

ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T        **ReturnCache)
{
	*ReturnCache = slab_create(CacheName, ObjectSize, MaxDepth, 0);
	return AE_OK;
}

ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T        *Cache)
{
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T        *Cache)
{
	return AE_OK;

}

ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T        *Cache,
    void                    *Object)
    {
	slab_free(Cache, Object);
    	return AE_OK;
    }

void *
AcpiOsAcquireObject (
    ACPI_CACHE_T        *Cache)
{
	return slab_allocate(Cache);
}
#endif
