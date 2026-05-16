#ifndef __MY_TIME_H
#define __MY_TIME_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
uint64_t gettime_us(void);
uint64_t gettime_ms(void);
#ifdef __cplusplus
}
#endif

#endif //__MY_TIME_H