/*
 * serial.h
 *
 *  Created on: 21.02.2018
 *      Author: korne
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include "stm32f1xx.h"
#include "stdio.h"

enum
{
	S_Command_None,
	S_Command_SetSamplingPeriod,
	S_Command_ReadRam,
	S_Command_ReadFlash
};

void S_Init ( void );
uint8_t S_IsNewData ( void );
uint8_t S_GetCommand ( void );

#endif /* SERIAL_H_ */
