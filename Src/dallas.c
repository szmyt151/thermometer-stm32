/*
 * dallas.c
 *
 *  Created on: 07.02.2018
 *      Author: korne
 */

#include "dallas.h"
#include "onewire.h"
#include "stdio.h"
#include "string.h"

#define DS_ROM_SIZE							8
#define DS_SCRATCHPAD_SIZE					9

#define DS_COMMAND_READROM					0x33
#define DS_COMMAND_READSCRATCHPAD			0xBE
#define DS_COMMAND_SKIPROM					0xCC
#define DS_COMMAND_CONVERTTEMP				0x44

/**
 * ROM storage
 */
uint8_t DS_ROM [ DS_ROM_SIZE ];

/**
 * ROM size
 */
uint8_t DS_ROMSize;

/**
 * Scratchpad storage
 */
uint8_t DS_Scratchpad [ DS_SCRATCHPAD_SIZE ];

/**
 * Scratchpad size
 */
uint8_t DS_ScratchpadSize;

/**
 * Sampling variable
 */
uint8_t DS_IsConversionReady;

/**
 * Last temperature
 */
uint16_t DS_LastTemp;

/**
 * Conversion trigger
 */
uint8_t DS_Convert;

/**
 * Dallas library state
 */
static enum
{
	DS_State_Init_Reset,
	DS_State_Init_ReadROM,
	DS_State_Init_ReadScratchpad,
	DS_State_Reset,
	DS_State_SkipROM,
	DS_State_ConvertTemp,
	DS_State_ResetToScratchpad,
	DS_State_SkipToScratchpad,
	DS_State_ReadScratchpad,
	DS_State_Ready,
}	state;

static uint8_t DS_VerifyCRC ( uint8_t* data, uint16_t size, uint8_t crc );
static uint8_t DS_UpdateCRC ( uint8_t shr, uint8_t data );
static void DS_ReadROM ( void );
static void DS_DoInit_ReadROM ( void );
static void DS_DoInit_ReadScratchpad ( void );
static void DS_StateMachine ( void );
static void DS_EndReadout ( void );

/**
 * @brief Initializes dallas library
 */
void DS_Init (  )
{
	/* Clear last temperature */
	DS_LastTemp = 0xFFFF;

	/* Init and reset */
	state = DS_State_Init_Reset;
	OW_Reset (  );
}

/**
 * @brief Clears ROM data
 */
static void DS_ClearROM ( void )
{
	memset ( DS_ROM, 0xFF, DS_ROM_SIZE );
}

/**
 * @brief Reads ROM during initialization
 */
static void DS_DoInit_ReadROM ( void )
{
	/* If not finished gathering ROM data, continue */
	if ( DS_ROMSize < DS_ROM_SIZE )
	{
		OW_ReadByte ( &DS_ROM [ DS_ROMSize++ ] );
	}
	/* If the data are valid, jump to scratchpad readout */
	else if ( DS_VerifyCRC ( DS_ROM, DS_ROMSize - 1, DS_ROM [ DS_ROMSize - 1 ] ) )
	{
		state = DS_State_Init_ReadScratchpad;
		DS_ScratchpadSize = 0;
		OW_WriteByte ( DS_COMMAND_READSCRATCHPAD );
	}
	/* Else reset, and read again */
	else
	{
		state = DS_State_Init_Reset;
		OW_Reset (  );
	}
}

/**
 * @brief Reads the scratchpad during initialization
 */
static void DS_DoInit_ReadScratchpad ( void )
{
	/* If not finished gathering scratchpad data, continue */
	if ( DS_ScratchpadSize < DS_SCRATCHPAD_SIZE )
	{
		OW_ReadByte ( &DS_Scratchpad [ DS_ScratchpadSize++ ] );
	}
	/* If the data are valid, jump to temperature conversion */
	else if ( DS_VerifyCRC ( DS_Scratchpad, DS_ScratchpadSize - 1, DS_Scratchpad [ DS_ScratchpadSize - 1 ] ) )
	{
		state = DS_State_Reset;
		OW_Reset (  );
	}
	/* Else reset, and read again */
	else
	{
		state = DS_State_Init_Reset;
		OW_Reset (  );
	}
}

/**
 * @brief Performs the Dallas state machine tasks
 */
static void DS_StateMachine ( void )
{
	switch ( state )
	{
		/* Initial reset */
		case DS_State_Init_Reset:
		{
			DS_ReadROM (  );
		}
		break;

		/* Device family and serial number */
		case DS_State_Init_ReadROM:
		{
			DS_DoInit_ReadROM (  );
		}
		break;

		/* Scratchpad */
		case DS_State_Init_ReadScratchpad:
		{
			DS_DoInit_ReadScratchpad (  );
		}
		break;

		/* After reset, skip ROM */
		case DS_State_Reset:
		{
			state = DS_State_SkipROM;
			OW_WriteByte ( DS_COMMAND_SKIPROM );
		}
		break;

		case DS_State_SkipROM:
		{
			state = DS_State_ConvertTemp;
			DS_IsConversionReady = 0;
			OW_WriteByte ( DS_COMMAND_CONVERTTEMP );
		}
		break;

		case DS_State_ConvertTemp:
		{
			/* Read line state until 1 occurs */
			if ( 0 == DS_IsConversionReady )
			{
				OW_ReadByte ( &DS_IsConversionReady );
			}
			else
			{
				/* Data ready, perform reset */
				state = DS_State_ResetToScratchpad;
				OW_Reset (  );
			}
		}
		break;

		case DS_State_ResetToScratchpad:
		{
			/* No ROM command, just reading scratchpad */
			state = DS_State_SkipToScratchpad;
			OW_WriteByte ( DS_COMMAND_SKIPROM );
		}
		break;

		case DS_State_SkipToScratchpad:
		{
			/* Reset scratchpad data counter and start readout */
			state = DS_State_ReadScratchpad;
			DS_ScratchpadSize = 0;
			OW_WriteByte ( DS_COMMAND_READSCRATCHPAD );
		}
		break;

		/* Scratchpad readout */
		case DS_State_ReadScratchpad:
		{
			/* If not finished gathering scratchpad data, continue */
			if ( DS_ScratchpadSize < DS_SCRATCHPAD_SIZE )
			{
				OW_ReadByte ( &DS_Scratchpad [ DS_ScratchpadSize++ ] );
			}
			/* If the data are valid, finish readout */
			else if ( DS_VerifyCRC ( DS_Scratchpad, DS_ScratchpadSize - 1, DS_Scratchpad [ DS_ScratchpadSize - 1 ] ) )
			{
				DS_EndReadout (  );
			}
			/* Else reset, and read again */
			else
			{
				state = DS_State_ResetToScratchpad;
				OW_Reset (  );
			}
		}
		break;

		/* If ready and conversion command, launch conversion */
		case DS_State_Ready:
		{
			if ( DS_Convert )
			{
				/* Start conversion procedure */
				state = DS_State_Reset;
				OW_Reset (  );
			}
		}
		break;

		default:
			break;
	}
}

/**
 * @brief Executes all DS tasks
 */
void DS_DoWork ( void )
{
	/* Ensure, that last operation is completed */
	if ( OW_IsReady (  ) )
	{
		/* Ensure, that the device is connected */
		if ( OW_IsPresent (  ) )
		{
			/* Enable led if device is present */
			HAL_GPIO_WritePin ( LED1_GPIO_Port, LED1_Pin, 1 );

			/* Enter state machine */
			DS_StateMachine (  );
		}
		else
		{
			/* Disable led if device not present */
			HAL_GPIO_WritePin ( LED1_GPIO_Port, LED1_Pin, 0 );

			/* Clear info about device */
			DS_ClearROM (  );

			/* Restart */
			state = DS_State_Init_Reset;
			OW_Reset (  );
		}
	}

	/* Invoke OneWire bus Watchdog */
	if ( OW_Watchdog (  ) )
	{
		/* Restart */
		state = DS_State_Init_Reset;
		OW_Reset (  );
	}
}

/**
 * @brief Read ROM from dallas device
 */
static void DS_ReadROM ( void )
{
	/* Reset ROM size counter */
	DS_ROMSize = 0;

	/* Switch state to ROM reading */
	state = DS_State_Init_ReadROM;

	/* Send 'READ ROM' Command */
	OW_WriteByte ( DS_COMMAND_READROM );
}

/**
 * @param Prints Dallas sensor ROM content
 */
void DS_PrintROM ( void )
{
	/* If the device is present */
	if ( OW_IsPresent (  ) )
	{
		/* Print the ROM */
		printf ( "Family code:\t%02x\r\n", DS_ROM [ 0 ] );
		printf ( "Serial number:\t%02x%02x%02x%02x%02x%02x\r\n", DS_ROM [ 6 ], DS_ROM [ 5 ], DS_ROM [ 4 ], DS_ROM [ 3 ], DS_ROM [ 2 ], DS_ROM [ 1 ] );
	}
	else
	{
		/* Print error info */
		printf ( "One wire nie podlaczone \r\n" );
	}
}

/**
 * @brief Prints temperature to standard out
 * @param temp Temperature
 * @retval string length
 */
uint8_t DS_PrintTemp ( uint16_t temp )
{
	static const int32_t decimal_radix = 10000;		/* 4 decimal places */
	static const uint16_t sign_bit = 1 << 11;		/* 11 bit contains sign */

	char buf [ 15 ];	/* String buffer */

	uint8_t exit = 0;	/* String padding removal loop exit flag */

	uint8_t size;		/* Size of string */

	int32_t T;			/* Integral temperature part */
	int32_t Tf;			/* Fractional temperature part */

	/* If error bit is on, print error and return */
	if ( temp & ( 1 << 12 ) )
	{
		printf ( "ERR" );
		return ( 3 );
	}

	/* If it is negative, make it 2s complement */
	if ( temp & sign_bit )
	{
		temp |= 0xF000;
	}

	/* Multiply by LSBit value * decimal_radix */
	T = 625 * ( int16_t ) temp;

	/* Get fractional value */
	Tf = T % decimal_radix;

	/* Remove minus sign from fractional part*/
	Tf = Tf < 0 ? -Tf : Tf;

	/* Crop the decimal place */
	T = T / decimal_radix;

	/* Print minus sign, if integral part is 0, but the fractional is not and the temp is negative */
	if ( T == 0 && Tf != 0 && ( temp & sign_bit ) )
	{
		size = snprintf ( buf, sizeof ( buf ) , "-%d.%04u", ( int ) T, ( unsigned int ) Tf );
	}
	/* If non negative and not 0, print plus sign */
	else if ( !( temp & sign_bit ) && temp != 0 )
	{
		size = snprintf ( buf, sizeof ( buf ) , "+%d.%04u", ( int ) T, ( unsigned int ) Tf );
	}
	/* Otherwise, let the snprintf decide ( no sign with 0 ) */
	else
	{
		size = snprintf ( buf, sizeof ( buf ) , "%d.%04u", ( int ) T, ( unsigned int ) Tf );
	}

	/* Remove unnecessary 0 padding */
	while ( !exit )
	{
		/* Stop at dot */
		if ( buf [ size - 1 ] == '.' )
		{
			buf [ size - 1 ] = '\0';
			exit = 1;
		}
		/* Stop at non-zero */
		if ( buf [ size - 1 ] != '0' )
		{
			buf [ size ] = '\0';
			exit = 1;
		}
		/* Decrement size */
		size--;
	}

	/* Print the temperature string */
	printf ( "%s", buf );

	return ( strlen ( buf ) );
}

/**
 * @brief Updates CRC shift register
 * @param shr Shift register value
 * @param data Data to update shift register content
 * @retval New value of shift register
 */
static uint8_t DS_UpdateCRC ( uint8_t shr, uint8_t data )
{
	uint8_t i;

	/* Xor with data */
	shr = shr ^ data;

	/* Iterate over all bits */
	for (i = 0; i < 8; i++)
	{
		/* If LSB */
		if (shr & 0x01)
		{
			/* Shift and XOR according to datasheet */
			shr = (shr >> 1) ^ 0x8C;
		}
		else
		{
			/* Just shift */
			shr >>= 1;
		}
	}
	return ( shr );
}

/**
 * @brief Launches the temperature conversion and returns last conversion result
 * @retval Last conversion result
 */
uint16_t DS_LaunchConversion ( void )
{
	uint16_t T;
	if ( DS_Convert )
	{
		/* Error */
		T = 0x1FFF;
	}
	else
	{
		T = DS_LastTemp;

		/* Return only 12 bits, so the FFFF situation is not possible, therefore we may save it in FLASH */
		T &= 0x0FFF;

		/* Clear last temperature */
		DS_LastTemp = 0xFFFF;
	}

	/* Set conversion flag */
	DS_Convert = 1;

	return ( T );
}

/**
 * @brief Sets state to ready and saves read temperature
 */
static void DS_EndReadout ( void )
{
	state = DS_State_Ready;

	/* LSByte | MSByte shifted left */
	DS_LastTemp = DS_Scratchpad [ 0 ] | ( ( uint16_t ) DS_Scratchpad [ 1 ] << 8 );

	/* Reset trigger flag */
	DS_Convert = 0;
}

/**
 * @brief CRC verification procedure
 * @param data data pointer
 * @param size count of data in buffer
 * @param crc CRC of data in buffer
 * @retval 0 if the CRC is invalid
 */
static uint8_t DS_VerifyCRC ( uint8_t* data, uint16_t size, uint8_t crc )
{
	uint8_t shr = 0;
	uint8_t i;

	/* Calculate CRC out of entire data structure */
	for ( i = 0 ; i < size ; i++ )
	{
		shr = DS_UpdateCRC ( shr, data [ i ] );
	}

	/* Update with packet CRC */
	shr = DS_UpdateCRC ( shr, crc );

	/* State register should contain 0
	 * if the CRC is valid */
	return ( 0 == shr );
}
