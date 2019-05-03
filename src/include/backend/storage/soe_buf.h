/*-------------------------------------------------------------------------
 *
 * soebuf.h
 * Barebones copy of Basic buffer manager data types.
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/buf.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SOE_BUF_H
#define SOE_BUF_H

/*
 * Buffer identifiers.
 *
 * Zero is invalid, positive is the index of a shared buffer (1..NBuffers),
 * negative is the index of a local buffer (-1 .. -NLocBuffer).
 */
typedef int Buffer;

#define InvalidBuffer	0

/*
 * BufferIsInvalid
 *		True iff the buffer is invalid.
 */
#define BufferIsInvalid(buffer) ((buffer) == InvalidBuffer)

/*
 * BufferIsLocal
 *		True iff the buffer is local (not visible to other backends).
 */
#define BufferIsLocal(buffer)	((buffer) < 0)

#endif							/* SOE_BUF_H */
