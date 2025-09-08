#include <../../../../components/Chassis/snippets/Motor.h>
#include <../../../../components/Chassis/src/Omni.h>
#include <../../../../components/Chassis/src/Chassis.h>
#include <../../../../components/ps3_controller_host/src/Ps3Controller.cpp>
#include <../../../../components/ESP32Servo/include/servoControl.h>
#include <../../../../components/ToF_Lidar/include/Lidar.hpp>

#include <initial_operation.hpp>
#include <bleserver.hpp>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_spp_api.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "nvs_flash.h"
}

//ピック機構を正面としたときの配置であり，基板シルク上のABCとは(現在は)対応していません
#define MOTOR_A_PWM GPIO_NUM_19
#define MOTOR_A_DIR GPIO_NUM_18

#define MOTOR_B_PWM GPIO_NUM_16
#define MOTOR_B_DIR GPIO_NUM_4

#define MOTOR_C_PWM GPIO_NUM_5
#define MOTOR_C_DIR GPIO_NUM_17

#define SERVO_A_PIN GPIO_NUM_14
#define SERVO_B_PIN GPIO_NUM_12 
#define SERVO_C_PIN GPIO_NUM_13

#define TOF_XSHUTA_PIN GPIO_NUM_33
#define TOF_XSHUTB_PIN GPIO_NUM_26

#define INSTRUCTION_DEFAULT 0xFF

extern i2c_master_dev_handle_t dev_STM32;

using namespace rct;

Motor motors[3] = {{MOTOR_A_PWM, MOTOR_A_DIR},{MOTOR_B_PWM, MOTOR_B_DIR},{MOTOR_C_PWM, MOTOR_C_DIR}};

Chassis<Omni<3>> chassis{
  [](std::array<float,3> pwm) {
  motors[0] = pwm[0];
  motors[1] = pwm[1];
  motors[2] = -pwm[2];
  },
  PidGain{0},//速度のゲイン
  PidGain{0.1}//位置のゲイン
};

servoControl servo[2];

/**
 * @brief 移動正面モード（0：ボールねじ正面、1：ピック機構＆カメラ正面、2：回路ボックス正面）
 * 
 */
int direction_mode = 0;
bool is_automatic_mode = false;

int battery = 0;
const auto& button = Ps3.data.button;
const auto& analog = Ps3.data.analog.stick;

void onConnect();
void DataUpdate();
void init_ps3();
void dump_input_state();

void init_ps3(){
  Ps3.attachOnConnect(onConnect);
  Ps3.begin("EC:E3:34:90:D8:EE");
}

void DataUpdate() {

  if ( battery != Ps3.data.status.battery ) {
    battery = Ps3.data.status.battery;
   printf("The controller battery is ");
    if ( battery == ps3_status_battery_charging )      printf("charging\r\n");
    else if ( battery == ps3_status_battery_full )     printf("FULL\r\n");
    else if ( battery == ps3_status_battery_high )     printf("HIGH\r\n");
    else if ( battery == ps3_status_battery_low)       printf("LOW\r\n");
    else if ( battery == ps3_status_battery_dying )    printf("DYING\r\n");
    else if ( battery == ps3_status_battery_shutdown ) printf("SHUTDOWN\r\n");
    else printf("UNDEFINED\r\n");
  }
  dump_input_state();
}

void dump_input_state()
{
    // ボタン状態をビットごとに出力
    ESP_LOGI("INPUT", "Buttons:");
    ESP_LOGI("INPUT", "  select: %d", button.select);
    ESP_LOGI("INPUT", "  l3: %d, r3: %d", button.l3, button.r3);
    ESP_LOGI("INPUT", "  start: %d, ps: %d", button.start, button.ps);
    ESP_LOGI("INPUT", "  d-pad: up:%d right:%d down:%d left:%d",
             button.up, button.right, button.down, button.left);
    ESP_LOGI("INPUT", "  shoulder: l1:%d r1:%d l2:%d r2:%d",
             button.l1, button.r1, button.l2, button.r2);
    ESP_LOGI("INPUT", "  shapes: triangle:%d circle:%d cross:%d square:%d",
             button.triangle, button.circle, button.cross, button.square);

    // アナログスティックの値
    ESP_LOGI("INPUT", "Analog Stick:");
    ESP_LOGI("INPUT", "  Left  (lx: %d, ly: %d)", analog.lx, analog.ly);
    ESP_LOGI("INPUT", "  Right (rx: %d, ry: %d)", analog.rx, analog.ry);

    // バッテリーステータス
    ESP_LOGI("INPUT", "Battery: %d", battery);
}


void onConnect() {
  printf("PS3 controller connected\r\n");
  Ps3.setPlayer(1);
}

void bluetooth_init() {
    const char* TAG = "BT_INIT";

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        ret = esp_bt_controller_init(&bt_cfg);
        ESP_ERROR_CHECK(ret);
    } else {
        ESP_LOGW(TAG, "BT controller already initialized");
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    ESP_ERROR_CHECK(ret);

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


    ESP_LOGI(TAG, "Bluetooth initialized successfully in dual mode.");
}

bool r1_last_button_up = false;
bool r1_last_button_down = false;
bool l1_last_button_circle = false;
bool last_select_button = false;

Velocity rotation_velocity(Velocity v){
  float v_x_new;
  float v_y_new;
  Velocity v_new;
  switch(direction_mode){
    case 0:
      return v;
    case 1:
      v_x_new = v.x_milli * cos(M_PI * 2.0 / 3.0) - v.y_milli * sin(M_PI * 2.0 / 3.0);
      v_y_new = v.x_milli * sin(M_PI * 2.0 / 3.0) + v.y_milli * cos(M_PI * 2.0 / 3.0);
      v_new = {v_x_new, v_y_new, v.ang_rad};
      return v_new;
    case 2:
      v_x_new = v.x_milli * cos(-M_PI * 2.0 / 3.0) - v.y_milli * sin(-M_PI * 2.0 / 3.0);
      v_y_new = v.x_milli * sin(-M_PI * 2.0 / 3.0) + v.y_milli * cos(-M_PI * 2.0 / 3.0);
      v_new = {v_x_new, v_y_new, v.ang_rad};
      return v_new;
  }
  return v;
}

Coordinate rotation_pos(Coordinate p){
  float p_x_new;
  float p_y_new;
  Coordinate p_new;
  switch(direction_mode){
    case 0:
      return p;
    case 1:
      p_x_new = p.x_milli * cos(M_PI * 2.0 / 3.0) - p.y_milli * sin(M_PI * 2.0 / 3.0);
      p_y_new = p.x_milli * sin(M_PI * 2.0 / 3.0) + p.y_milli * cos(M_PI * 2.0 / 3.0);
      p_new = {p_x_new, p_y_new, p.ang_rad};
      return p_new;
    case 2:
      p_x_new = p.x_milli * cos(-M_PI * 2.0 / 3.0) - p.y_milli * sin(-M_PI * 2.0 / 3.0);
      p_y_new = p.x_milli * sin(-M_PI * 2.0 / 3.0) + p.y_milli * cos(-M_PI * 2.0 / 3.0);
      p_new = {p_x_new, p_y_new, p.ang_rad};
      return p_new;
  }
  return p;
}

extern "C" void app_main() {
    i2c_master_bus_handle_t bus_handle;
    
    BleServer bleServer;
    ToF_Lidar tof(TOF_XSHUTA_PIN, TOF_XSHUTB_PIN, SERVO_C_PIN);
    bluetooth_init();

    int rotation_count = 0;
    int stay_theta = 17;
    int64_t prev_time = esp_timer_get_time();

    //vTaskDelay(pdMS_TO_TICKS(100));
    ESP_init(&bus_handle);
    if(tof.init(bus_handle)){
        tof.start_task();
    }

    init_ps3();
    bleServer.init();

    servo[0].attach(SERVO_A_PIN, 500, 2400, LEDC_CHANNEL_5, LEDC_TIMER_0);
    servo[1].attach(SERVO_B_PIN, 500, 2400, LEDC_CHANNEL_6, LEDC_TIMER_0);

    servo[0].write(90);
    servo[1].write(90);

    while(true){
        if(Ps3.isConnected()){
          DataUpdate();
          if(!is_automatic_mode) ps3SetLed(direction_mode + 1);
        }

        if(tof.is_task_finished() && rotation_count <= 2){
          tof.start_task();
        }

        int analog_x = analog.lx;
        int analog_y = analog.ly;
        int analog_r = analog.rx;

        float analog_x_num = -analog_x/128.0;
        float analog_y_num = -analog_y/128.0;
        float analog_r_num = analog_r/128.0;

        uint8_t instruction = INSTRUCTION_DEFAULT;

        if(button.r1){
            if(button.up){
                r1_last_button_up = true;
            }
            if(button.down){
                r1_last_button_down = true;
            }
        }

        if(button.l1){
          if(button.circle){
            l1_last_button_circle = true;
          }
        }

        if(button.select){
          last_select_button = true;
        }

        if((button.up == 0) && r1_last_button_up){
            instruction = 0x01;
            r1_last_button_up = false;
        }

        if((button.down == 0) && r1_last_button_down){
            instruction = 0x02;
            r1_last_button_down = false;
        }

         if(instruction != INSTRUCTION_DEFAULT){
            i2c_master_transmit(dev_STM32, &instruction, sizeof(instruction), pdMS_TO_TICKS(1000));
            printf("Send to STM32!\n");
        }

        if((button.select == 0) && last_select_button){
          direction_mode = (direction_mode + 1) % 3;
          printf("direction_mode: %d\n",direction_mode);
          last_select_button = false;
        }

        if((button.circle == 0) && l1_last_button_circle){
          //自動走行モード
          direction_mode = 1;
          is_automatic_mode = true;
          if(tof.vertical_distance > 308.5){
            //ライントレースの処理
            int64_t now = esp_timer_get_time();
            auto delta_us = std::chrono::microseconds(now - prev_time);
            float centerline = bleServer.centerLine;
            float width = bleServer.width;
            float coefficient = 0.8;
            float deviation = coefficient * ((centerline - width/2) / (width/2));
            Coordinate dst = {0.3,0,0};
            Coordinate pos = {0.3,0,deviation};
            chassis.auto_move(rotation_pos(dst), rotation_pos(pos), delta_us);
            prev_time = now;
            if(Ps3.isConnected()){
              if(rotation_count >= 2){
                ps3SetLed(10);
              }else{
                ps3SetLed(5);
              }
            }
          }else{
            if(rotation_count < 2){
              //旋回処理
              if(Ps3.isConnected()){
                ps3SetLed(8);
              }
              float azimuth_init = bleServer.azimuth;
              while(fabsf(bleServer.azimuth - azimuth_init) < 90.0){
                Velocity vel = {0,0,0.5};
                chassis.move(rotation_velocity(vel));
                if(tof.is_task_finished()){
                  tof.start_task();
                }
                vTaskDelay(pdMS_TO_TICKS(10));
              }
              rotation_count++;
            }
          }

          if(rotation_count >= 2){
            if(tof.scan(&stay_theta)){
              servo[0].write(30);
              servo[1].write(160);
              vTaskDelay(pdMS_TO_TICKS(1000));
              l1_last_button_circle = false;
              is_automatic_mode = false;
              rotation_count = 0;
            }
          }
        }else{
          //手動走行モード
          Velocity vel = {analog_x_num, analog_y_num, analog_r_num};
          chassis.move(rotation_velocity(vel));
        }
        
        if(button.l2){
          servo[0].write(30);
        }else{
          servo[0].write(90);
        }

        if(button.r2){
          servo[1].write(160);
        }else{
          servo[1].write(90);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}