/*-------------------------------------------------------------------------
 *
 * geo_selfuncs.c
 *	  Selectivity routines registered in the operator catalog in the
 *	  "oprrest" and "oprjoin" attributes.
 *
 * Portions Copyright (c) 1996-2020, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/geo_selfuncs.c
 *
 *	XXX These are totally bogus.  Perhaps someone will make them do
 *	something reasonable, someday.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/builtins.h"
#include "utils/geo_decls.h"


#include "access/htup_details.h"
#include "catalog/pg_statistic.h"
#include "nodes/pg_list.h"
#include "optimizer/pathnode.h"
#include "optimizer/optimizer.h"
#include "utils/lsyscache.h"
#include "utils/typcache.h"
#include "utils/selfuncs.h"
#include "utils/rangetypes.h"



 /*
  *	Selectivity functions for geometric operators.  These are bogus -- unless
  *	we know the actual key distribution in the index, we can't make a good
  *	prediction of the selectivity of these operators.
  *
  *	Note: the values used here may look unreasonably small.  Perhaps they
  *	are.  For now, we want to make sure that the optimizer will make use
  *	of a geometric index if one is available, so the selectivity had better
  *	be fairly small.
  *
  *	In general, GiST needs to search multiple subtrees in order to guarantee
  *	that all occurrences of the same key have been found.  Because of this,
  *	the estimated cost for scanning the index ought to be higher than the
  *	output selectivity would indicate.  gistcostestimate(), over in selfuncs.c,
  *	ought to be adjusted accordingly --- but until we can generate somewhat
  *	realistic numbers here, it hardly matters...
  */


  /*
   * Selectivity for operators that depend on area, such as "overlap".
   */

double calculate_range_join_overlap_fraction(
	double* xa,
	double* ya,
	double min_a,
	double max_a,
	double* xb,
	double* yb,
	double min_b,
	double max_b,
	unsigned int countOfElements
);

Datum
areasel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.005);
}

/*
 * Range Overlaps Join Selectivity.
 */
Datum
rangeoverlapsjoinsel(PG_FUNCTION_ARGS)
{
	PlannerInfo* root = (PlannerInfo*)PG_GETARG_POINTER(0);
	Oid         operator = PG_GETARG_OID(1);
	List* args = (List*)PG_GETARG_POINTER(2);
	JoinType    jointype = (JoinType)PG_GETARG_INT16(3);
	SpecialJoinInfo* sjinfo = (SpecialJoinInfo*)PG_GETARG_POINTER(4);
	Oid         collation = PG_GET_COLLATION();

	double      selec = 0.005;

	VariableStatData vardata1;
	VariableStatData vardata2;
	Oid         opfuncoid;
	AttStatsSlot histSlot1, histNumbersSlot1, histBoundsSlot1, histSlot2, histNumbersSlot2, histBoundsSlot2;
	int         numberOfBins1, numberOfBins2;
	int64* histogram1, * histogram2;
	int         i;
	TypeCacheEntry* typcache1 = NULL, * typcache2 = NULL;
	bool        join_is_reversed;


	get_join_variables(root, args, sjinfo,
		&vardata1, &vardata2, &join_is_reversed);

	typcache1 = range_get_typcache(fcinfo, vardata1.vartype);
	typcache2 = range_get_typcache(fcinfo, vardata2.vartype);
	opfuncoid = get_opcode(operator);

	memset(&histSlot1, 0, sizeof(histSlot1));
	memset(&histNumbersSlot1, 0, sizeof(histNumbersSlot1));
	memset(&histBoundsSlot1, 0, sizeof(histBoundsSlot1));
	memset(&histSlot2, 0, sizeof(histSlot2));
	memset(&histNumbersSlot2, 0, sizeof(histNumbersSlot2));
	memset(&histBoundsSlot2, 0, sizeof(histBoundsSlot2));

	/* Can't use the histogram with insecure range support functions */
	if (!statistic_proc_security_check(&vardata1, opfuncoid))
		PG_RETURN_FLOAT8((float8)selec);
	if (!statistic_proc_security_check(&vardata2, opfuncoid))
		PG_RETURN_FLOAT8((float8)selec);

	if (HeapTupleIsValid(vardata1.statsTuple))
	{
		if (!get_attstatsslot(&histSlot1, vardata1.statsTuple,
			STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM,
			InvalidOid, ATTSTATSSLOT_VALUES))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			PG_RETURN_FLOAT8((float8)selec);
		}
			
		if (!get_attstatsslot(&histNumbersSlot1, vardata1.statsTuple,
				STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM,
				InvalidOid, ATTSTATSSLOT_NUMBERS))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			free_attstatsslot(&histSlot1);
			PG_RETURN_FLOAT8((float8)selec);
		}
			
		if (!get_attstatsslot(&histBoundsSlot1, vardata1.statsTuple,
				STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM_BOUNDS,
				InvalidOid, ATTSTATSSLOT_VALUES))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			free_attstatsslot(&histSlot1);
			free_attstatsslot(&histNumbersSlot1);
			
			PG_RETURN_FLOAT8((float8)selec);
		}
	} else {
		ReleaseVariableStats(vardata1);
		ReleaseVariableStats(vardata2);
		PG_RETURN_FLOAT8((float8)selec);
	}
	if (HeapTupleIsValid(vardata2.statsTuple))
	{
		if (!get_attstatsslot(&histSlot2, vardata2.statsTuple,
			STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM,
			InvalidOid, ATTSTATSSLOT_VALUES))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			free_attstatsslot(&histSlot1);
			free_attstatsslot(&histNumbersSlot1);
			free_attstatsslot(&histBoundsSlot1);
			PG_RETURN_FLOAT8((float8)selec);
		}	
		if(!get_attstatsslot(&histNumbersSlot2, vardata2.statsTuple,
			STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM,
			InvalidOid, ATTSTATSSLOT_NUMBERS))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			free_attstatsslot(&histSlot1);
			free_attstatsslot(&histNumbersSlot1);
			free_attstatsslot(&histBoundsSlot1);
			free_attstatsslot(&histSlot2);
			PG_RETURN_FLOAT8((float8)selec);
		}
		if(!get_attstatsslot(&histBoundsSlot2, vardata2.statsTuple,
			STATISTIC_KIND_EQUIWIDTH_RANGE_HISTOGRAM_BOUNDS,
			InvalidOid, ATTSTATSSLOT_VALUES))
		{
			ReleaseVariableStats(vardata1);
			ReleaseVariableStats(vardata2);
			free_attstatsslot(&histSlot1);
			free_attstatsslot(&histNumbersSlot1);
			free_attstatsslot(&histBoundsSlot1);
			free_attstatsslot(&histSlot2);
			free_attstatsslot(&histNumbersSlot2);
			PG_RETURN_FLOAT8((float8)selec);
		}
	} else {
		ReleaseVariableStats(vardata1);
		ReleaseVariableStats(vardata2);
		free_attstatsslot(&histSlot1);
		free_attstatsslot(&histNumbersSlot1);
		free_attstatsslot(&histBoundsSlot1);
		PG_RETURN_FLOAT8((float8)selec);
	}

	numberOfBins1 = histSlot1.nvalues;
	numberOfBins2 = histSlot2.nvalues;
	int64* histogramValues1 = (int64*)histSlot1.values,
		* histogramValues2 = (int64*)histSlot2.values;
	/* fprintf(stderr, "Number of values: %i\n", numberOfBins1);
	fprintf(stderr, "Histogram = [");
	for (i = 0; i < numberOfBins1; i++) {
		fprintf(stderr, "%i", histogramValues1[i]);
		if (i < numberOfBins1 - 1) fprintf(stderr, ", ");
	}
	fprintf(stderr, "]\n");
	fprintf(stderr, "Number of values: %i\n", numberOfBins2);
	fprintf(stderr, "Histogram = [");
	for (i = 0; i < numberOfBins2; i++) {
		fprintf(stderr, "%i", histogramValues2[i]);
		if (i < numberOfBins2 - 1) fprintf(stderr, ", ");
	}
	fprintf(stderr, "]\n"); */

	Datum* histogramBounds1 = histBoundsSlot1.values[0],
		* histogramBounds2 = histBoundsSlot2.values[0];

	bool empty1, empty2;
	RangeBound* histogram1LowerBound, * histogram1UpperBound,
		* histogram2LowerBound, * histogram2UpperBound;

	histogram1LowerBound = (RangeBound*)palloc(sizeof(RangeBound));
	histogram1UpperBound = (RangeBound*)palloc(sizeof(RangeBound));
	histogram2LowerBound = (RangeBound*)palloc(sizeof(RangeBound));
	histogram2UpperBound = (RangeBound*)palloc(sizeof(RangeBound));

	range_deserialize(typcache1, DatumGetRangeTypeP(histogramBounds1),
		histogram1LowerBound, histogram1UpperBound, &empty1);
	range_deserialize(typcache2, DatumGetRangeTypeP(histogramBounds2),
		histogram2LowerBound, histogram2UpperBound, &empty2);

	fprintf(stderr, "Joining two histograms: [%i, %i] && [%i, %i]\n"
		, histogram1LowerBound->val, histogram1UpperBound->val
		, histogram2LowerBound->val, histogram2UpperBound->val);


	float8* histogram1x = (float8*)palloc(numberOfBins1 * sizeof(float8));
	float8* histogram1y = (float8*)palloc(numberOfBins1 * sizeof(float8));
	float8* histogram2x = (float8*)palloc(numberOfBins2 * sizeof(float8));
	float8* histogram2y = (float8*)palloc(numberOfBins2 * sizeof(float8));
	float4 binWidth1 = histNumbersSlot1.numbers[0];
	float4 avgBinsPerRange1 = histNumbersSlot1.numbers[1];
	float4 histogramWidth1 = histNumbersSlot1.numbers[2];
	float4 binWidth2 = histNumbersSlot2.numbers[0];
	float4 avgBinsPerRange2 = histNumbersSlot2.numbers[1];
	float4 histogramWidth2 = histNumbersSlot2.numbers[2];
	RangeBound* minBound = histogram1LowerBound->val < histogram2LowerBound->val ? histogram1LowerBound : histogram2LowerBound;
	float8 lowerDiff1 = DatumGetFloat8(FunctionCall2Coll(&typcache1->rng_subdiff_finfo,
		typcache1->rng_collation,
		histogram1LowerBound->val, minBound->val));
	float8 lowerDiff2 = DatumGetFloat8(FunctionCall2Coll(&typcache1->rng_subdiff_finfo,
		typcache1->rng_collation,
		histogram2LowerBound->val, minBound->val));
	float8 baseValue1 = lowerDiff1 + binWidth1 / 2;
	float8 baseValue2 = lowerDiff2 + binWidth2 / 2;
	for (int i = 0; i < numberOfBins1; ++i) {
		histogram1x[i] = baseValue1 + binWidth1 * i;
		histogram1y[i] = (float8)histSlot1.values[i];
	}
	for (int i = 0; i < numberOfBins2; ++i) {
		histogram2x[i] = baseValue2 + binWidth2 * i;
		histogram2y[i] = (float8)histSlot2.values[i];
	}

	{
		fprintf(stderr, "Histogram 1 X: [");
		for (int i = 0; i < numberOfBins1; ++i) {
			fprintf(stderr, "%f, ", histogram1x[i]);
		}
		fprintf(stderr, "]\n");
		fprintf(stderr, "Histogram 1 Y: [");
		for (int i = 0; i < numberOfBins1; ++i) {
			fprintf(stderr, "%f, ", histogram1y[i]);
		}
		fprintf(stderr, "]\n");
		fprintf(stderr, "Histogram 2 X: [");
		for (int i = 0; i < numberOfBins2; ++i) {
			fprintf(stderr, "%f, ", histogram2x[i]);
		}
		fprintf(stderr, "]\n");
		fprintf(stderr, "Histogram 2 Y: [");
		for (int i = 0; i < numberOfBins2; ++i) {
			fprintf(stderr, "%f, ", histogram2y[i]);
		}
		fprintf(stderr, "]\n");
	}

	fprintf(stderr, "Cross your fingers\n");
	double result = calculate_range_join_overlap_fraction(
		histogram1x,
		histogram1y,
		lowerDiff1,
		lowerDiff1 + histogramWidth1,
		histogram2x,
		histogram2y,
		lowerDiff2,
		lowerDiff2 + histogramWidth2,
		numberOfBins1
	);
	fprintf(stderr, "Fuck it worked: %f \n", result);


	pfree(histogram1LowerBound);
	pfree(histogram1UpperBound);
	pfree(histogram2LowerBound);
	pfree(histogram2UpperBound);

	free_attstatsslot(&histSlot1);
	free_attstatsslot(&histNumbersSlot1);
	free_attstatsslot(&histBoundsSlot1);
	free_attstatsslot(&histSlot2);
	free_attstatsslot(&histNumbersSlot2);
	free_attstatsslot(&histBoundsSlot2);

	ReleaseVariableStats(vardata1);
	ReleaseVariableStats(vardata2);

	CLAMP_PROBABILITY(result);
	PG_RETURN_FLOAT8((float8)result);
}

/*
 * DBSA PROJECT overlaps && join cardinality
 */
Datum
areajoinsel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.005);
}

/*
 *	positionsel
 *
 * How likely is a box to be strictly left of (right of, above, below)
 * a given box?
 */

Datum
positionsel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.1);
}

Datum
positionjoinsel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.1);
}

/*
 *	contsel -- How likely is a box to contain (be contained by) a given box?
 *
 * This is a tighter constraint than "overlap", so produce a smaller
 * estimate than areasel does.
 */

Datum
contsel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.001);
}

Datum
contjoinsel(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(0.001);
}

