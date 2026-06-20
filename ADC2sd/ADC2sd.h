#include <string.h>
#include "initADC.h"
#include "initSDspi.h" // Used for spi mode
#include "format_wav.h"


#ifdef __cplusplus
extern "C" {
#endif

void record_wav(uint32_t rec_time, const char *filename);

#ifdef __cplusplus
}
#endif