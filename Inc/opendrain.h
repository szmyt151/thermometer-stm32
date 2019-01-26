/*
 * opendrain.h
 *
 *  Created on: 19.02.2018
 *      Author: korne
 */

#ifndef OPENDRAIN_H_
#define OPENDRAIN_H_

#include "stm32f1xx.h"

/**
 * External interrupt callback type
 */
typedef void ( *OD_Callback_t ) ( void );

void OD_Init ( OD_Callback_t falling, OD_Callback_t rising );
void OD_Pull ( void );
void OD_Release ( void );

#endif /* OPENDRAIN_H_ */
