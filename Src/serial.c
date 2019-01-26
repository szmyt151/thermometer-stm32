/*
 * serial.c
 *
 *  Created on: 21.02.2018
 *      Author: korne
 */

#include "serial.h"
#include "string.h"

#define S_TX_BUFFER_SIZE		2048

extern UART_HandleTypeDef huart2;

static uint16_t S_Tail = 0;
static uint16_t S_NextTail = 0;
static uint16_t S_Head = 0;

static uint8_t S_TxBuffer [ S_TX_BUFFER_SIZE ];

static uint8_t S_Rx;
static uint8_t S_RxTmp;
static uint8_t S_NewData;

void S_Init ( void )
{
	/**
	 * Set internal buffers to zero, as we do not want to bother with flushes
	 */
	setvbuf ( stdin, NULL, _IONBF, 0 );
	setvbuf ( stdout, NULL, _IONBF, 0 );
	setvbuf ( stderr, NULL, _IONBF, 0 );

	S_Tail = S_TX_BUFFER_SIZE - 1;
	S_NextTail = S_Tail;
	S_Head = 0;

	S_NewData = 0;

	HAL_UART_Receive_IT ( &huart2, &S_Rx, 1 );
}

/**
 * @brief Transmission procedure
 */
void S_TxCall ( void )
{
	unsigned short h = S_Head;
	unsigned short new_t = ( S_TX_BUFFER_SIZE - 1 );

	/**
	 * Restore tail from previous TX call
	 */
	S_Tail = S_NextTail;
	new_t &= ( S_Tail + 1 );

	/**
	 * Select which chunk of data will be transmitted
	 */
	if ( new_t < h )	/* Tail to head */
	{
		HAL_UART_Transmit_DMA ( &huart2, &S_TxBuffer [ new_t ], h - new_t );
		S_NextTail = h - 1;
	}
	else if ( h < new_t )	/* Tail to buffer end */
	{
		HAL_UART_Transmit_DMA ( &huart2, &S_TxBuffer [ new_t ], S_TX_BUFFER_SIZE - new_t );
		S_NextTail = S_TX_BUFFER_SIZE - 1;
	}
}

/**
 * @brief Checks if there are new data
 * @retval 0 if there are no data
 */
uint8_t S_IsNewData ( void )
{
	return ( S_NewData );
}

/**
 * @brief Gets user command
 * @retval Command
 */
uint8_t S_GetCommand ( void )
{
	uint8_t data = S_RxTmp;
	S_NewData = 0;
	return ( data );
}

/**
 * @brief Transmit complete callback
 * @param UART structure
 */
void HAL_UART_TxCpltCallback ( UART_HandleTypeDef *huart )
{
	if ( &huart2 == huart )
	{
		S_TxCall (  );
	}
}

void HAL_UART_RxCpltCallback ( UART_HandleTypeDef *huart )
{
	if ( &huart2 == huart )
	{
		if ( 0 == S_NewData )
		{
			S_RxTmp = S_Rx;
			S_NewData = 1;
		}
		HAL_UART_Receive_IT ( &huart2, &S_Rx, 1 );
	}
}

/**
 * @brief Overloaded write method, so we can printf in our software
 * @param file			Stream indicator
 * @param ptr			Pointer to input buffer
 * @param len			Number of bytes to write
 * @return				Number of data sent
 */
int _write ( int file, char *ptr, int len )
{
	unsigned short idx = 0;

	/**
	 * If the target is stdout stream:
	 */
	if ( file == 1 )
	{
		/**
		 * Place data in the buffer
		 */
		while ( idx < len )
		{
			/* Wait if overflow */
			while ( S_Head == S_Tail );
			S_TxBuffer [ S_Head ] = ptr [ idx++ ];
			S_Head = ( S_Head + 1 ) & ( S_TX_BUFFER_SIZE - 1 );
		}

		/**
		 * If the transmission is finished, trigger manually
		 */
		if ( huart2.gState == HAL_UART_STATE_READY )
		{
			S_TxCall (  );
		}
	}
	return ( idx );
}
