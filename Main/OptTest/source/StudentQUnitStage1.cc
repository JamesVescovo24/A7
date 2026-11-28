/****************************************************
** COMP 530 A7 -- Stage 1 tests: the optimizer     **
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
**   studentUnitTestStage1          run all        **
**   studentUnitTestStage1 N        start from N   **
**   studentUnitTestStage1 N only   run N and stop **
**   Add --require-check-plan after Stage 2.       **
****************************************************/

#include "QUnit.h"
#include "Queries.h"
#include "TestHelpers.h"
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#define FALLTHROUGH_INTENDED do {} while (0)

static bool runOne = false;

int main (int argc, char *argv[]) {
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
	bool requireCheckPlan = string (argv[argc - 1]) == "--require-check-plan";

	QUnit::UnitTest qunit (cerr, QUnit::normal);
	map <string, MyDB_TablePtr> allTables = loadCatalog ("myCat");

	switch (start) {

	case 1:
	{
		// an example of the format: a one-table query with a GROUP BY.  The expected cost is
		// the one in Examples.sql.  A correct plan is a single table scan, so we also look at
		// the shape of the plan and at the number of tuples it says it returns
		cout << "TEST 1...E2..." << flush;
		Optimized o (allTables, query ("E2").sql);
		QUNIT_IS_TRUE (o.ok ());
		if (o.ok ()) {
			cout << "cost " << o.cost << ", expected " << query ("E2").cost << endl;
			QUNIT_IS_TRUE (closeEnough (o.cost, query ("E2").cost, 1e-6));
			QUNIT_IS_TRUE (o.plan->isTableScan ());
			QUNIT_IS_TRUE (closeEnough (o.plan->getStats ()->getTupleCount (), o.cost, 1e-6));
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 2:
	{
		cout << "TEST 2...all 21 example costs..." << endl;
		map <string, double> costs;
		for (const auto &example : allQueries ()) {
			Optimized o (allTables, example.sql);
			cout << example.name << ": cost " << o.cost << ", expected " << example.cost;
			if (!o.error.empty ())
				cout << " (" << o.error << ")";
			cout << endl;
			QUNIT_IS_TRUE (o.ok ());
			if (o.ok ()) {
				QUNIT_IS_TRUE (closeEnough (o.cost, example.cost));
				costs.emplace (example.name, o.cost);
			}
		}
		for (int number = 3; number <= 7; ++number) {
			string comma = "E" + to_string (number);
			string explicitJoin = "J" + to_string (number);
			if (costs.count (comma) && costs.count (explicitJoin))
				QUNIT_IS_TRUE (closeEnough (costs.at (comma), costs.at (explicitJoin)));
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 3:
	{
		cout << "TEST 3...projection through aggregates and GROUP BY..." << endl;
		Optimized e2 (allTables, query ("E2").sql);
		QUNIT_IS_TRUE (e2.ok ());
		if (e2.ok ()) {
			vector <string> expected {"l_l_quantity", "l_l_extendedprice", "l_l_discount",
				"l_l_tax", "l_l_returnflag"};
			QUNIT_IS_TRUE (outputAttributes (e2.plan) == expected);
		}
		Optimized grouping (allTables, "select sum (1) from nation as n group by n.n_name");
		QUNIT_IS_TRUE (grouping.ok ());
		if (grouping.ok ()) {
			QUNIT_IS_TRUE (outputAttributes (grouping.plan) == vector <string> {"n_n_name"});
			QUNIT_IS_EQUAL (grouping.cost, 25);
			QUNIT_IS_EQUAL (grouping.plan->getStats ()->getAttVals ("n_n_name"), 25);
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 4:
	{
		cout << "TEST 4...join predicate placement and scan projection..." << endl;
		Optimized m3 (allTables, query ("M3").sql);
		QUNIT_IS_TRUE (m3.ok () && m3.plan->isJoin ());
		if (m3.ok () && m3.plan->isJoin ()) {
			auto join = m3.plan->toJoin ();
			QUNIT_IS_TRUE (join->getPredicates () == m3.stmt->getQuery ().getAllPredicates ());
			auto nation = findScan (m3.plan, "n_n_name");
			auto region = findScan (m3.plan, "r_r_name");
			QUNIT_IS_TRUE (nation != nullptr && nation->getPredicates ().empty ());
			QUNIT_IS_TRUE (region != nullptr && region->getPredicates ().empty ());
		}
		Optimized m4 (allTables, query ("M4").sql);
		QUNIT_IS_TRUE (m4.ok () && m4.plan->isJoin ());
		if (m4.ok () && m4.plan->isJoin ()) {
			QUNIT_IS_TRUE (outputAttributes (m4.plan) == vector <string> {"s_s_name"});
			auto join = m4.plan->toJoin ();
			auto left = join->getLeftInput ();
			auto right = join->getRightInput ();
			auto nation = findScan (left, "n_n_name") != nullptr ? left : right;
			QUNIT_IS_TRUE (outputAttributes (nation) == vector <string> {"n_n_nationkey"});
			QUNIT_IS_EQUAL (nation->getStats ()->getAttVals ("n_n_nationkey"), 1);
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 5:
	{
		cout << "TEST 5...cross products, constant predicates, and empty projection..." << endl;
		Optimized m1 (allTables, query ("M1").sql);
		QUNIT_IS_TRUE (m1.ok () && m1.plan->isJoin ());
		if (m1.ok () && m1.plan->isJoin ()) {
			QUNIT_IS_TRUE (m1.plan->toJoin ()->getPredicates ().empty ());
			auto nation = findScan (m1.plan, "n_n_name");
			auto region = findScan (m1.plan, "r_r_name");
			QUNIT_IS_TRUE (nation != nullptr && nation->getPredicates ().empty ());
			QUNIT_IS_TRUE (region != nullptr && region->getPredicates ().empty ());
		}
		Optimized constant (allTables, query ("M1").sql + " where 1 < 2");
		QUNIT_IS_TRUE (constant.ok () && constant.plan->isJoin ());
		if (constant.ok () && constant.plan->isJoin ()) {
			QUNIT_IS_TRUE (closeEnough (constant.cost, 55));
			QUNIT_IS_TRUE (constant.plan->toJoin ()->getPredicates ().empty ());
			auto nation = findScan (constant.plan, "n_n_name");
			auto region = findScan (constant.plan, "r_r_name");
			auto predicates = constant.stmt->getQuery ().getAllPredicates ();
			QUNIT_IS_TRUE (nation != nullptr && nation->getPredicates () == predicates);
			QUNIT_IS_TRUE (region != nullptr && region->getPredicates ().empty ());
		}
		Optimized filtered (allTables, "select n.n_name from nation as n, region as r"
			" where r.r_name = \"AMERICA\"");
		QUNIT_IS_TRUE (filtered.ok ());
		if (filtered.ok ()) {
			QUNIT_IS_EQUAL (filtered.cost, 51);
			auto region = findScan (filtered.plan, "r_r_name");
			QUNIT_IS_TRUE (region != nullptr && region->getOutputTable ()->getSchema ()->getAtts ().empty ());
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 6:
	{
		cout << "TEST 6...self join aliases and output names..." << endl;
		Optimized m5 (allTables, query ("M5").sql);
		QUNIT_IS_TRUE (m5.ok () && m5.plan->isJoin ());
		if (m5.ok () && m5.plan->isJoin ()) {
			auto first = findScan (m5.plan, "c1_c_name");
			auto second = findScan (m5.plan, "c2_c_name");
			QUNIT_IS_TRUE (first != nullptr && second != nullptr && first != second);
			if (first != nullptr && second != nullptr) {
				QUNIT_IS_EQUAL (first->getInputTableName (), "customer");
				QUNIT_IS_EQUAL (second->getInputTableName (), "customer");
				QUNIT_IS_EQUAL (first->getOutputTable ()->getName (), "scan_c1_1");
				QUNIT_IS_EQUAL (first->getOutputTable ()->getStorageLoc (), "scan_c1_1.bin");
				QUNIT_IS_EQUAL (second->getOutputTable ()->getName (), "scan_c2_2");
			}
			vector <string> expected {"c1_c_name", "c2_c_name"};
			QUNIT_IS_TRUE (outputAttributes (m5.plan) == expected);
			QUNIT_IS_EQUAL (m5.plan->getOutputTable ()->getName (), "join_c1_1_c2_2");
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 7:
	{
		cout << "TEST 7...exhaustive split counts and bushy plan..." << endl;
		for (const auto &expected : vector <pair <string, long>> {{"M6", 1}, {"M1", 5}, {"J4", 37}}) {
			Optimized o (allTables, query (expected.first).sql);
			QUNIT_IS_TRUE (o.ok ());
			QUNIT_IS_EQUAL (o.calls, expected.second);
		}
		Optimized m8 (allTables, query ("M8").sql);
		QUNIT_IS_EQUAL (m8.calls, 365);
		QUNIT_IS_TRUE (m8.ok () && m8.plan->isJoin ());
		if (m8.ok () && m8.plan->isJoin ()) {
			auto join = m8.plan->toJoin ();
			auto left = join->getLeftInput ();
			auto right = join->getRightInput ();
			QUNIT_IS_TRUE (left->isJoin () && right->isJoin ());
			if (findScan (left, "p_p_name") == nullptr)
				left.swap (right);
			QUNIT_IS_TRUE (findScan (left, "p_p_name") != nullptr);
			QUNIT_IS_TRUE (findScan (left, "ps_ps_partkey") != nullptr);
			QUNIT_IS_TRUE (findScan (right, "s_s_name") != nullptr);
			QUNIT_IS_TRUE (findScan (right, "n_n_name") != nullptr);
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 8:
	{
		cout << "TEST 8...Stage 2 plan checker..." << endl;
		Optimized o (allTables, query ("E4").sql);
		QUNIT_IS_TRUE (o.ok ());
		if (o.ok ()) {
			QUNIT_IS_TRUE (closeEnough (o.cost, query ("E4").cost));
			bool accepted = o.check (allTables);
			if (accepted || requireCheckPlan) {
				QUNIT_IS_TRUE (accepted);
			} else {
				cout << "XFAIL: Stage 2 checkPlan stub rejects the plan." << endl;
			}
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 9:
	{
		cout << "TEST 9...aliased attribute name collision..." << endl;
		auto schema = make_shared <MyDB_Schema> ();
		schema->appendAtt ({"key", make_shared <MyDB_IntAttType> ()});
		schema->appendAtt ({"b_key", make_shared <MyDB_IntAttType> ()});
		auto input = make_shared <MyDB_Table> ("probe", "probe.bin", schema);
		vector <size_t> distinct {5, 5};
		input->setTupleCount (5);
		input->setDistinctValues (distinct);
		map <string, MyDB_TablePtr> tables {{"probe", input}};
		ostringstream diagnostic;
		auto previous = cerr.rdbuf (diagnostic.rdbuf ());
		Optimized o (tables, "select a.b_key, a_b.key from probe as a, probe as a_b");
		cerr.rdbuf (previous);
		QUNIT_IS_TRUE (o.error.empty ());
		QUNIT_IS_TRUE (o.plan == nullptr);
		QUNIT_IS_EQUAL (o.cost, -1);
		QUNIT_IS_EQUAL (o.calls, 0);
		QUNIT_IS_TRUE (diagnostic.str ().find ("a_b_key") != string::npos);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 10:
	{
		cout << "TEST 10...output names and paths after collisions..." << endl;
		auto tables = allTables;
		auto existing = make_shared <MyDB_Table> (*tables.at ("region"));
		existing->getName () = "scan_a_1";
		existing->getStorageLoc () = "./scan_c_1.bin";
		tables.emplace (existing->getName (), existing);
		Optimized o (tables, "select a.p_name, c.n_name from part as a, partsupp as b_1_c,"
			" supplier as a_1_b, nation as c where (a.p_partkey = b_1_c.ps_partkey)"
			" and (b_1_c.ps_suppkey = a_1_b.s_suppkey) and (a_1_b.s_nationkey = c.n_nationkey)"
			" and (a.p_size = 15) and (c.n_name = \"FRANCE\")");
		QUNIT_IS_TRUE (o.ok () && o.plan->isJoin ());
		if (o.ok () && o.plan->isJoin ()) {
			QUNIT_IS_TRUE (closeEnough (o.cost, query ("M8").cost));
			auto left = o.plan->toJoin ()->getLeftInput ();
			auto right = o.plan->toJoin ()->getRightInput ();
			QUNIT_IS_TRUE (left->isJoin () && right->isJoin ());
			set <string> joinNames {left->getOutputTable ()->getName (), right->getOutputTable ()->getName ()};
			QUNIT_IS_EQUAL (joinNames.size (), 2);
			QUNIT_IS_EQUAL (joinNames.count ("join_a_1_b_1_c_1"), 1);
			QUNIT_IS_TRUE (joinNames.count ("join_encoded_3_a_1_7_b_1_c_1") ||
				joinNames.count ("join_encoded_7_a_1_b_1_3_c_1"));
			auto part = findScan (o.plan, "a_p_name");
			auto nation = findScan (o.plan, "c_n_name");
			QUNIT_IS_TRUE (part != nullptr && nation != nullptr);
			if (part != nullptr && nation != nullptr) {
				QUNIT_IS_EQUAL (part->getOutputTable ()->getName (), "scan_encoded_3_a_1");
				QUNIT_IS_EQUAL (nation->getOutputTable ()->getStorageLoc (), "scan_encoded_3_c_1.bin");
			}
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 11:
	{
		cout << "TEST 11...order-sensitive scan costing and distinct values..." << endl;
		string base = "select n.n_nationkey from nation as n where ";
		string equality = "(n.n_name = \"FRANCE\")";
		string range = "(n.n_nationkey < 10)";
		Optimized first (allTables, base + equality + " and " + range);
		Optimized second (allTables, base + range + " and " + equality);
		QUNIT_IS_TRUE (first.ok () && first.plan->isTableScan ());
		QUNIT_IS_TRUE (second.ok () && second.plan->isTableScan ());
		if (first.ok () && first.plan->isTableScan () && second.ok () && second.plan->isTableScan ()) {
			// Costing works from the back. Range first gives 25/3 rows, then equality gives 1.
			// Equality first gives 1 row, then the range gives 1/3 and caps distinct values.
			QUNIT_IS_TRUE (closeEnough (first.cost, 1));
			QUNIT_IS_TRUE (closeEnough (second.cost, 1.0 / 3));
			QUNIT_IS_TRUE (closeEnough (first.plan->getStats ()->getAttVals ("n_n_nationkey"), 1));
			QUNIT_IS_TRUE (closeEnough (second.plan->getStats ()->getAttVals ("n_n_nationkey"), 1.0 / 3));
			QUNIT_IS_TRUE (first.plan->toTableScan ()->getPredicates () == first.stmt->getQuery ().getAllPredicates ());
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 12:
	{
		cout << "TEST 12...ON and WHERE order changes join statistics..." << endl;
		string base = "select n.n_name, r.r_name from nation as n join region as r on ";
		string regionKey = "(n.n_regionkey = r.r_regionkey)";
		string name = "(r.r_name = n.n_name)";
		Optimized first (allTables, base + regionKey + " where " + name);
		Optimized second (allTables, base + name + " where " + regionKey);
		QUNIT_IS_TRUE (first.ok () && first.plan->isJoin ());
		QUNIT_IS_TRUE (second.ok () && second.plan->isJoin ());
		if (first.ok () && first.plan->isJoin () && second.ok () && second.plan->isJoin ()) {
			// Region key first gives 25 join rows, then 25/5. Name first gives 5, then 5/5.
			QUNIT_IS_EQUAL (first.cost, 35);
			QUNIT_IS_EQUAL (second.cost, 31);
			QUNIT_IS_EQUAL (first.plan->getStats ()->getAttVals ("n_n_name"), 5);
			QUNIT_IS_EQUAL (second.plan->getStats ()->getAttVals ("n_n_name"), 1);
			QUNIT_IS_TRUE (first.plan->toJoin ()->getPredicates () == first.stmt->getQuery ().getAllPredicates ());
			QUNIT_IS_TRUE (second.plan->toJoin ()->getPredicates () == second.stmt->getQuery ().getAllPredicates ());
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 13:
	{
		cout << "TEST 13...equal child and candidate costs..." << endl;
		Optimized o (allTables, "select a.r_name, b.r_name from region as b, region as a");
		QUNIT_IS_TRUE (o.ok () && o.plan->isJoin ());
		if (o.ok () && o.plan->isJoin ()) {
			auto left = o.plan->toJoin ()->getLeftInput ();
			auto right = o.plan->toJoin ()->getRightInput ();
			QUNIT_IS_EQUAL (left->getStats ()->getTupleCount (), 5);
			QUNIT_IS_EQUAL (right->getStats ()->getTupleCount (), 5);
			QUNIT_IS_EQUAL (o.cost, 35);
			QUNIT_IS_EQUAL (o.calls, 5);
			QUNIT_IS_TRUE (outputAttributes (left) == vector <string> {"a_r_name"});
			QUNIT_IS_TRUE (outputAttributes (right) == vector <string> {"b_r_name"});
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	default:
		break;
	}

	return qunit.errors ();
}
