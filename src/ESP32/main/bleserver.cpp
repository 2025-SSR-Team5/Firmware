#include "bleserver.hpp"

BleServer* BleServer::instance = nullptr;

const char* BleServer::TAG = "BLE_HANDLER";

const uint8_t BleServer::SERVICE_UUID[16] = {
    0xab, 0x90, 0x78, 0x56, 0x34, 0x12, 0x12, 0x34,
    0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab
};

const uint8_t BleServer::CHAR_UUID[16] = {
    0xab, 0xcd, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78,
    0x90, 0xab, 0xcd, 0xef, 0x56, 0x78, 0x90, 0xab
};

/**
 * @brief BLEのMACアドレスを表示する関数
 * 
 */
void BleServer::print_ble_mac() {
    const uint8_t* mac = esp_bt_dev_get_address();
    if (mac != nullptr) {
        ESP_LOGI("BLE_MAC", "BLE MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE("BLE_MAC", "Failed to get BLE MAC address");
    }
}

/**
 * @brief クライアントから書き込みイベントが来た時の処理
 * @note roll,pitch,azimuth または width, centerLine を更新する
 * 
 * @param param 
 */
void BleServer::handle_write_event(esp_ble_gatts_cb_param_t *param) {
    if (param->write.handle != char_attr_handle) {
        ESP_LOGW(TAG, "Write to unknown handle %d (expected %d)", param->write.handle, char_attr_handle);
        return;
    }

    if (param->write.len != 12 && param->write.len != 8) {
        ESP_LOGW(TAG, "Unexpected data length: %d (expected 8 or 12)", param->write.len);
        return;
    }

    switch (param->write.len) {
        case 8:
            memcpy(&centerLine, param->write.value + 0, 4);
            memcpy(&width, param->write.value + 4, 4);
            ESP_LOGI(TAG, "Received centerLine=%.1f width=%.1f (len=%d)",
             centerLine, width, param->write.len);
            break;

        case 12:
            memcpy(&roll,  param->write.value + 0, 4);
            memcpy(&pitch, param->write.value + 4, 4);
            memcpy(&azimuth,param->write.value + 8, 4);
            ESP_LOGI(TAG, "Received roll=%.3f pitch=%.3f azimuth=%.3f (len=%d)",
             roll, pitch, azimuth, param->write.len);
            break;
    }
}

/**
 * @brief BLE GATTサーバーイベントを処理するコールバック関数の静的ラッパー関数
 * 
 * @param event 
 * @param gatts_if 
 * @param param 
 */
void BleServer::gatts_profile_event_handler_static(esp_gatts_cb_event_t event,
                                                   esp_gatt_if_t gatts_if,
                                                   esp_ble_gatts_cb_param_t *param) {
    if (instance != nullptr) {
        instance->gatts_profile_event_handler_internal(event, gatts_if, param);
    }
}

/**
 * @brief BLE GATTサーバーイベントを処理するコールバック関数
 * 
 * @param event 
 * @param gatts_if 
 * @param param 
 */
void BleServer::gatts_profile_event_handler_internal(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        //GATTサーバーが登録されたときの処理
        case ESP_GATTS_REG_EVT: {
            esp_gatt_srvc_id_t service_id;
            memset(&service_id, 0, sizeof(service_id));
            service_id.is_primary = true;
            service_id.id.inst_id = 0;
            service_id.id.uuid.len = ESP_UUID_LEN_128;
            memcpy(service_id.id.uuid.uuid.uuid128, SERVICE_UUID, 16);

            esp_err_t err = esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
            if (err) {
                ESP_LOGE(TAG, "create_service failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "create_service requested");
            }
            break;
        }

        //サービスが作成された時の処理
        case ESP_GATTS_CREATE_EVT: {
            service_handle = param->create.service_handle;
            ESP_LOGI(TAG, "Service created, handle=%d", service_handle);

            esp_err_t err = esp_ble_gatts_start_service(service_handle);
            if (err) {
                ESP_LOGE(TAG, "start_service failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "start_service ok");
            }

            esp_bt_uuid_t char_uuid;
            memset(&char_uuid, 0, sizeof(char_uuid));
            char_uuid.len = ESP_UUID_LEN_128;
            memcpy(char_uuid.uuid.uuid128, CHAR_UUID, 16);

            esp_gatt_char_prop_t wanted_prop = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
            char_property = wanted_prop;

            err = esp_ble_gatts_add_char(service_handle,
                                        &char_uuid,
                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                        wanted_prop,
                                        NULL, NULL);
            if (err) {
                ESP_LOGE(TAG, "add_char failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "add_char requested");
            }
            break;
        }

        //キャラクタリスティックが追加された時の処理
        case ESP_GATTS_ADD_CHAR_EVT: {
            char_attr_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "CHAR added: handle=%d (prop=0x%02x)", char_attr_handle, char_property);
            break;
        }

        //クライアントから書き込まれた時の処理
        case ESP_GATTS_WRITE_EVT: {
            handle_write_event(param);

            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, 
                                            param->write.conn_id, 
                                            param->write.trans_id, 
                                            ESP_GATT_OK, 
                                            NULL);
            }
            break;
        }

        //クライアントが接続された時の処理
        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "Client connected, conn_id=%d", param->connect.conn_id);
            esp_ble_gap_stop_advertising();
            break;
        }
        
        //クライアントが切断したときの処理
        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(TAG, "Client disconnected, conn_id=%d, reason=0x%x", param->disconnect.conn_id, param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        }

        //GATTサービスが開始された時の処理
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "GATT service started");
            {
                esp_err_t err = esp_ble_gap_start_advertising(&adv_params);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "adv_start failed: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "BLE advertising started");
                }
            }
            break;

        case ESP_GATTS_STOP_EVT:
            ESP_LOGI(TAG, "GATT service stopped");
            break;

        default:
            break;
    }
}

/**
 * @brief BLEの初期化を行う関数
 * 
 */
void BleServer::init(){
    const char* TAG = "BleServer";
    esp_err_t ret;
    // ret = nvs_flash_init();
    // //NVSの初期化に失敗した場合、NVS領域を消去して再度初期化
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    // esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
    //     ret = esp_bt_controller_init(&bt_cfg);
    //     ESP_ERROR_CHECK(ret);
    // } else {
    //     ESP_LOGW(TAG, "BT controller already initialized");
    // }

    // //Classic BluetoothとBLEの両方を使用する
    // ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "bt_controller_enable failed: %s", esp_err_to_name(ret));
    //     return;
    // }

    // ret = esp_bluedroid_init();
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "bluedroid_init failed: %s", esp_err_to_name(ret));
    //     return;
    // }

    // ret = esp_bluedroid_enable();
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "bluedroid_enable failed: %s", esp_err_to_name(ret));
    //     return;
    // }

    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_LOGE(TAG, "Bluedroid not enabled yet");
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler_static);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gatts_register_callback failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_app_register(0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gatts_app_register failed: %s", esp_err_to_name(ret));
        return;
    }

    esp_err_t e = esp_ble_gap_set_device_name("ESP32_BLE_HANDLER");
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "set_device_name failed: %s", esp_err_to_name(e));
    }

    ESP_LOGI(TAG, "BLE GATT server init done");
    
}

