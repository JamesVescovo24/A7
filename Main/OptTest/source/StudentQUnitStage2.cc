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
		// an example of the format: the plan the optimizer produces for E3 must be accepted,
		// and the same plan with a claimed cost that is off by one must be rejected
		cout << "TEST 1...E3 accepted, E3 with the wrong cost rejected..." << flush;
		Optimized o (allTables, query ("E3").sql);
		QUNIT_IS_TRUE (o.ok ());
		if (o.ok ()) {
			bool good = o.check (allTables);
			bool bad = o.check (o.plan, o.cost + 1, allTables);
			cout << (good ? "accepted" : "REJECTED") << ", wrong cost " << (bad ? "ACCEPTED" : "rejected") << endl;
			QUNIT_IS_TRUE (good);
			QUNIT_IS_TRUE (!bad);
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	default:
		break;
	}

	return qunit.errors ();
}
