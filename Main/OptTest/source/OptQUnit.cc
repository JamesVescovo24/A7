/****************************************************
** COMP 530 A7 TESTS -- Fall 2026                   **
**                                                 **
** Tests of the query optimizer (Stage 1), the     **
** plan checker (Stage 2), and memoization         **
** (Stage 3).  Every expected cost comes from      **
** Examples/Examples.sql (see Queries.h).  The     **
** hidden tests have the same form, with more      **
** queries and more damaged plans.                 **
**                                                 **
**   optUnitTest          run all                  **
**   optUnitTest N        start from test N        **
**   optUnitTest N only   run test N and stop      **
**                                                 **
** Run from Build/bin, where myCat is.  main ()    **
** returns the number of failures.                 **
****************************************************/

#ifndef OPT_TEST_H
#define OPT_TEST_H

#include "QUnit.h"
#include "Queries.h"
#include "TestHelpers.h"
#include <iostream>
#include <map>
#include <set>

#define FALLTHROUGH_INTENDED do {} while (0)

static bool runOne = false;

// Stage 1: optimize the named example, compare the cost, and run checkPlan on the plan
void stage1 (QUnit::UnitTest &qunit, map <string, MyDB_TablePtr> &allTables, string name) {
	ExampleQuery q = query (name);
	cout << name << "..." << flush;
	Optimized o (allTables, q.sql);
	if (!o.ok ()) {
		cout << "could not optimize: " << o.error << endl;
		QUNIT_IS_TRUE (false);
		return;
	}
	cout << "cost " << o.cost << ", expected " << q.cost << "..." << flush;
	QUNIT_IS_TRUE (closeEnough (o.cost, q.cost, 1e-6));
	bool good = o.check (allTables);
	cout << (good ? "plan checks" : "plan does NOT check") << endl;
	QUNIT_IS_TRUE (good);
}

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

	QUnit::UnitTest qunit (cerr, QUnit::normal);
	map <string, MyDB_TablePtr> allTables = loadCatalog ("myCat");

	switch (start) {

	case 1:
	{
		// Stage 1: one table, selections only
		cout << "TEST 1..." << flush;
		stage1 (qunit, allTables, "E1");
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 2:
	{
		// Stage 1: two tables
		cout << "TEST 2..." << flush;
		stage1 (qunit, allTables, "E3");
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 3:
	{
		// Stage 1: three tables, JOIN ... ON syntax, no WHERE clause
		cout << "TEST 3..." << flush;
		stage1 (qunit, allTables, "J4");
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 4:
	{
		// Stage 2: checkPlan accepts the plan the optimizer produces, for every example
		cout << "TEST 4...checkPlan accepts every example's plan..." << flush;
		int accepted = 0;
		for (auto &q : allQueries ()) {
			Optimized o (allTables, q.sql);
			if (o.ok () && o.check (allTables))
				accepted++;
			else
				cout << "\n  " << q.name << " not accepted" << flush;
		}
		cout << accepted << " of 21 accepted" << endl;
		QUNIT_IS_EQUAL (accepted, 21);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 5:
	{
		// Stage 2: a claimed cost that is off by one is rejected (and the true cost accepted)
		cout << "TEST 5...cost off by one..." << flush;
		Optimized o (allTables, query ("E4").sql);
		QUNIT_IS_TRUE (o.ok ());
		QUNIT_IS_TRUE (o.check (o.plan, o.cost, allTables));
		bool got = o.check (o.plan, o.cost + 1, allTables);
		cout << "checkPlan said " << (got ? "valid" : "invalid") << endl;
		QUNIT_IS_TRUE (!got);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	case 6:
	{
		// Stage 3: E5 with and without memoization: same cost, valid plans, and at least
		// ten times fewer calls with memoization
		cout << "TEST 6...E5 with and without memoization..." << flush;
		Optimized with (allTables, query ("E5").sql, true);
		Optimized without (allTables, query ("E5").sql, false);
		QUNIT_IS_TRUE (with.ok () && without.ok ());
		cout << with.calls << " calls with, " << without.calls << " without" << endl;
		QUNIT_IS_TRUE (closeEnough (with.cost, without.cost, 1e-6));
		QUNIT_IS_TRUE (closeEnough (with.cost, query ("E5").cost, 1e-6));
		QUNIT_IS_TRUE (with.check (allTables));
		QUNIT_IS_TRUE (without.check (allTables));
		QUNIT_IS_TRUE (with.calls > 0 && without.calls >= 10 * with.calls);
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	default:
		break;
	}

	return qunit.errors ();
}

#endif
