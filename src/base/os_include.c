#if OS_LINUX
# include "linux/linux_essential.h"
# include "linux/linux_essential.c"
#elif OS_WINDOWS
# include "win32/win32_essential.h"
# include "win32/win32_essential.c"
#else
# error no backend for os_include.c on this operating system
#endif
