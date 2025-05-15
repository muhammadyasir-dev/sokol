#include <endian.h>

/* For writing/reading the version field in a portable manner */
static inline uint64_t cpu_ucode_htole64(uint64_t val) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return __bswap_64(val);  /* Convert from big endian to little endian */
#else
    return val;              /* Already little endian */
#endif
}

static inline uint64_t cpu_ucode_letoh64(uint64_t val) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return __bswap_64(val);  /* Convert from little endian to big endian */
#else
    return val;              /* Already little endian */
#endif
}
