#include <array>
#include "driver/gpio.h"
#include "driver/ledc.h"

#define MOTOR_A_PWM GPIO_NUM_16
#define MOTOR_A_DIR GPIO_NUM_4
#define MOTOR_B_PWM GPIO_NUM_17
#define MOTOR_B_DIR GPIO_NUM_5

// ESP-IDF用の簡単なMotorクラス
class Motor {
public:
    Motor() = default;
    Motor(gpio_num_t pwm_pin, gpio_num_t dir_pin) : pwm_pin_(pwm_pin), dir_pin_(dir_pin) {}
    
    void set_pwm(float power) {
        // PWM設定の実装（後で追加）
    }
    
    Motor& operator=(float val) {
        set_pwm(val);
        return *this;
    }
    
private:
    gpio_num_t pwm_pin_;
    gpio_num_t dir_pin_;
};

Motor motors[3] = {
    Motor(MOTOR_A_PWM, MOTOR_A_DIR),
    Motor(MOTOR_B_PWM, MOTOR_B_DIR),
    Motor(GPIO_NUM_18, GPIO_NUM_19)
};

extern "C" void app_main() {
    
}