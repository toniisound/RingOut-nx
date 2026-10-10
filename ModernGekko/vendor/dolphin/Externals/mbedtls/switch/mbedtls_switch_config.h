/* mbedtls adjustments for the Nintendo Switch (libnx) build.
 * Included through MBEDTLS_USER_CONFIG_FILE, after include/mbedtls/config.h.
 *
 * - No /dev/urandom or getrandom(): entropy comes from the kernel's random
 *   source via MBEDTLS_ENTROPY_HARDWARE_ALT (switch/entropy_switch.c).
 * - No alarm(): MBEDTLS_TIMING_C is only used by DTLS/self-tests, never here.
 * - MBEDTLS_NET_C stays: libnx has BSD sockets, and the Wii SSL emulation
 *   calls mbedtls_net_send/recv. net_sockets.c accepts __SWITCH__ (patched). */
#ifndef MBEDTLS_SWITCH_CONFIG_H
#define MBEDTLS_SWITCH_CONFIG_H

#undef MBEDTLS_TIMING_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#endif
