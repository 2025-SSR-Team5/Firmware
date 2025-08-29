/*
 * initial_operation.h
 *
 *  Created on: Aug 15, 2025
 *      Author: takeu
 */

#ifndef INC_INITIAL_OPERATION_H_
#define INC_INITIAL_OPERATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

HAL_StatusTypeDef Check_I2C_to_ESP32(uint8_t devstat);
/*
 * devstat=0 : 正常
 * devstat=1 ： 異常
 * devstat=2 ： スリープモード
 * devstat=3 : ユーザー定義
 */


#ifdef __cplusplus
}
#endif

#endif /* INC_INITIAL_OPERATION_H_ */
