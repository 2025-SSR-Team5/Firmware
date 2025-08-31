/*
 * motion.h
 *
 *  Created on: Aug 15, 2025
 *      Author: takeu
 */

#ifndef INC_CONNECTION_H_
#define INC_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

HAL_StatusTypeDef do_instruction(uint8_t devstat, uint8_t instruction);

#ifdef __cplusplus
}
#endif

#endif /* INC_CONNECTION_H_ */
