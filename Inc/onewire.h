/*
 * onewire.h
 *
 *  Created on: 07.02.2018
 *      Author: korne
 */

#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#include "stm32f1xx.h"

void OW_Init ( void );

uint8_t OW_IsReady ( void );
uint8_t OW_IsPresent ( void );

void OW_Reset ( void );
void OW_WriteByte ( uint8_t byte );
void OW_ReadByte ( uint8_t* byte );

uint8_t OW_Watchdog ( void );

void OW_Callback_TimerEnd ( void );

#endif /* ONEWIRE_H_ */
