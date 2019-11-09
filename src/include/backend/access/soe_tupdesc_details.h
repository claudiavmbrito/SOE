/*-------------------------------------------------------------------------
 *
 * tupdesc_details.h
 * Bare bones copy of POSTGRES tuple descriptor definitions we can't include
 * everywhere for enclave execution.
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/tupdesc_details.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef SOE_TUPDESC_DETAILS_H
#define SOE_TUPDESC_DETAILS_H

/*
 * Structure used to represent value to be used when the attribute is not
 * present at all in a tuple, i.e. when the column was created after the tuple
 */
typedef struct attrMissing
{
	bool		am_present;		/* true if non-NULL missing value exists */
	Datum		am_value;		/* value when attribute is missing */
}			AttrMissing;

#endif							/* TUPDESC_DETAILS_H */
