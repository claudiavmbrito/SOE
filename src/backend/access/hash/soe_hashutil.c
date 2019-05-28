/*-------------------------------------------------------------------------
 *
 * hashutil.c
 *	  Utility code for Postgres hash implementation.
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/access/hash/hashutil.c
 *
 *-------------------------------------------------------------------------
 */


#include "access/soe_hash.h"
#include "logger/logger.h"

#define CALC_NEW_BUCKET_s(old_bucket, lowmask) \
			old_bucket | (lowmask + 1)



/*
 * _hash_datum2hashkey -- given a Datum, call the index's hash function
 *
 * The Datum is assumed to be of the index's column type, so we can use the
 * "primary" hash function that's tracked for us by the generic index code.
 */
uint32
_hash_datum2hashkey_s(VRelation rel, const char* datum, unsigned int datumSize)
{

	Datum		result;

	/* 
	 * For the prototype we choose the function to hash the datum
	 * based on the foid defined when the soe is initialized.
	 * On the final version of the prototype this function will
	 * be a secure hash function.
	 */
	//Use the function hashbpchar which uses the hash_any
	if(rel->foid == 1080)
	{
		result = hash_any_s((unsigned char *) datum, datumSize);

	}else{
		result = -1;
		selog(ERROR, "invalid function oid, can't hash tuple");
	}


	return DatumGetUInt32_s(result);


	//FmgrInfo   *procinfo;
	//Oid			collation;

	/* XXX assumes index has only one attribute */
	//procinfo = index_getprocinfo(rel, 1, HASHSTANDARD_PROC);
	//collation = rel->rd_indcollation[0];

	//return DatumGetUInt32(FunctionCall1Coll(procinfo, collation, key));
	/**
	 * TODO: Since this code runs inside the enclave and we don't know which
	 * function must be used to create an hash key we will have to hard-code
	 * it in the enclave for now. 
	 */
	return 0;
}

/*
 * _hash_datum2hashkey_type -- given a Datum of a specified type,
 *			hash it in a fashion compatible with this index
 *
 * This is much more expensive than _hash_datum2hashkey, so use it only in
 * cross-type situations.
 */
uint32
_hash_datum2hashkey_type_s(VRelation rel, Datum key, Oid keytype)
{
//	RegProcedure hash_proc;
//	Oid			collation;
	/*TODO: Function will be hardcoded to accepted data types.*/
	/* XXX assumes index has only one attribute */
	/*hash_proc = get_opfamily_proc(rel->rd_opfamily[0],
								  keytype,
								  keytype,
								  HASHSTANDARD_PROC);
	if (!RegProcedureIsValid(hash_proc))
		elog(ERROR, "missing support function %d(%u,%u) for index \"%s\"",
			 HASHSTANDARD_PROC, keytype, keytype,
			 RelationGetRelationName(rel));
	collation = rel->rd_indcollation[0];

	return DatumGetUInt32(OidFunctionCall1Coll(hash_proc, collation, key));*/

	//TODO: FIX THIS
	return 0;
}

/*
 * _hash_hashkey2bucket -- determine which bucket the hashkey maps to.
 */
Bucket
_hash_hashkey2bucket_s(uint32 hashkey, uint32 maxbucket,
					 uint32 highmask, uint32 lowmask)
{
	Bucket		bucket;

	bucket = hashkey & highmask;
	if (bucket > maxbucket)
		bucket = bucket & lowmask;

	return bucket;
}

/*
 * _hash_log2 -- returns ceil(lg2(num))
 */
uint32
_hash_log2_s(uint32 num)
{
	uint32		i,
				limit;

	limit = 1;
	for (i = 0; limit < num; limit <<= 1, i++)
		;
	return i;
}

/*
 * _hash_spareindex -- returns spare index / global splitpoint phase of the
 *					   bucket
 */
uint32
_hash_spareindex_s(uint32 num_bucket)
{
	uint32		splitpoint_group;
	uint32		splitpoint_phases;

	splitpoint_group = _hash_log2_s(num_bucket);

	if (splitpoint_group < HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE)
		return splitpoint_group;

	/* account for single-phase groups */
	splitpoint_phases = HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE;

	/* account for multi-phase groups before splitpoint_group */
	splitpoint_phases +=
		((splitpoint_group - HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE) <<
		 HASH_SPLITPOINT_PHASE_BITS);

	/* account for phases within current group */
	splitpoint_phases +=
		(((num_bucket - 1) >>
		  (splitpoint_group - (HASH_SPLITPOINT_PHASE_BITS + 1))) &
		 HASH_SPLITPOINT_PHASE_MASK);	/* to 0-based value. */

	return splitpoint_phases;
}

/*
 *	_hash_get_totalbuckets -- returns total number of buckets allocated till
 *							the given splitpoint phase.
 */
uint32
_hash_get_totalbuckets_s(uint32 splitpoint_phase)
{
	uint32		splitpoint_group;
	uint32		total_buckets;
	uint32		phases_within_splitpoint_group;

	if (splitpoint_phase < HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE)
		return (1 << splitpoint_phase);

	/* get splitpoint's group */
	splitpoint_group = HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE;
	splitpoint_group +=
		((splitpoint_phase - HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE) >>
		 HASH_SPLITPOINT_PHASE_BITS);

	/* account for buckets before splitpoint_group */
	total_buckets = (1 << (splitpoint_group - 1));

	/* account for buckets within splitpoint_group */
	phases_within_splitpoint_group =
		(((splitpoint_phase - HASH_SPLITPOINT_GROUPS_WITH_ONE_PHASE) &
		  HASH_SPLITPOINT_PHASE_MASK) + 1); /* from 0-based to 1-based */
	total_buckets +=
		(((1 << (splitpoint_group - 1)) >> HASH_SPLITPOINT_PHASE_BITS) *
		 phases_within_splitpoint_group);

	return total_buckets;
}

/*
 * _hash_checkpage -- sanity checks on the format of all hash pages
 *
 * If flags is not zero, it is a bitwise OR of the acceptable page types
 * (values of hasho_flag & LH_PAGE_TYPE).
 */
void
_hash_checkpage_s(VRelation rel, Buffer buf, int flags)
{
	Page		page = BufferGetPage_s(rel, buf);

	/*
	 * ReadBuffer verifies that every newly-read page passes
	 * PageHeaderIsValid, which means it either contains a reasonably sane
	 * page header or is all-zero.  We have to defend against the all-zero
	 * case, however.
	 */
	if (PageIsNew_s(page))
		selog(ERROR, "index contains unexpected zero page at block %u",
						BufferGetBlockNumber_s(buf));
		/*TODO: error messages*/
		/*ereport(ERROR,
				(errcode(ERRCODE_INDEX_CORRUPTED),
				 errmsg("index \"%s\" contains unexpected zero page at block %u",
						RelationGetRelationName(rel),
						BufferGetBlockNumber(buf)),
				 errhint("Please REINDEX it.")));*/

	/*
	 * Additionally check that the special area looks sane.
	 */
	if (PageGetSpecialSize_s(page) != MAXALIGN_s(sizeof(HashPageOpaqueData)))
		selog(ERROR, "1-index contains corrupted page at block");
		/*TODO: error messages*/
		/*ereport(ERROR,
				(errcode(ERRCODE_INDEX_CORRUPTED),
				 errmsg("index \"%s\" contains corrupted page at block %u",
						RelationGetRelationName(rel),
						BufferGetBlockNumber(buf)),
				 errhint("Please REINDEX it.")));*/

	if (flags)
	{
		HashPageOpaque opaque = (HashPageOpaque) PageGetSpecialPointer_s(page);

		if ((opaque->hasho_flag & flags) == 0)
			selog(ERROR, "2-index contains corrupted page at block");

			/*TODO: error messages*/
			/*ereport(ERROR,
					(errcode(ERRCODE_INDEX_CORRUPTED),
					 errmsg("index \"%s\" contains corrupted page at block %u",
							RelationGetRelationName(rel),
							BufferGetBlockNumber(buf)),
					 errhint("Please REINDEX it.")));*/
	}

	/*
	 * When checking the metapage, also verify magic number and version.
	 */
	if (flags == LH_META_PAGE)
	{
		HashMetaPage metap = HashPageGetMeta_s(page);

		if (metap->hashm_magic != HASH_MAGIC)
			selog(ERROR, "2-index is not a hash index");

			/*TODO: error messages*/
			/*ereport(ERROR,
					(errcode(ERRCODE_INDEX_CORRUPTED),
					 errmsg("index \"%s\" is not a hash index",
							RelationGetRelationName(rel))));*/

		if (metap->hashm_version != HASH_VERSION)
			selog(ERROR, "2-index has a wrong hash version");

			/*TODO: error messages*/
			/*ereport(ERROR,
					(errcode(ERRCODE_INDEX_CORRUPTED),
					 errmsg("index \"%s\" has wrong hash version",
							RelationGetRelationName(rel)),
					 errhint("Please REINDEX it.")));*/
	}
}


/*
 * _hash_get_indextuple_hashkey - get the hash index tuple's hash key value
 */
uint32
_hash_get_indextuple_hashkey_s(IndexTuple itup)
{
	char	   *attp;

	/*
	 * We assume the hash key is the first attribute and can't be null, so
	 * this can be done crudely but very very cheaply ...
	 */
	attp = (char *) itup + IndexInfoFindDataOffset_s(itup->t_info);
	//Todo: this code has to decrypt the hashkey and only then cast to uint32.
	return *((uint32 *) attp);
}

/*
 * _hash_convert_tuple - convert raw index data to hash key
 *
 * Inputs: values and isnull arrays for the user data column(s)
 * Outputs: values and isnull arrays for the index tuple, suitable for
 *		passing to index_form_tuple().
 *
 * Returns true if successful, false if not (because there are null values).
 * On a false result, the given data need not be indexed.
 *
 * Note: callers know that the index-column arrays are always of length 1.
 * In principle, there could be more than one input column, though we do not
 * currently support that.
 */
bool
_hash_convert_tuple_s(VRelation index,
					const char *datum, unsigned int datumSize,
					Datum *index_values, bool *index_isnull)
{
	uint32		hashkey;

	/*
	 * We do not insert null values into hash indexes.  This is okay because
	 * the only supported search operator is '=', and we assume it is strict.
	 */
	if (datumSize < 0)
		return false;

	hashkey = _hash_datum2hashkey_s(index, datum, datumSize);
	index_values[0] = UInt32GetDatum_s(hashkey);
	index_isnull[0] = false;
	return true;
}

/*
 * _hash_binsearch - Return the offset number in the page where the
 *					 specified hash value should be sought or inserted.
 *
 * We use binary search, relying on the assumption that the existing entries
 * are ordered by hash key.
 *
 * Returns the offset of the first index entry having hashkey >= hash_value,
 * or the page's max offset plus one if hash_value is greater than all
 * existing hash keys in the page.  This is the appropriate place to start
 * a search, or to insert a new item.
 */
OffsetNumber
_hash_binsearch_s(Page page, uint32 hash_value)
{
	OffsetNumber upper;
	OffsetNumber lower;

	/* Loop invariant: lower <= desired place <= upper */
	upper = PageGetMaxOffsetNumber_s(page) + 1;
	lower = FirstOffsetNumber;

	while (upper > lower)
	{
		OffsetNumber off;
		IndexTuple	itup;
		uint32		hashkey;

		off = (upper + lower) / 2;
		//Assert(OffsetNumberIsValid(off));

		itup = (IndexTuple) PageGetItem_s(page, PageGetItemId_s(page, off));
		hashkey = _hash_get_indextuple_hashkey_s(itup);
		if (hashkey < hash_value)
			lower = off + 1;
		else
			upper = off;
	}

	return lower;
}

/*
 * _hash_binsearch_last
 *
 * Same as above, except that if there are multiple matching items in the
 * page, we return the offset of the last one instead of the first one,
 * and the possible range of outputs is 0..maxoffset not 1..maxoffset+1.
 * This is handy for starting a new page in a backwards scan.
 */
OffsetNumber
_hash_binsearch_last_s(Page page, uint32 hash_value)
{
	OffsetNumber upper;
	OffsetNumber lower;

	/* Loop invariant: lower <= desired place <= upper */
	upper = PageGetMaxOffsetNumber_s(page);
	lower = FirstOffsetNumber - 1;

	while (upper > lower)
	{
		IndexTuple	itup;
		OffsetNumber off;
		uint32		hashkey;

		off = (upper + lower + 1) / 2;
		//Assert(OffsetNumberIsValid(off));

		itup = (IndexTuple) PageGetItem_s(page, PageGetItemId_s(page, off));
		hashkey = _hash_get_indextuple_hashkey_s(itup);
		if (hashkey > hash_value)
			upper = off - 1;
		else
			lower = off;
	}

	return lower;
}