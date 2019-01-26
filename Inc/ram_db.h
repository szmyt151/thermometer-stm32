/*
 * ram_db.h
 *
 *  Created on: 23.02.2018
 *      Author: korne
 */

#ifndef RAM_DB_H_
#define RAM_DB_H_

#include "main.h"

void RDB_Init ( void );
void RDB_RegisterVal ( uint16_t T );
void RDB_ResetReadTail ( void );
uint16_t RDB_ReadData ( void );
uint16_t RDB_GetCountOfSamples ( void );

#endif /* RAM_DB_H_ */
