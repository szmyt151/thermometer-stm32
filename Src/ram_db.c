/*
 * ram_db.c
 *
 *  Created on: 23.02.2018
 *      Author: korne
 */

#include "ram_db.h"
#include "string.h"

#define RDB_DATA_SIZE		4096

static uint16_t RDB_Data [ RDB_DATA_SIZE ];

static uint16_t RDB_Head;
static uint16_t RDB_Tail;
static uint16_t RDB_ReadTail;

static uint16_t RDB_GetNextIdx ( uint16_t idx );

void RDB_Init ( void )
{
	RDB_Head = 0;
	RDB_Tail = 0;
	RDB_ReadTail = 0;
	memset ( RDB_Data, 0xFF, sizeof ( RDB_Data ) );
}

void RDB_RegisterVal ( uint16_t T )
{
	uint16_t new_head = RDB_GetNextIdx ( RDB_Head );
	RDB_Data [ RDB_Head ] = T;
	if ( new_head == RDB_Tail )
	{
		RDB_Tail = RDB_GetNextIdx ( RDB_Tail );
	}
	RDB_Head = new_head;
}

void RDB_ResetReadTail ( void )
{
	RDB_ReadTail = RDB_Tail;
}

uint16_t RDB_ReadData ( void )
{
	uint16_t retval = 0xFFFF;

	if ( RDB_ReadTail != RDB_Head )
	{
		RDB_ReadTail = RDB_GetNextIdx ( RDB_ReadTail );
		retval = RDB_Data [ RDB_ReadTail ];
	}
	return ( retval );
}

uint16_t RDB_GetCountOfSamples ( void )
{
	if ( RDB_Tail <= RDB_Head )
	{
		/* Data not wrapped */
		return ( RDB_Head - RDB_Tail );
	}
	else
	{
		/* Data wrapped around */
		return ( ( ( RDB_DATA_SIZE ) + RDB_Head ) - RDB_Tail );
	}
}

static uint16_t RDB_GetNextIdx ( uint16_t idx )
{
	idx += 1;

	if ( ( RDB_DATA_SIZE ) <= idx )
	{
		idx = 0;
	}
	return ( idx );
}
