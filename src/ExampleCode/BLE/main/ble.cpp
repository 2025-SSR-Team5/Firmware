// main/ble.cpp  (C++ compatible, for ESP-IDF v5.x)
// 修正内容:
// - C ヘッダを extern "C" で囲み C++ ビルドエラー回避
// - サービス/キャラクタリスティックのハンドルを動的管理 (service_handle, char_attr_handle)
// - ADD_CHAR_EVT で param 側の property フィールドに依存しない（自分で設定したフラグを保持）
// - 不要な gatts_handle_table を排除
// - 安全な書き込み処理（長さチェック等）
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

static const char *TAG = "BLE_HANDLER";

// ハンドル管理（動的にセット）
static uint16_t service_handle = 0;
static uint16_t char_attr_handle = 0;
static esp_gatt_char_prop_t char_property = 0;

#define GATTS_NUM_HANDLE_TEST 4

static const uint8_t SERVICE_UUID[16] = {
    0xab, 0x90, 0x78, 0x56, 0x34, 0x12, 0x12, 0x34,
    0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab
};

static const uint8_t CHAR_UUID[16] = {
    0xab, 0xcd, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78,
    0x90, 0xab, 0xcd, 0xef, 0x56, 0x78, 0x90, 0xab
};

void print_ble_mac() {
    const uint8_t* mac = esp_bt_dev_get_address();
    if (mac != nullptr) {
        ESP_LOGI("BLE_MAC", "BLE MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE("BLE_MAC", "Failed to get BLE MAC address");
    }
}

// 書き込み受信処理（安全にバイト長チェックして memcpy）
static void handle_write_event(esp_ble_gatts_cb_param_t *param) {
    // 書き込みハンドルが登録したキャラクタリスティックと等しいか
    if (param->write.handle != char_attr_handle) {
        ESP_LOGW(TAG, "Write to unknown handle %d (expected %d)", param->write.handle, char_attr_handle);
        return;
    }

    if (param->write.len != 12) {
        ESP_LOGW(TAG, "Unexpected data length: %d (expected 12)", param->write.len);
        return;
    }

    // float を直接 memcpy（環境が Little Endian 前提）。
    float roll = 0.0f, pitch = 0.0f, azimuth = 0.0f;
    memcpy(&roll,  param->write.value + 0, 4);
    memcpy(&pitch, param->write.value + 4, 4);
    memcpy(&azimuth,param->write.value + 8, 4);

    ESP_LOGI(TAG, "Received roll=%.3f pitch=%.3f azimuth=%.3f (len=%d)",
             roll, pitch, azimuth, param->write.len);
}

// GATT イベントハンドラ
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            // サービス ID を組み立ててサービス作成を要求する
            esp_gatt_srvc_id_t service_id;
            memset(&service_id, 0, sizeof(service_id));
            service_id.is_primary = true;
            service_id.id.inst_id = 0;
            service_id.id.uuid.len = ESP_UUID_LEN_128;
            memcpy(service_id.id.uuid.uuid.uuid128, SERVICE_UUID, 16);

            esp_err_t err = esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE_TEST);
            if (err) {
                ESP_LOGE(TAG, "create_service failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "create_service requested");
            }
            break;
        }

        case ESP_GATTS_CREATE_EVT: {
            // create_evt では service_handle が返る（uint16_t）
            service_handle = param->create.service_handle;
            ESP_LOGI(TAG, "Service created, handle=%d", service_handle);

            esp_err_t err = esp_ble_gatts_start_service(service_handle);
            if (err) {
                ESP_LOGE(TAG, "start_service failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "start_service ok");
            }

            // キャラクタリスティック UUID を構築して追加
            esp_bt_uuid_t char_uuid;
            memset(&char_uuid, 0, sizeof(char_uuid));
            char_uuid.len = ESP_UUID_LEN_128;
            memcpy(char_uuid.uuid.uuid128, CHAR_UUID, 16);

            // 自分で設定するプロパティを保持しておく（param に依存しない）
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

        case ESP_GATTS_ADD_CHAR_EVT: {
            // attr_handle を保存（param のフィールド名は IDF のバージョンによって変わるため attr_handle を利用）
            char_attr_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "CHAR added: handle=%d (prop=0x%02x)", char_attr_handle, char_property);
            break;
        }

        case ESP_GATTS_WRITE_EVT: {
            // 書き込みイベントの処理は専用関数へ
            handle_write_event(param);

            // Android（クライアント）が応答を要求している場合
            if (param->write.need_rsp) {
                // 応答を送信する
                esp_ble_gatts_send_response(gatts_if, 
                                            param->write.conn_id, 
                                            param->write.trans_id, 
                                            ESP_GATT_OK, 
                                            NULL);
            }
            break;
        }

        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "Client connected, conn_id=%d", param->connect.conn_id);
            // オプション: 接続が確立したらアドバタイズを停止して電力消費を抑える
            esp_ble_gap_stop_advertising();
            break;
        }
        
        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(TAG, "Client disconnected, conn_id=%d, reason=0x%x", param->disconnect.conn_id, param->disconnect.reason);
            // オプション: 切断されたら再度アドバタイズを開始して次の接続を待つ
            esp_ble_adv_params_t adv_params = {
                    .adv_int_min        = 0x20,
                    .adv_int_max        = 0x40,
                    .adv_type           = ADV_TYPE_IND,
                    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
                    .channel_map        = ADV_CHNL_ALL,
                    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                };
            esp_ble_gap_start_advertising(&adv_params); // adv_params をグローバルにするか、再度定義する必要あり
            break;
        }

        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "GATT service started");
            // ここでアドバタイズ開始
            {
                esp_ble_adv_params_t adv_params = {
                    .adv_int_min        = 0x20,
                    .adv_int_max        = 0x40,
                    .adv_type           = ADV_TYPE_IND,
                    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
                    .channel_map        = ADV_CHNL_ALL,
                    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                };

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

extern "C" void app_main(void) {
    esp_err_t ret;

    // 注意: PS3コントローラーなど Classic BT が既に使われている環境では
    // esp_bt_controller_mem_release(ESP_BT_MODE_BLE) を呼ばないこと（メモリ解放してしまう）。
    // BTDM モードで Classic + BLE を有効にする。
    // NVS 初期化
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bt_controller_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM); // Classic + BLE
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bt_controller_enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bluedroid_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bluedroid_enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // GATT コールバック登録 & アプリ登録
    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gatts_register_callback failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_app_register(0); // アプリID=0
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gatts_app_register failed: %s", esp_err_to_name(ret));
        return;
    }

    // オプション: デバイス名を設定（必要なら）
    esp_err_t e = esp_ble_gap_set_device_name("ESP32_BLE_HANDLER");
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "set_device_name failed: %s", esp_err_to_name(e));
    }

    ESP_LOGI(TAG, "BLE GATT server init done");
}