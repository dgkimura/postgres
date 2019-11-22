#include "postgres.h"

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
#include "fmgr.h"

typedef struct ZoitPageOpaqueData
{
	ItemPointerData heapPtr;
} ZoitPageOpaqueData;

typedef ZoitPageOpaqueData *ZoitPageOpaque;


PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(zthandler);

extern bytea *ztoptions(Datum reloptions, bool validate);

bytea *
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

	LockRelationForExtension(index, ExclusiveLock);
	metabuf = ReadBuffer(index, P_NEW);
	LockBuffer(metabuf, BUFFER_LOCK_EXCLUSIVE);

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

	itup = index_form_tuple(RelationGetDescr(indexRelation), values, isnull);
	itup->t_tid = *heap_tid;

	pfree(itup);

	return false;
}

static void
ztcostestimate(PlannerInfo *root, IndexPath *path, double loop_count,
			   Cost *indexStartupCost, Cost *indexTotalCost,
			   Selectivity *indexSelectivity, double *indexCorrelation,
			   double *indexPages)
{
	/* Tell planner to never use this index! */
	*indexStartupCost = 0;
	*indexTotalCost = 0;

	/* Do not care about the rest */
	*indexSelectivity = 1;
	*indexCorrelation = 0;
	*indexPages = 1;
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

	/*
	 * Create an IndexScanDesc
	 */
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
	amroutine->amgettuple = NULL;
	amroutine->amgetbitmap = ztgetbitmap;
	amroutine->amendscan = ztendscan;
	amroutine->ammarkpos = NULL;
	amroutine->amrestrpos = NULL;
	amroutine->amestimateparallelscan = NULL;
	amroutine->aminitparallelscan = NULL;
	amroutine->amparallelrescan = NULL;

	PG_RETURN_POINTER(amroutine);
}
