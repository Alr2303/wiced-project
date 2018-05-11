#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __cplusplus
    extern "C" char *		program_invocation_short_name;
#else
    extern char *			program_invocation_short_name;
#endif
#define getprogname()		program_invocation_short_name

