#include <inttypes.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup NVS_helperfuncs
 * 
 * @brief assists in error checking when reading and writing values in the non-Volatile storage.
 * 
 * @{
 */

int32_t get_nvs_counter(const char * dict_key);
bool set_nvs_counter(const char * dict_key, int32_t dict_val);

/** @} */ // This closes the group

#ifdef __cplusplus
}
#endif