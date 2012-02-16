#ifndef _BACKTRACE_ARCH_H_
#define _BACKTRACE_ARCH_H_

#ifdef __mips__

#include "backtrace_mips.h"
#define get_backtrace(buffer, func, size, uc) backtrace_mips(buffer, func, size, uc)

#else

#ifdef __linux__
#include <execinfo.h>
#include <sys/ucontext.h>
#endif

#define get_backtrace(buffer, func, size, uc) backtrace(func, size)

#endif

#endif
