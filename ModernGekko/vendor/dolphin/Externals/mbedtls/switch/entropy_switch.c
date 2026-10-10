/* mbedtls hardware entropy source for the Nintendo Switch (libnx). */
#include "common.h"

#include <stddef.h>

#include <switch.h>

#include "mbedtls/entropy_poll.h"

int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t* olen)
{
    (void)data;
    /* Seeded from svcGetInfo(RandomEntropy); the kernel's own CSPRNG output. */
    randomGet(output, len);
    *olen = len;
    return 0;
}
