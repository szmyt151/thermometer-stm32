/*
 * opendrain.c
 *
 *  Created on: 19.02.2018
 *      Author: korne
 */

#include "opendrain.h"
#include "main.h"

/* Exti callbacks */
OD_Callback_t RisingCall;
OD_Callback_t FallingCall;

/**
 * @brief Initialize open-drain pin with EXTI
 */
void OD_Init ( OD_Callback_t falling, OD_Callback_t rising )
{
/* Assign interrupts */
	RisingCall = rising;
	FallingCall = falling;

	/* Set Alternate Function */
	AFIO -> EXTICR [ 3 ] |= AFIO_EXTICR4_EXTI12_PA;

	/* Enable Rising and Falling edge trigger */
	EXTI -> RTSR |= EXTI_RTSR_RT12;
	EXTI -> FTSR |= EXTI_FTSR_FT12;

	/* Enable EXTI */
	EXTI -> IMR |= EXTI_IMR_IM12;

	/* Link to NVIC */
	NVIC_SetPriority ( EXTI15_10_IRQn, 2 );
	NVIC_EnableIRQ ( EXTI15_10_IRQn );
}

/**
 * @brief Pull open drain line to GND
 */
void OD_Pull ( void )
{
	/* To GND */
	ONEWIRE_GPIO_Port -> BRR |= ONEWIRE_Pin;
}

/**
 * @brief Release open drain line
 */
void OD_Release ( void )
{
	/* Let go */
	ONEWIRE_GPIO_Port -> BSRR |= ONEWIRE_Pin;
}

/**
 * @brief External Interrupt Handler
 */
void EXTI15_10_IRQHandler ( void )
{
	/* Check flag */
	if ( EXTI -> PR & EXTI_PR_PIF12 )
	{
		/* Clear flag */
		EXTI -> PR = EXTI_PR_PIF12;

		/* Check edge */
		if ( ONEWIRE_GPIO_Port -> IDR & ONEWIRE_Pin )
		{
			if ( NULL != RisingCall )
			{
				RisingCall (  );
			}
		}
		else if ( NULL != FallingCall )
		{
			FallingCall (  );
		}
	}
}
