/*
 * flash_db.h
 *
 *  Created on: 21.02.2018
 *      Author: korne
 */

#ifndef FLASH_DB_H_
#define FLASH_DB_H_

#include "stm32f1xx.h"

void FDB_Init ( void );
uint8_t FDB_RegisterVal ( uint16_t readout );
uint16_t FDB_GetCountOfSamples ( void );
void FDB_ResetReadTail ( void );
uint16_t FDB_ReadData ( void );

#endif /* FLASH_DB_H_ */
