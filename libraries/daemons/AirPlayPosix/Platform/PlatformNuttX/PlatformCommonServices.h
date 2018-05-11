#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

/* random() - not supported by NuttX, use rand() instead. */
#define random() (uint32_t)rand()

#define getprogname()		"airplay"

