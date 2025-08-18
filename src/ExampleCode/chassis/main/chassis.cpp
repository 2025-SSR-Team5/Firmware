/*

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

#include <array>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MOTOR_A_PWM GPIO_NUM_16
#define MOTOR_A_DIR GPIO_NUM_4

#define MOTOR_B_PWM GPIO_NUM_5
#define MOTOR_B_DIR GPIO_NUM_17

#define MOTOR_C_PWM GPIO_NUM_19
#define MOTOR_C_DIR GPIO_NUM_18

using namespace rct;

Motor motors[3] = {{MOTOR_A_PWM, MOTOR_A_DIR},{MOTOR_B_PWM, MOTOR_B_DIR},{MOTOR_C_PWM, MOTOR_C_DIR}};

Omni<3> omni{[](std::array<float, 3> pwm) {
  for(int i = 0; i < 3; ++i) {
    motors[i] = pwm[i];
  }
}};

extern "C" void app_main() {
    
    while(true) {
        Velocity vel = {0, -0.5, 0};
        omni.move(vel);

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms遅延
    }
}