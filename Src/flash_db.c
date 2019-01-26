/*
 * flash_db.c
 *
 *  Created on: 21.02.2018
 *      Author: korne
 */

#include "flash_db.h"

#define FLASH_PAGE_COUNT		5
#define FLASH_PAGE_ADDR(x) 		( FLASH_BASE + ( ( ( ( x > ( FLASH_PAGE_COUNT - 1 ) ) || ( x < 0 ) ) ? ( 127 - FLASH_PAGE_COUNT ) : ( ( 127 - FLASH_PAGE_COUNT + 1 ) + x ) ) * FLASH_PAGE_SIZE ) )

#define FDB_MEMORY_SIZE			( ( FLASH_PAGE_COUNT * FLASH_PAGE_SIZE ) / sizeof ( *FDB_Data ) )
#define FDB_PAGE_FROM_IDX(i)	( i / ( FLASH_PAGE_SIZE >> 1 ) )
#define FDB_ELEMENT_COUNT		( 2000 ) // 2000

/* Circular buffer indices */
uint16_t FDB_Head = 0;
uint16_t FDB_Tail = 0;
uint16_t FDB_ReadTail = 0;

/* Flash buffer */
uint16_t* FDB_Data = ( uint16_t* ) FLASH_PAGE_ADDR ( 0 );

static uint8_t FDB_ClearPage ( uint8_t page );
static uint8_t FDB_FindHead ( void );
static uint16_t FDB_GetNextIdx ( uint16_t idx );
static void FDB_FindTail ( void );
void FDB_ClearMemory ( void );
static void FDB_ValidateDataCount ( void );
static uint16_t FDB_PopTail ( void );

void FDB_Init ( void )
{
	if ( FDB_FindHead (  ) )
	{
		FDB_Tail = 0;
	}
	else
	{
		FDB_FindTail (  );
		FDB_ValidateDataCount (  );
	}
}

uint8_t FDB_RegisterVal ( uint16_t readout )
{
	uint32_t retval;

	/* Next head */
	FDB_Head = FDB_GetNextIdx ( FDB_Head );

	HAL_FLASH_Unlock (  );

	__disable_irq (  );

	retval = HAL_FLASH_Program ( FLASH_TYPEPROGRAM_HALFWORD, ( uint32_t ) &FDB_Data [ FDB_Head ], readout );

	__enable_irq (  );

	HAL_FLASH_Lock (  );

	FDB_ValidateDataCount (  );

	if ( HAL_OK != retval )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint16_t FDB_GetCountOfSamples ( void )
{
	if ( FDB_Tail <= FDB_Head )
	{
		/* Data not wrapped */
		return ( FDB_Head - FDB_Tail );
	}
	else
	{
		/* Data wrapped around */
		return ( ( ( FDB_MEMORY_SIZE ) + FDB_Head ) - FDB_Tail );
	}
}

void FDB_ResetReadTail ( void )
{
FDB_ReadTail = FDB_Tail;
}

uint16_t FDB_ReadData ( void )
{
	uint16_t retval = 0xFFFF;

		/* If length is not NULL */
		if ( FDB_ReadTail != FDB_Head )
		{
			/* Increment tail */
			FDB_ReadTail = FDB_GetNextIdx ( FDB_ReadTail );

			/* Extract current tail value */
			retval = FDB_Data [ FDB_ReadTail ];
		}
		return ( retval );
}

static uint16_t FDB_PopTail ( void )
{
	uint16_t retval = 0xFFFF;

	/* If length is not NULL */
	if ( FDB_Tail != FDB_Head )
	{
		uint16_t new_tail;
		uint8_t page_no;

		/* Extract current tail value */
		retval = FDB_Data [ FDB_Tail ];

		/* Increment tail */
		new_tail = FDB_GetNextIdx ( FDB_Tail );

		/* Get old page number */
		page_no = FDB_PAGE_FROM_IDX ( FDB_Tail );

		/* Move tail */
		FDB_Tail = new_tail;

		/* We have moved tail to new page */
		if ( FDB_PAGE_FROM_IDX ( new_tail ) != page_no )
		{
			if ( FDB_PAGE_FROM_IDX ( FDB_Head ) == page_no )
			{
				/* Failure, clear memory */
				FDB_ClearMemory (  );
			}
			else
			{
				/* Prepare page to next write */
				FDB_ClearPage ( page_no );
			}
		}
	}
	return ( retval );
}

static void FDB_FindTail ( void )
{
	uint16_t new_tail = FDB_Head;

	do
	{
		/* Go to previous value */
		new_tail = new_tail - 1;
		if ( FDB_MEMORY_SIZE <= new_tail )
		{
			new_tail = FDB_MEMORY_SIZE - 1;
		}

		/* Tail is first empty cell */
		if ( 0xFFFF == FDB_Data [ new_tail ] )
		{
			FDB_Tail = new_tail;
			return;
		}
	}
	while ( new_tail != FDB_Head );	/* Loop until head */

	/* We have no tail, entire memory is occupied by data */
	/* Recover from failure */
	FDB_ClearMemory (  );
}

static uint8_t FDB_FindHead ( void )
{
	uint16_t i;
	enum
	{
		State_Empty,
		State_Data,
	}	state = State_Empty;

	/* Iterate over entire buffer */
	for ( i = 0 ; i < FDB_MEMORY_SIZE ; i++ )
	{
		if ( 0xFFFF == FDB_Data [ i ] )
		{
			/* Empty cell */
			if ( State_Data == state )
			{
				FDB_Head = ( i - 1 );
				return 0;
			}
		}
		/* If cell not empty and state empty, skip data sector */
		else if ( State_Empty == state)
		{
			state = State_Data;
		}
	}

	switch ( state )
	{
		case State_Empty:
		{
			FDB_Head = 0;
			return 1;
		}
		break;

		case State_Data:
		{
			FDB_Head = ( FDB_MEMORY_SIZE ) - 1;
		}
		break;
	}
	return 0;
}

static void FDB_ValidateDataCount ( void )
{
	/* Pop all until proper number of elements remains */
	while ( FDB_ELEMENT_COUNT < FDB_GetCountOfSamples (  ) )
	{
		FDB_PopTail (  );
	}
}

 void FDB_ClearMemory ( void )
{
	uint16_t i;

	/* Clear all pages */
	for ( i = 0 ; i < FLASH_PAGE_COUNT ; i++ )
	{
		FDB_ClearPage ( i );
	}

	/* Reset tail and head */
	FDB_Tail = 0;
	FDB_Head = 0;
}

static uint16_t FDB_GetNextIdx ( uint16_t idx )
{
	idx += 1;

	if ( ( FDB_MEMORY_SIZE ) <= idx )
	{
		idx = 0;
	}
	return ( idx );
}

static uint8_t FDB_ClearPage ( uint8_t page )
{
	FLASH_EraseInitTypeDef erase;
	uint32_t err = 0;
	uint32_t retval;

	/* Fill erase structure */
	erase.Banks = FLASH_BANK_1;
	erase.NbPages = 1;
	erase.PageAddress = FLASH_PAGE_ADDR ( page );
	erase.TypeErase = FLASH_TYPEERASE_PAGES;

	/* Do erase with flash unlocking */
	HAL_FLASH_Unlock (  );

	__disable_irq (  );

	retval = HAL_FLASHEx_Erase( &erase, &err );

	__enable_irq (  );

	HAL_FLASH_Lock (  );

	if ( HAL_OK != retval )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

