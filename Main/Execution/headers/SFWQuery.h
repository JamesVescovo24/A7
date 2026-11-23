#ifndef SFWQUERY_H
#define SFWQUERY_H

#include "ExprTree.h"
#include "MyDB_LogicalOps.h"
#include <map>
#include <string>
#include <vector>

// structure that stores an entire SFW query.  This file is included from ParserTypes.h, after
// the structs it uses (ValueList, FromList, FromItem, CNF) have been declared
struct SFWQuery {

private:

	// the various parts of the SQL query, as parsed
	vector <ExprTreePtr> valuesToSelect;		// the SELECT clause
	vector <FromItemPtr> fromItems;			// the FROM clause: tables and JOIN ... ON trees
	vector <ExprTreePtr> allDisjunctions;		// the WHERE clause, a CNF: these are ANDed together
	vector <ExprTreePtr> groupingClauses;		// the GROUP BY clause

	// the FROM clause, flattened (filled in by flatten ()): every (table name, alias) pair in it,
	// left to right, and every ON condition in it, in the order the JOINs appear.  An ON condition
	// is a CNF too, so this is a list of disjunctions, exactly like allDisjunctions
	vector <pair <string, string>> tablesToProcess;
	vector <ExprTreePtr> allOnDisjunctions;

	void flatten () {
		for (auto a : fromItems) {
			a->collectTables (tablesToProcess);
			a->collectOnDisjunctions (allOnDisjunctions);
		}
	}

	// whether optimizeQueryPlan () may memoize (Stage 3); on by default
	bool memoize = true;

public:

	SFWQuery () {}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
		struct CNF *cnf, struct ValueList *grouping) {
		valuesToSelect = selectClause->valuesToCompute;
		fromItems = fromClause->items;
		allDisjunctions = cnf->disjunctions;
		groupingClauses = grouping->valuesToCompute;
		flatten ();
	}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
		struct CNF *cnf) {
		valuesToSelect = selectClause->valuesToCompute;
		fromItems = fromClause->items;
		allDisjunctions = cnf->disjunctions;
		flatten ();
	}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause, struct ValueList *grouping) {
		valuesToSelect = selectClause->valuesToCompute;
		fromItems = fromClause->items;
		groupingClauses = grouping->valuesToCompute;
		flatten ();
	}

	SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
		valuesToSelect = selectClause->valuesToCompute;
		fromItems = fromClause->items;
		flatten ();
	}

	~SFWQuery () {}

	/*************************************************************/
	/** the parts of the query, for the optimizer and for tests **/
	/*************************************************************/

	// every (table name, alias) pair in the FROM clause, left to right
	vector <pair <string, string>> &getTables () {return tablesToProcess;}

	// every predicate the query applies, in order: first the ON conditions (in the order the
	// JOINs appear in the FROM clause), then the WHERE clause.  Each entry is one disjunction;
	// the entries are ANDed together.  This is THE order the predicates should appear in, in
	// any list of predicates handed to MyDB_Stats (see MyDB_Stats.cc for why the order matters)
	vector <ExprTreePtr> getAllPredicates () {
		vector <ExprTreePtr> res = allOnDisjunctions;
		for (auto a : allDisjunctions)
			res.push_back (a);
		return res;
	}

	vector <ExprTreePtr> &getValuesToSelect () {return valuesToSelect;}
	vector <ExprTreePtr> &getGroupingClauses () {return groupingClauses;}

	/*************************************************************/
	/** semantic checking (provided; see SemanticCheck.cc)      **/
	/*************************************************************/

	// checks the query against the given set of tables.  Returns "" if the query is OK, and
	// otherwise an error message.  main.cc calls this before optimizing, so the optimizer may
	// assume the query is valid
	string check (map <string, MyDB_TablePtr> &allTables);

	/*************************************************************/
	/** the optimizer (Stages 1 and 3; see SFWQuery.cc)         **/
	/*************************************************************/

	// builds and optimizes a logical query plan for this query, using the exhaustive enumeration
	// algorithm from class, and returns the best plan and its cost.  The cost of a plan is the
	// total number of tuples produced, summed over every op in the plan, as computed by MyDB_Stats.
	//
	// allTables: every table in the database, by table name (NOT by alias)
	pair <LogicalOpPtr, double> optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables);

	// the recursive step: the best plan for the given set of tables (by alias), producing exactly
	// the attributes in totSchema, and applying exactly the predicates in allDisjunctions
	pair <LogicalOpPtr, double> optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables,
        	MyDB_SchemaPtr totSchema, vector <ExprTreePtr> &allDisjunctions);

	// Stage 3: memoization.  When memoization is off, every call to the recursive step does its
	// own work; when it is on, the optimizer may remember and reuse results.  Either way the
	// plan and the cost returned must be identical.  The counter counts calls to the recursive
	// step that did the work (a call answered from memory does not count)
	void setMemoization (bool on) {memoize = on;}
	bool memoizationIsOn () {return memoize;}
	static long getNumCalls ();
	static void resetNumCalls ();

	/*************************************************************/
	/** the plan checker (Stage 2; see PlanChecker.cc)          **/
	/*************************************************************/

	// returns true if and only if plan is a correct logical plan for this query whose statistics
	// and cost are right: every table in the FROM clause is scanned exactly once, under its alias;
	// every predicate of the query is applied exactly once, at an op whose inputs contain every
	// table it refers to; every op's output schema holds exactly the attributes needed above it;
	// every op's statistics are what MyDB_Stats computes from its inputs and predicates; and the
	// tuple counts of all of the ops sum to claimedCost.  Prints a message for every violation
	bool checkPlan (LogicalOpPtr plan, double claimedCost, map <string, MyDB_TablePtr> &allTables);

	// prints the parsed query, for debugging
	void print () {
		cout << "Selecting the following:\n";
		for (auto a : valuesToSelect)
			cout << "\t" << a->toString () << "\n";
		cout << "From the following:\n";
		for (auto a : fromItems)
			cout << "\t" << a->toString () << "\n";
		cout << "Where the following are true:\n";
		for (auto a : allDisjunctions)
			cout << "\t" << a->toString () << "\n";
		cout << "Group using:\n";
		for (auto a : groupingClauses)
			cout << "\t" << a->toString () << "\n";
	}

	#include "FriendDecls.h"
};

#endif
