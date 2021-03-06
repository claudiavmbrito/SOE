
#ifndef SOE_RELSCAN_H
#define SOE_RELSCAN_H

#include "storage/soe_bufmgr.h"
#include "storage/soe_ost_bufmgr.h"
#include "access/soe_skey.h"
#include "access/soe_itup.h"
#include "access/soe_htup.h"

/*
 * We use the same IndexScanDescData structure for both amgettuple-based
 * and amgetbitmap-based index scans.  Some fields are only relevant in
 * amgettuple-based scans.
 */
typedef struct IndexScanDescData
{
	/* scan parameters */
	VRelation indexRelation;	/* index relation descriptor */
	OSTRelation ost;			/* Data to scan with the OST protocol */
	ScanKey		keyData;		/* array of index qualifier descriptors */


	/* index access method's private state */
	void	   *opaque;			/* access-method-specific info */

	/* xs_ctup/xs_cbuf/xs_recheck are valid after a successful index_getnext */
	HeapTupleData xs_ctup;		/* current heap tuple, if any */
	Buffer		xs_cbuf;		/* current heap buffer in scan, if any */

	/* state data for traversing HOT chains in index_getnext */
	bool		xs_continue_hot;	/* T if must keep walking HOT chain */

	unsigned int opoid;
	/* oid of where comparison clause. */
    //Forest ORAM or PathORAM
    Mode mode;


}			IndexScanDescData;


/* struct definitions appear in relscan.h */
typedef struct IndexScanDescData *IndexScanDesc;

#endif              /* SOE_RELSCAN_H*/
