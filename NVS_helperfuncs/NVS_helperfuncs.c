
#include "NVS_helperfuncs.h"

static const char *TAG = "NVS Helper Functions";
int32_t get_nvs_counter(const char * dict_key){
    /**
    * \brief The function will read a dictionary integer from nvs then return the value read.
    *
    * \param dict_key This is the key for the nvs dictionary.
    * \return returns the value at the key provided or ~0 if there was an error
    */
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // Open NVS handle
    ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return ~0; //This would be an error
    }
    // Read back the value
    ESP_LOGI(TAG, "\nReading counter from NVS...");
    int32_t dict_val = 0;
    err = nvs_get_i32(my_handle, dict_key, &dict_val);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read counter = %" PRIu32, dict_val);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "The value is not initialized yet!");
            dict_val = 0;
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    // Close
    nvs_close(my_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    return dict_val;
}

bool set_nvs_counter(const char * dict_key, int32_t dict_val){
    /**
    * \brief The function will set a dictionary integer from nvs then save it to the pointer provided
    *
    * \param nvs_ptr this is the pointer where the nvs integer will be saved.
    * \param dict_key This is the key for the nvs dictionary.
    * \return if true there were no errors else false.
    */
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // Open NVS handle
    ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return false; //This would be an error
    }
    bool res = true;
    ESP_LOGI(TAG, "\nWriting counter to NVS...");
    err = nvs_set_i32(my_handle, dict_key, dict_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write counter!");
        res = false;
    }
    // Commit changes
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    ESP_LOGI(TAG, "\nCommitting updates in NVS...");
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
        res = false;
    }
    // Close
    nvs_close(my_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    return res;
}