#pragma comment(lib, "ws2_32")

#include <limits.h>
#include <stdint.h>
#include <winsock2.h>

#ifdef __cplusplus
# define INC_BIN_EXTERN  extern "C"
#else
# define INC_BIN_EXTERN  extern
#endif


/**
 * @Dale Weiler
 * https://github.com/graphitemaster/incbin/blob/main/incbin.h
 */
#if   defined(__AVX512BW__) || \
      defined(__AVX512CD__) || \
      defined(__AVX512DQ__) || \
      defined(__AVX512ER__) || \
      defined(__AVX512PF__) || \
      defined(__AVX512VL__) || \
      defined(__AVX512F__)
# define INC_BIN_ALIGN 64
#elif defined(__AVX__)      || \
      defined(__AVX2__)
# define INC_BIN_ALIGN 32
#elif defined(__SSE__)      || \
      defined(__SSE2__)     || \
      defined(__SSE3__)     || \
      defined(__SSSE3__)    || \
      defined(__SSE4_1__)   || \
      defined(__SSE4_2__)   || \
      defined(__neon__)     || \
      defined(__ARM_NEON)   || \
      defined(__ALTIVEC__)
# define INC_BIN_ALIGN 16
#elif ULONG_MAX != 0xffffffffu
# define INC_BIN_ALIGN 8
#else
# define INC_BIN_ALIGN 4
#endif


/* reference the data */
#define INC_BIN(PTR)      INC_BIN_EXTERN const __declspec( align(INC_BIN_ALIGN) ) uint8_t PTR[]

/* pointer to data (4 bytes offset) */
#define GETINC_BIN(PTR)   ( ((const uint8_t *)PTR) + 4 )
#define GETINC_TEXT(PTR)  ( ((const char *)PTR)    + 4 )

/* receive data size (saved as uint32_t Big Endian) */
#define GETINC_SIZE(PTR)  ntohl(*((const uint32_t *)PTR))


#if 0

/* reference external symbol "data_txt"
 * created from file "data.txt" */
INC_BIN(data_txt);

/* create pointers to data */
const uint8_t *ptr = GETINC_BIN(data_txt);
const char *text = GETINC_TEXT(data_txt);

/* print data size */
printf("size: %d\n", GETINC_SIZE(data_txt));

#endif
