#include "postgres.h"

#include "access/amapi.h"
#include "nodes/execnodes.h"
#include "fmgr.h"

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

	result = (IndexBuildResult *) palloc(sizeof(IndexBuildResult));

	/* fill this in with actual data... */
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
	amroutine->aminsert = NULL;
	amroutine->ambulkdelete = NULL;
	amroutine->amvacuumcleanup = NULL;
	amroutine->amcanreturn = NULL;
	amroutine->amcostestimate = NULL;
	amroutine->amoptions = ztoptions;
	amroutine->amproperty = NULL;
	amroutine->ambuildphasename = NULL;
	amroutine->amvalidate = NULL;
	amroutine->ambeginscan = NULL;
	amroutine->amrescan = NULL;
	amroutine->amgettuple = NULL;
	amroutine->amgetbitmap = NULL;
	amroutine->amendscan = NULL;
	amroutine->ammarkpos = NULL;
	amroutine->amrestrpos = NULL;
	amroutine->amestimateparallelscan = NULL;
	amroutine->aminitparallelscan = NULL;
	amroutine->amparallelrescan = NULL;

	PG_RETURN_POINTER(amroutine);
}
