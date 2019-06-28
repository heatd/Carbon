#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xdeadc0de
#else
#define STACK_CHK_GUARD 0xdeadd00ddeadc0de
#endif

unsigned long __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void panic(const char *s);

__attribute__((noreturn))
void __stack_chk_fail()
{
	panic("*******stack smashing detected*******");
}