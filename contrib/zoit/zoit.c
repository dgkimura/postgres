#include "postgres.h"

#include "access/hash.h"
#include "access/relscan.h"
#include "utils/index_selfuncs.h"

#include "access/amapi.h"
#include "access/generic_xlog.h"
#include "common/relpath.h"
#include "nodes/pathnodes.h"
#include "nodes/execnodes.h"
#include "optimizer/plancat.h"
#include "storage/block.h"
#include "storage/bufmgr.h"
#include "storage/smgr.h"
#include "storage/lmgr.h"
#include "utils/rel.h"
#include "utils/selfuncs.h"
#include "fmgr.h"

typedef struct ZoitPageOpaqueData
{
	ItemPointerData heapPtr;
} ZoitPageOpaqueData;

typedef ZoitPageOpaqueData *ZoitPageOpaque;

#define ZOIT_PAGE 0

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(zthandler);

static bytea *
ztoptions(Datum reloptions, bool validate)
{
	return NULL;
}

/*
 * build index from a heap relation that potentially has data.
 */
static IndexBuildResult *
ztbuild(Relation heap, Relation index, IndexInfo *indexInfo)
{
	IndexBuildResult *result;
	Buffer metabuf;
	Page page;
	GenericXLogState *state;
	char *contents;

	/* Must extend the file */
	LockRelationForExtension(index, ExclusiveLock);

	metabuf = ReadBuffer(index, P_NEW);
	LockBuffer(metabuf, BUFFER_LOCK_EXCLUSIVE);

	UnlockRelationForExtension(index, ExclusiveLock);

	state = GenericXLogStart(index);
	page = GenericXLogRegisterBuffer(state, metabuf,
									 GENERIC_XLOG_FULL_IMAGE);

	PageInit(page, BLCKSZ, sizeof(struct ZoitPageOpaqueData));
	contents = PageGetContents(page);
	memset(contents, 0, sizeof(struct ZoitPageOpaqueData));

	GenericXLogFinish(state);
	UnlockReleaseBuffer(metabuf);

	result = (IndexBuildResult *) palloc(sizeof(IndexBuildResult));
	result->heap_tuples = 0;
	result->index_tuples = 0;
	return result;
}

/*
 * build empty index.
 */
static void
ztbuildempty(Relation index)
{
}

static bool
ztinsert(Relation indexRelation, Datum *values, bool *isnull,
		 ItemPointer heap_tid, Relation heapRelation,
		 IndexUniqueCheck checkUnique,
		 IndexInfo *indexInfo)
{
	IndexTuple itup;
	Buffer metabuf;
	Page page;
	PageHeader pageHeader;
	ZoitPageOpaqueData *zpage;
	GenericXLogState *state;

	itup = index_form_tuple(RelationGetDescr(indexRelation), values, isnull);
	itup->t_tid = *heap_tid;

	metabuf = ReadBuffer(indexRelation, ZOIT_PAGE);
	LockBuffer(metabuf, BUFFER_LOCK_EXCLUSIVE);

	state = GenericXLogStart(indexRelation);
	page = GenericXLogRegisterBuffer(state, metabuf, GENERIC_XLOG_FULL_IMAGE);

	zpage = (ZoitPageOpaqueData *)PageGetContents(page);
	zpage->heapPtr = itup->t_tid;

	pageHeader = (PageHeader)page;
	pageHeader->pd_lower = sizeof(PageHeaderData) + sizeof(ZoitPageOpaqueData);

	GenericXLogFinish(state);
	UnlockReleaseBuffer(metabuf);

	pfree(itup);

	return false;
}

static void
ztcostestimate(PlannerInfo *root, IndexPath *path, double loop_count,
			   Cost *indexStartupCost, Cost *indexTotalCost,
			   Selectivity *indexSelectivity, double *indexCorrelation,
			   double *indexPages)
{
	GenericCosts costs;

	MemSet(&costs, 0, sizeof(costs));

	genericcostestimate(root, path, loop_count, &costs);

	*indexStartupCost = costs.indexStartupCost;
	*indexTotalCost = costs.indexTotalCost;
	*indexSelectivity = costs.indexSelectivity;
	*indexCorrelation = costs.indexCorrelation;
	*indexPages = costs.numIndexPages;
}

static int64
ztgetbitmap(IndexScanDesc scan, TIDBitmap *tbm)
{
	return 0;
}

static IndexScanDesc
ztbeginscan(Relation r, int nkeys, int norderbys)
{
	IndexScanDesc scan;

	scan = RelationGetIndexScan(r, nkeys, norderbys);

	return scan;
}

static void
ztrescan(IndexScanDesc scan, ScanKey scankey, int nscankeys,
		 ScanKey orderbys, int norderbys)
{
}

static void
ztendscan(IndexScanDesc scan)
{
}

static IndexBulkDeleteResult *
ztvacuumcleanup(IndexVacuumInfo *info, IndexBulkDeleteResult *stats)
{
	return NULL;
}

static bool
ztgettuple(IndexScanDesc scan, ScanDirection dir)
{
	Buffer metabuf;
	Page page;
	ZoitPageOpaqueData *zpage;

	static bool first = true;

	metabuf = ReadBuffer(scan->indexRelation, ZOIT_PAGE);
	LockBuffer(metabuf, BUFFER_LOCK_SHARE);

	page = BufferGetPage(metabuf);
	zpage = (ZoitPageOpaqueData *)PageGetContents(page);

	scan->xs_heaptid = zpage->heapPtr;

	/* let's pretend we're not lossy.. */
	scan->xs_recheck = false;

	UnlockReleaseBuffer(metabuf);

	if (first)
	{
		first = false;
		return true;
	}
	return false;

	//return true;
}

Datum
zthandler(PG_FUNCTION_ARGS)
{
	IndexAmRoutine *amroutine = makeNode(IndexAmRoutine);

	amroutine->amstrategies = 0;
	amroutine->amsupport = 1;
	amroutine->amcanorder = false;
	amroutine->amcanorderbyop = false;
	amroutine->amcanbackward = false;
	amroutine->amcanunique = false;
	amroutine->amcanmulticol = true;
	amroutine->amoptionalkey = true;
	amroutine->amsearcharray = false;
	amroutine->amsearchnulls = false;
	amroutine->amstorage = false;
	amroutine->amclusterable = false;
	amroutine->ampredlocks = false;
	amroutine->amcanparallel = false;
	amroutine->amcaninclude = false;
	amroutine->amkeytype = InvalidOid;

	amroutine->ambuild = ztbuild;
	amroutine->ambuildempty = ztbuildempty;
	amroutine->aminsert = ztinsert;
	amroutine->ambulkdelete = NULL;
	amroutine->amvacuumcleanup = ztvacuumcleanup;
	amroutine->amcanreturn = NULL;
	amroutine->amcostestimate = ztcostestimate;
	amroutine->amoptions = ztoptions;
	amroutine->amproperty = NULL;
	amroutine->ambuildphasename = NULL;
	amroutine->amvalidate = NULL;
	amroutine->ambeginscan = ztbeginscan;
	amroutine->amrescan = ztrescan;
	amroutine->amgettuple = ztgettuple;
	amroutine->amgetbitmap = ztgetbitmap;
	amroutine->amendscan = ztendscan;
	amroutine->ammarkpos = NULL;
	amroutine->amrestrpos = NULL;
	amroutine->amestimateparallelscan = NULL;
	amroutine->aminitparallelscan = NULL;
	amroutine->amparallelrescan = NULL;

	PG_RETURN_POINTER(amroutine);
}
