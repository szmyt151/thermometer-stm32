/*
 * onewire.c
 *
 *  Created on: 07.02.2018
 *      Author: korne
 */

#include "onewire.h"
#include "opendrain.h"
#include "main.h"

#define OW_TIMER_TICKS_PER_US		2

#define OW_RESET_DURATION			540
#define OW_RESET_WAIT				540
#define OW_DURATION_WRITE0			90
#define OW_RECOVERY_WRITE0			5
#define OW_DURATION_WRITE1			5
#define OW_RECOVERY_WRITE1			90
#define OW_DURATION_READ			2
#define OW_RECOVERY_READ			63
#define OW_DURATION_READ1			( 15 * ticks_per_us )

/**
 * Timer handle
 */
extern TIM_HandleTypeDef htim1;

/**
 * System tick time
 */
extern uint32_t OS_Msec;

/**
 * Bus communication state
 */
static volatile enum
{
	OW_State_Ready,
	OW_State_ResetPulse,
	OW_State_WaitForPresence,
	OW_State_PresenceOk,

	OW_State_Write0,
	OW_State_Write0_Recovery,
	OW_State_Write1,
	OW_State_Write1_Recovery,

	OW_State_Read,
	OW_State_Read_Recovery,
}	state;

/**
 * Presence timestamps
 */
uint16_t start_ts;
uint16_t end_ts;

/**
 * Time scaling
 */
uint32_t ticks_per_us;

/**
 * Result of last reset + presence sequence
 */
uint8_t is_present;

/**
 * Temporary byte storage
 */
uint8_t tx_byte;
uint8_t rx_byte;

/**
 * Reception pointer
 */
uint8_t* rx_ptr;

/**
 * Bit counter
 */
uint8_t bit_cnt;

/**
 * Watchdog timestamp
 */
uint32_t watchdog;

static void OW_StartTimer ( uint16_t duration );
static void OW_StopTimer ( void );
static void OW_PutBit ( void );
static void OW_GetBit ( void );
static void OW_Callback_ExtiFalling ( void );
static void OW_Callback_ExtiRising ( void );

/**
 * @brief Initializes one wire bus
 * @param Number of timer ticks per us
 */
void OW_Init ( void )
{
	/* Assign timer frequency */
	ticks_per_us = OW_TIMER_TICKS_PER_US;

	/* Reset state */
	state = OW_State_Ready;

	/* Init open drain */
	OD_Init ( OW_Callback_ExtiFalling, OW_Callback_ExtiRising );

	/* Release line */
	OD_Release (  );

	/* Assume not present */
	is_present = 0;

	/* Reset watchdog */
	watchdog = OS_Msec;
}

/**
 * @brief Reset device
 */
void OW_Reset ( void )
{
	/* If ready */
	if ( OW_State_Ready == state )
	{
		/* Launch reset */
		state = OW_State_ResetPulse;
		OD_Pull (  );
	}
}

/**
 * @brief Start timer
 * @param Count of us, after which the interrupt will be called
 */
static void OW_StartTimer ( uint16_t duration )
{
	/* Reset counter and clear IT */
	__HAL_TIM_SET_COUNTER ( &htim1, 0 );
	__HAL_TIM_CLEAR_IT ( &htim1, TIM_IT_UPDATE );

	/* Set autoreload register to delay value */
	__HAL_TIM_SET_AUTORELOAD ( &htim1, duration * ticks_per_us );

	/* Data and instruction synchronisation barrier */
	__DSB();
	__ISB();

	/* Start the timer */
	HAL_TIM_Base_Start_IT ( &htim1 );
}

/**
 * @brief Stop timer
 */
static void OW_StopTimer ( void )
{
	/* Stop the timer */
	HAL_TIM_Base_Stop_IT ( &htim1 );
}

/**
 * @brief Check, if the device is ready
 * @retval 0, if the device is busy
 */
uint8_t OW_IsReady ( void )
{
	return ( OW_State_Ready == state );
}

/**
 * @brief Result of last reset operation
 * @retval 0 if there is no device on the bus
 */
uint8_t OW_IsPresent ( void )
{
	return ( is_present != 0 );
}

/**
 * @brief Timer update interrupt
 */
void OW_Callback_TimerEnd ( void )
{
	/* Stop the timer */
	OW_StopTimer (  );

	/* State machine */
	switch ( state )
	{
		case OW_State_ResetPulse:
		{
			state = OW_State_WaitForPresence;
			OD_Release (  );
		}
		break;

		case OW_State_PresenceOk:
		{
			is_present = 1;
			state = OW_State_Ready;
		}
		break;

		case OW_State_WaitForPresence:
		{
			is_present = 0;
			state = OW_State_Ready;
		}
		break;

		case OW_State_Write0:
		{
			state = OW_State_Write0_Recovery;
			OD_Release (  );
		}
		break;

		case OW_State_Write1:
		{
			state = OW_State_Write1_Recovery;
			OD_Release (  );
		}
		break;

		case OW_State_Write0_Recovery:
		case OW_State_Write1_Recovery:
		{
			OW_PutBit (  );
		}
		break;

		case OW_State_Read:
		{
			state = OW_State_Read_Recovery;
			OW_StartTimer ( OW_RECOVERY_READ );
			OD_Release (  );
		}
		break;

		case OW_State_Read_Recovery:
		{
			if ( end_ts < OW_DURATION_READ1 )
			{
				rx_byte |= 1 << bit_cnt;
			}
			bit_cnt++;

			OW_GetBit (  );
		}
		break;

		default:
			break;
	}

	/* Reset watchdog */
	watchdog = OS_Msec;
}

/**
 * @brief Falling edge callback
 */
static void OW_Callback_ExtiFalling ( void )
{
	/* State machine */
	switch ( state )
	{
		case OW_State_ResetPulse:
		{
			OW_StartTimer ( OW_RESET_DURATION );
		}
		break;

		case OW_State_WaitForPresence:
		{
			start_ts = __HAL_TIM_GET_COUNTER ( &htim1 );
			state = OW_State_PresenceOk;
		}
		break;

		case OW_State_Write0:
		{
			OW_StartTimer ( OW_DURATION_WRITE0 );
		}
		break;

		case OW_State_Write1:
		{
			OW_StartTimer ( OW_DURATION_WRITE1 );
		}
		break;

		case OW_State_Read:
		{
			OW_StartTimer ( OW_DURATION_READ );
		}
		break;

		default:
			break;
	}

	/* Reset watchdog */
	watchdog = OS_Msec;
}

/**
 * @brief Rising edge callback
 */
static void OW_Callback_ExtiRising ( void )
{
	/* State machine */
	switch ( state )
	{
		case OW_State_WaitForPresence:
		{
			OW_StartTimer ( OW_RESET_WAIT );
		}
		break;

		case OW_State_PresenceOk:
		{
			end_ts = __HAL_TIM_GET_COUNTER ( &htim1 );
		}
		break;

		case OW_State_Write0_Recovery:
		{
			OW_StartTimer ( OW_RECOVERY_WRITE0 );
		}
		break;

		case OW_State_Write1_Recovery:
		{
			OW_StartTimer ( OW_RECOVERY_WRITE1 );
		}
		break;

		case OW_State_Read_Recovery:
		{
			end_ts = __HAL_TIM_GET_COUNTER ( &htim1 );
		}
		break;

		default:
			break;
	}

	/* Reset watchdog */
	watchdog = OS_Msec;
}

/**
 * @brief Put next bit
 */
static void OW_PutBit ( void )
{
	/* If not finished... */
	if ( bit_cnt < 8 )
	{
		/* Set state according to next bit */
		if ( tx_byte & ( 1 << bit_cnt ) )
		{
			state = OW_State_Write1;
		}
		else
		{
			state = OW_State_Write0;
		}

		/* Go to next bit and start transmission */
		bit_cnt++;
		OD_Pull (  );
	}
	else
	{
		/* Mark as ready */
		state = OW_State_Ready;
	}
}

/**
 * @brief Get next bit
 */
static void OW_GetBit ( void )
{
	/* If not finished... */
	if ( bit_cnt < 8 )
	{
		/* Reset end timestamp */
		end_ts = 0;

		/* Set state to read */
		state = OW_State_Read;

		/* Start read slot */
		OD_Pull (  );
	}
	else
	{
		/* Output the byte, mark as ready */
		*rx_ptr = rx_byte;
		state = OW_State_Ready;
	}
}

/**
 * @brief Write single byte to one wire bus
 * @param Byte to be sent
 */
void OW_WriteByte ( uint8_t byte )
{
	/**
	 * If the bus is ready
	 */
	if ( OW_State_Ready == state )
	{
		/* Init state variables */
		tx_byte = byte;
		bit_cnt = 0;

		/* Start transmission */
		OW_PutBit (  );
	}
}

/**
 * @brief Reads single byte from one wire bus
 * @param Pointer to output memory
 */
void OW_ReadByte ( uint8_t* byte )
{
	/* If the bus is ready */
	if ( OW_State_Ready == state )
	{
		/* Init state variables */
		bit_cnt = 0;
		rx_byte = 0;
		rx_ptr = byte;

		/* Start reception */
		OW_GetBit (  );
	}
}

/**
 * @brief Called periodically monitors the behavior of OneWire bus and resets if necessary
 * @retval 1 if watchdog was triggered
 */
uint8_t OW_Watchdog ( void )
{
	if ( OW_State_Ready == state )
	{
		watchdog = OS_Msec;
	}
	else if ( GetTimeDiff ( watchdog ) > 100 )
	{
		OW_Init (  );
		return ( 1 );
	}
	return ( 0 );
}
