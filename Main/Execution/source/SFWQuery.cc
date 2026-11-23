#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "ParserTypes.h"
#include <map>
#include <set>

// The query optimizer: Stage 1 (the exhaustive enumeration algorithm) and Stage 3 (memoization).
// Your code goes in this file.  The interface, with comments, is in headers/SFWQuery.h; the
// algorithm is in the handout; the plan data structures are in MyDB_LogicalOps.h, and the
// costing is done for you by MyDB_Stats (MyDB_Stats.h, MyDB_Stats.cc).

// the number of calls to the recursive step that did their own work (that is, were not
// answered from the memo table).  Increment numCalls at the start of every such call
static long numCalls = 0;
long SFWQuery :: getNumCalls () {return numCalls;}
void SFWQuery :: resetNumCalls () {numCalls = 0;}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
// and its cost.  allTables is every table in the database, by table NAME; the query's tables
// are in tablesToProcess as (name, alias) pairs, and its predicates in getAllPredicates ()
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables) {

	// TODO: set up the three arguments of the recursive step, and call it
	return make_pair (nullptr, 0.0);
}

// the recursive step
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables,
	MyDB_SchemaPtr totSchema, vector <ExprTreePtr> &allDisjunctions) {

	// TODO
	return make_pair (nullptr, 0.0);
}

#endif
