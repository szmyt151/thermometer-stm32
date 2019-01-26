/*
 * dallas.h
 *
 *  Created on: 07.02.2018
 *      Author: korne
 */

#ifndef DALLAS_H_
#define DALLAS_H_

#include "main.h"

void DS_Init (  );
void DS_DoWork (  );
uint8_t DS_PrintTemp ( uint16_t temp );
void DS_PrintROM ( void );
uint16_t DS_LaunchConversion ( void );

#endif /* DALLAS_H_ */
