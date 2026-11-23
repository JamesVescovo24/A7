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
****************************************************/

#include "QUnit.h"
#include "Queries.h"
#include "TestHelpers.h"
#include <iostream>
#include <map>
#include <set>

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

	default:
		break;
	}

	return qunit.errors ();
}
