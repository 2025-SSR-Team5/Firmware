extern "C" {
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "nvs_flash.h"
}

#define GATTS_NUM_HANDLE 4

class BleServer
{
    public:
        BleServer() {
            instance = this;
        }

        void print_ble_mac();
        void init();
        void handle_write_event(esp_ble_gatts_cb_param_t *param);
        void gatts_profile_event_handler_internal(esp_gatts_cb_event_t event,
                                              esp_gatt_if_t gatts_if,
                                              esp_ble_gatts_cb_param_t *param);
        static void gatts_profile_event_handler_static(esp_gatts_cb_event_t event,
                                                   esp_gatt_if_t gatts_if,
                                                   esp_ble_gatts_cb_param_t *param);
    private:
        static const char *TAG;

        static BleServer* instance; 

        uint16_t service_handle = 0;
        uint16_t char_attr_handle = 0;
        esp_gatt_char_prop_t char_property = 0;

        static const uint8_t SERVICE_UUID[16];
        static const uint8_t CHAR_UUID[16];

        esp_ble_adv_params_t adv_params = {
            .adv_int_min        = 0x20,
            .adv_int_max        = 0x40,
            .adv_type           = ADV_TYPE_IND,
            .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
            .channel_map        = ADV_CHNL_ALL,
            .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };
        
};