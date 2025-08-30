/*
2025/08/25 動作確認完了

     A
     ◯
    /  \
   /    \
  /  　  \ 
 /        \
◯---------◯
C        　 B

*/

#include <../../../../components/Chassis/snippets/Motor.h>
#include <../../../../components/Chassis/src/Omni.h>
#include <../../../../components/ps3_controller_host/src/Ps3Controller.cpp>

#include <array>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MOTOR_B_PWM GPIO_NUM_16
#define MOTOR_B_DIR GPIO_NUM_4

#define MOTOR_C_PWM GPIO_NUM_5
#define MOTOR_C_DIR GPIO_NUM_17

#define MOTOR_A_PWM GPIO_NUM_19
#define MOTOR_A_DIR GPIO_NUM_18

using namespace rct;

Motor motors[3] = {{MOTOR_A_PWM, MOTOR_A_DIR},{MOTOR_B_PWM, MOTOR_B_DIR},{MOTOR_C_PWM, MOTOR_C_DIR}};

Omni<3> omni{[](std::array<float, 3> pwm) {
  motors[0] = pwm[0];
  motors[1] = pwm[1];
  motors[2] = -pwm[2];
}};

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

extern "C" void app_main() {
  init_ps3();
    
    while(true) {
        if(Ps3.isConnected()){
          DataUpdate();
        }

        int analog_x = analog.lx;
        int analog_y = analog.ly;
        int analog_r = analog.rx;

        float analog_x_num = -analog_x/128.0;
        float analog_y_num = -analog_y/128.0;
        float analog_r_num = analog_r/128.0;

        Velocity vel = {analog_x_num, analog_y_num, analog_r_num};
        omni.move(vel);

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms遅延
    }
}