/*
 * connection.c
 *
 *  Created on: Aug 15, 2025
 *      Author: takeu
 */

#include "initial_operation.h"
#include "connection.h"
#include "stepper.h"

#define STEP_PER_REV 512

extern I2C_HandleTypeDef hi2c1;

HAL_StatusTypeDef do_instruction(uint8_t devstat, uint8_t instruction){
	HAL_StatusTypeDef result = HAL_ERROR;
	switch (instruction){
	case 0x00 :
		result = Check_I2C_to_ESP32(devstat);
		break;
	case 0x01 :
		//if(HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_4) == GPIO_PIN_SET){
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_11);
			move_clockwise(STEP_PER_REV,500);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_11);
		//}
		break;
	case 0x02 :
		//if(HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_5) == GPIO_PIN_SET){
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
			move_anticlockwise(STEP_PER_REV,500);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
		//}
		break;
	default:
		break;
	}

	return result;
}
