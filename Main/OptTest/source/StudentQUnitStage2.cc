/****************************************************
** COMP 530 A7 -- Stage 2 tests: checkPlan         **
**                                                 **
** This file is yours (well, your AI's).  Rules:   **
**  - every test is a case in the switch below,    **
**    numbered from 1, each ending in              **
**    FALLTHROUGH_INTENDED; if (runOne) break;     **
**  - every test prints "TEST N..." first and can  **
**    run on its own                               **
**  - expected costs come from Examples.sql (or    **
**    Queries.h), from arithmetic you can check by **
**    hand on the catalog's statistics, or from    **
**    comparing two ways of writing the same       **
**    query; never from running the code under     **
**    test and pasting what it printed             **
**  - helpers go in headers/TestHelpers.h or a new **
**    header under headers/; test code goes only   **
**    in this file                                 **
**                                                 **
**   studentUnitTestStage2          run all        **
**   studentUnitTestStage2 N        start from N   **
**   studentUnitTestStage2 N only   run N and stop **
****************************************************/

#include "QUnit.h"
#include "Queries.h"
#include "Stage2TestHelpers.h"
#include <algorithm>
#include <iostream>
#include <map>

#define FALLTHROUGH_INTENDED do {} while (0)

static bool runOne = false;

int main (int argc, char *argv[]) {
	using namespace stage2Test;
	int start = 1;
	if (argc > 1 && argv[1][0] >= '0' && argv[1][0] <= '9') {
		start = atoi (argv[1]);
	}
	if (argc > 2 && string (argv[2]) == "only") {
		runOne = true;
		cout << "running only test " << start << endl;
	} else {
		cout << "start from test " << start << endl;
	}

	QUnit::UnitTest qunit (cerr, QUnit::normal);
	map <string, MyDB_TablePtr> allTables = loadCatalog ("myCat");

	switch (start) {

	case 1:
	{
		cout << "TEST 1...supplied E1 cost and wrong total cost" << endl;
		auto example = query ("E1");
		Optimized optimized (allTables, example.sql);
		QUNIT_IS_TRUE (optimized.ok ());
		if (optimized.ok ()) {
			QUNIT_IS_TRUE (optimized.check (optimized.plan, example.cost, allTables));
			QUNIT_IS_FALSE (optimized.check (optimized.plan, example.cost + 1, allTables));
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 2:
	{
		cout << "TEST 2...scan identity and aliased input schema" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a");
		auto base = tables.at ("alpha");
		auto correct = scan (base->alias ("a"), {"a_k"});
		QUNIT_IS_TRUE (check (stmt, correct, 12, tables));

		auto wrongName = base->alias ("a");
		wrongName->getName () = "beta";
		auto wrongLocation = base->alias ("a");
		wrongLocation->getStorageLoc () = "other.bin";
		auto wrongType = base->alias ("a");
		wrongType->getSchema ()->getAtts ()[1].second = make_shared <MyDB_StringAttType> ();
		auto wrongOrder = base->alias ("a");
		swap (wrongOrder->getSchema ()->getAtts ()[1], wrongOrder->getSchema ()->getAtts ()[2]);
		for (const auto &input : {wrongName, wrongLocation, base, wrongType, wrongOrder})
			QUNIT_IS_FALSE (check (stmt, scan (input, {"a_k"}, {}, correct->getStats ()), 12, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 3:
	{
		cout << "TEST 3...self-join, repeated alias, and missing alias" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a, alpha as b");
		auto base = tables.at ("alpha");
		auto left = scan (base->alias ("a"), {"a_k"});
		auto right = scan (base->alias ("b"), {});
		// 12 + 12 + 12*12 = 168.
		QUNIT_IS_TRUE (check (stmt, join (left, right, {"a_k"}), 168, tables));
		auto repeated = scan (base->alias ("a"), {});
		QUNIT_IS_FALSE (check (stmt, join (left, repeated, {"a_k"}), 168, tables));
		QUNIT_IS_FALSE (check (stmt, left, 12, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 4:
	{
		cout << "TEST 4...predicate text and legitimate repeated disjunctions" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a where a.v < 7 and a.v < 7");
		auto first = make_shared <LtOp> (id ("a", "v"), make_shared <IntLiteral> (7));
		auto second = make_shared <LtOp> (id ("a", "v"), make_shared <IntLiteral> (7));
		auto plan = scan (tables.at ("alpha")->alias ("a"), {"a_k"}, {first, second});
		QUNIT_IS_TRUE (check (stmt, plan, 12.0 / 9, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 5:
	{
		cout << "TEST 5...missing, extra, and duplicated ON/WHERE predicates" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables,
			"select a.v from alpha as a join beta as b on a.k = b.k where a.x < 4");
		auto a = tables.at ("alpha")->alias ("a");
		auto b = tables.at ("beta")->alias ("b");
		auto filter = make_shared <LtOp> (id ("a", "x"), make_shared <IntLiteral> (4));
		auto extra = make_shared <GtOp> (id ("a", "x"), make_shared <IntLiteral> (0));
		auto right = scan (b, {"b_k"});
		// One filter gives 4 rows from a, 6 from b, and 6 from the join.
		QUNIT_IS_TRUE (check (stmt, join (scan (a, {"a_v", "a_k"}, {filter}), right, {"a_v"},
			{keyEquality ()}), 16, tables));
		QUNIT_IS_FALSE (check (stmt, join (scan (a, {"a_v"}, {filter}), scan (b, {}), {"a_v"}), 34, tables));
		QUNIT_IS_FALSE (check (stmt, join (scan (a, {"a_v", "a_k"}), right, {"a_v"},
			{keyEquality ()}), 36, tables));
		// Two filters give 4/3 rows from a, 6 from b, and 8/3 from the join.
		QUNIT_IS_FALSE (check (stmt, join (scan (a, {"a_v", "a_k"}, {filter, extra}), right, {"a_v"},
			{keyEquality ()}), 10, tables));
		QUNIT_IS_FALSE (check (stmt, join (scan (a, {"a_v", "a_k"}, {filter, filter}), right, {"a_v"},
			{keyEquality ()}), 10, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 6:
	{
		cout << "TEST 6...predicate applied below one of its aliases" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables,
			"select a.v from alpha as a join beta as b on a.k = b.k");
		// costSelection can cost this misplaced equality using a.k alone, giving
		// 3 scan rows. The cross product then gives 18 rows, for a total of 27.
		auto left = scan (tables.at ("alpha")->alias ("a"), {"a_v"}, {keyEquality ()});
		auto right = scan (tables.at ("beta")->alias ("b"), {});
		QUNIT_IS_FALSE (check (stmt, join (left, right, {"a_v"}), 27, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 7:
	{
		cout << "TEST 7...query predicate order with and without a cost change" << endl;
		auto tables = fixtureCatalog ();
		auto a = tables.at ("alpha")->alias ("a");
		auto lower = make_shared <GtOp> (id ("a", "x"), make_shared <IntLiteral> (2));
		auto upper = make_shared <LtOp> (id ("a", "v"), make_shared <IntLiteral> (5));
		auto independent = checkedQuery (qunit, tables,
			"select a.k from alpha as a where a.v < 5 and a.x > 2");
		QUNIT_IS_TRUE (check (independent, scan (a, {"a_k"}, {upper, lower}), 12.0 / 9, tables));
		QUNIT_IS_FALSE (check (independent, scan (a, {"a_k"}, {lower, upper}), 12.0 / 9, tables));

		auto equal = make_shared <EqOp> (id ("a", "v"), make_shared <IntLiteral> (5));
		auto limit = make_shared <LtOp> (id ("a", "x"), make_shared <IntLiteral> (6));
		auto dependent = checkedQuery (qunit, tables,
			"select a.k from alpha as a where a.v = 5 and a.x < 6");
		// In query order, the range limits a.v to four distinct values before
		// equality is costed. Reversing the predicates gives (12/6)/3.
		auto ordered = scan (a, {"a_k"}, {equal, limit});
		QUNIT_IS_TRUE (check (dependent, ordered, 1, tables));
		auto reversed = scan (a, {"a_k"}, {limit, equal});
		QUNIT_IS_TRUE (closeEnough (reversed->getStats ()->getTupleCount (), 2.0 / 3));
		auto reversedWithCorrectStats = scan (a, {"a_k"}, {limit, equal}, ordered->getStats ());
		auto orderErrors = rejectionLines (qunit, dependent, reversedWithCorrectStats, 1, tables);
		auto orderAndStatsErrors = rejectionLines (qunit, dependent, reversed, 1, tables);
		QUNIT_IS_TRUE (orderErrors > 0);
		QUNIT_IS_TRUE (orderAndStatsErrors > orderErrors);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 8:
	{
		cout << "TEST 8...non-optimal plan with a deferred filter" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables,
			"select a.k from alpha as a join beta as b on a.k = b.k where a.v < 4");
		auto a = tables.at ("alpha")->alias ("a");
		auto right = scan (tables.at ("beta")->alias ("b"), {"b_k"});
		auto filter = make_shared <LtOp> (id ("a", "v"), make_shared <IntLiteral> (4));
		auto pushed = join (scan (a, {"a_k"}, {filter}), right, {"a_k"}, {keyEquality ()});
		auto deferred = join (scan (a, {"a_k", "a_v"}), right, {"a_k"}, {keyEquality (), filter});
		QUNIT_IS_TRUE (check (stmt, pushed, 4 + 6 + 6, tables));
		QUNIT_IS_TRUE (check (stmt, deferred, 12 + 6 + 18 / 3, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 9:
	{
		cout << "TEST 9...root SELECT/GROUP BY attributes, multiplicity, and types" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select sum(a.v + a.x) from alpha as a group by a.k");
		auto a = tables.at ("alpha")->alias ("a");
		auto correct = scan (a, {"a_x", "a_k", "a_v"});
		QUNIT_IS_TRUE (check (stmt, correct, 12, tables));
		QUNIT_IS_FALSE (check (stmt, scan (a, {"a_x", "a_v"}), 12, tables));
		QUNIT_IS_FALSE (check (stmt, scan (a, {"a_k", "a_v"}), 12, tables));
		QUNIT_IS_FALSE (check (stmt, scan (a, {"a_k", "a_v", "a_x", "a_v"}), 12, tables));
		auto wrongType = scan (a, {"a_x", "a_k", "a_v"});
		wrongType->getOutputTable ()->getSchema ()->getAtts ()[0].second = make_shared <MyDB_StringAttType> ();
		QUNIT_IS_FALSE (check (stmt, wrongType, 12, tables));

		auto selectOnly = checkedQuery (qunit, tables, "select a.v from alpha as a");
		QUNIT_IS_TRUE (check (selectOnly, scan (a, {"a_v"}), 12, tables));
		QUNIT_IS_FALSE (check (selectOnly, correct, 12, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 10:
	{
		cout << "TEST 10...nested join projections and an empty child schema" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables,
			"select a.v from alpha as a join beta as b on a.k = b.k, gamma as c");
		auto a = tables.at ("alpha")->alias ("a");
		auto b = scan (tables.at ("beta")->alias ("b"), {"b_k"});
		auto c = scan (tables.at ("gamma")->alias ("c"), {});
		auto left = scan (a, {"a_k", "a_v"});
		auto inner = join (left, b, {"a_v"}, {keyEquality ()});
		// Scans: 12, 6 and 2. Inner join: 18. Outer cross product: 36.
		QUNIT_IS_TRUE (check (stmt, join (inner, c, {"a_v"}), 74, tables));
		auto missingKey = join (scan (a, {"a_v"}), b, {"a_v"}, {keyEquality ()});
		QUNIT_IS_FALSE (check (stmt, join (missingKey, c, {"a_v"}), 74, tables));
		auto extraKey = join (left, b, {"a_v", "a_k"}, {keyEquality ()});
		QUNIT_IS_FALSE (check (stmt, join (extraKey, c, {"a_v"}), 74, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 11:
	{
		cout << "TEST 11...wrong scan tuple count with a correct claimed cost" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a");
		auto a = tables.at ("alpha")->alias ("a");
		auto copy = tables.at ("alpha")->alias ("a");
		copy->setTupleCount (13);
		auto wrongStats = make_shared <MyDB_Stats> (copy);
		QUNIT_IS_TRUE (check (stmt, scan (a, {"a_k"}), 12, tables));
		QUNIT_IS_FALSE (check (stmt, scan (a, {"a_k"}, {}, wrongStats), 12, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 12:
	{
		cout << "TEST 12...join tuple count and cost recomputed from catalog statistics" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a join beta as b on a.k = b.k");
		auto a = tables.at ("alpha")->alias ("a");
		auto right = scan (tables.at ("beta")->alias ("b"), {"b_k"});
		vector <ExprTreePtr> predicates {keyEquality ()};
		auto correct = join (scan (a, {"a_k"}), right, {"a_k"}, predicates);
		QUNIT_IS_TRUE (check (stmt, correct, 36, tables));

		auto enlarged = tables.at ("alpha")->alias ("a");
		enlarged->setTupleCount (24);
		auto wrongStats = MyDB_Stats (enlarged).costJoin (predicates, right->getStats ());
		auto damagedJoin = join (correct->toJoin ()->getLeftInput (), right, {"a_k"}, predicates, wrongStats);
		// The recorded join count is 36 instead of 18. Only the second claim
		// adds a cost error to the tuple-count error.
		auto correctClaim = rejectionLines (qunit, stmt, damagedJoin, 36, tables);
		auto recordedClaim = rejectionLines (qunit, stmt, damagedJoin, 54, tables);
		QUNIT_IS_TRUE (correctClaim > 0);
		QUNIT_IS_TRUE (recordedClaim > correctClaim);

		auto damagedScan = scan (a, {"a_k"}, {}, make_shared <MyDB_Stats> (enlarged));
		auto correctJoinStats = join (damagedScan, right, {"a_k"}, predicates, correct->getStats ());
		auto wrongJoinStats = join (damagedScan, right, {"a_k"}, predicates);
		auto childErrors = rejectionLines (qunit, stmt, correctJoinStats, 36, tables);
		auto childAndParentErrors = rejectionLines (qunit, stmt, wrongJoinStats, 36, tables);
		QUNIT_IS_TRUE (childErrors > 0);
		QUNIT_IS_TRUE (childAndParentErrors > childErrors);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 13:
	{
		cout << "TEST 13...wrong scan and join distinct-value counts" << endl;
		auto tables = fixtureCatalog ();
		auto a = tables.at ("alpha")->alias ("a");
		auto fake = fixtureTable ("alpha", 12, {3, 6, 12})->alias ("a");
		auto scanQuery = checkedQuery (qunit, tables, "select a.k from alpha as a");
		auto left = scan (a, {"a_k"});
		auto wrongLeft = scan (a, {"a_k"}, {}, make_shared <MyDB_Stats> (fake));
		QUNIT_IS_TRUE (check (scanQuery, left, 12, tables));
		QUNIT_IS_FALSE (check (scanQuery, wrongLeft, 12, tables));

		auto joinQuery = checkedQuery (qunit, tables, "select a.k from alpha as a, beta as b");
		auto right = scan (tables.at ("beta")->alias ("b"), {});
		vector <ExprTreePtr> none;
		auto wrongStats = wrongLeft->getStats ()->costJoin (none, right->getStats ());
		QUNIT_IS_TRUE (check (joinQuery, join (left, right, {"a_k"}), 90, tables));
		QUNIT_IS_FALSE (check (joinQuery, join (left, right, {"a_k"}, {}, wrongStats), 90, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 14:
	{
		cout << "TEST 14...inclusive absolute tolerance for cost and statistics" << endl;
		auto tables = fixtureCatalog ();
		tables["precision"] = fixtureTable ("precision", 20000, {20000, 1, 1});
		auto stmt = checkedQuery (qunit, tables, "select 1 from precision as p where p.k = 1");
		auto p = tables.at ("precision")->alias ("p");
		vector <ExprTreePtr> equal {make_shared <EqOp> (id ("p", "k"), make_shared <IntLiteral> (1))};
		auto correct = scan (p, {}, equal);
		QUNIT_IS_TRUE (check (stmt, correct, 1.00005, tables));
		QUNIT_IS_FALSE (check (stmt, correct, 1.00015, tables));
		auto fullTable = checkedQuery (qunit, tables, "select 1 from precision as p");
		QUNIT_IS_FALSE (check (fullTable, scan (p, {}), 20000.001, tables));

		auto fake = tables.at ("precision")->alias ("p");
		fake->setTupleCount (20001);
		QUNIT_IS_TRUE (check (stmt, scan (p, {}, equal, MyDB_Stats (fake).costSelection (equal)), 1, tables));
		fake->setTupleCount (20003);
		QUNIT_IS_FALSE (check (stmt, scan (p, {}, equal, MyDB_Stats (fake).costSelection (equal)), 1, tables));

		auto zeroQuery = checkedQuery (qunit, tables, "select 1 from gamma as c where c.v <> 0");
		auto unequal = make_shared <NeqOp> (id ("c", "v"), make_shared <IntLiteral> (0));
		// 2 - 2/1 = 0. Comparing with zero tests the boundary without subtraction rounding.
		auto zeroPlan = scan (tables.at ("gamma")->alias ("c"), {}, {unequal});
		QUNIT_IS_TRUE (check (zeroQuery, zeroPlan, 0.0001, tables));
		QUNIT_IS_FALSE (check (zeroQuery, zeroPlan, 0.00010001, tables));
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 15:
	{
		cout << "TEST 15...malformed plans and multiple diagnostics" << endl;
		auto tables = fixtureCatalog ();
		auto stmt = checkedQuery (qunit, tables, "select a.k from alpha as a, beta as b");
		QUNIT_IS_FALSE (check (stmt, nullptr, 0, tables));
		vector <ExprTreePtr> malformed {nullptr, make_shared <EqOp> (id ("b", "k"), nullptr)};
		vector <ExprTreePtr> none;
		auto brokenScan = make_shared <LogicalTableScan> (nullptr, nullptr, nullptr, malformed);
		auto brokenJoin = make_shared <LogicalJoin> (nullptr, brokenScan, output ({"a_x"}), none, nullptr);
		auto lines = rejectionLines (qunit, stmt, brokenJoin, 0, tables);
		QUNIT_IS_TRUE (lines >= 4);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	default:
		break;
	}

	return qunit.errors ();
}
