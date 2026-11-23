/****************************************************
** COMP 530 A7 -- Stage 3 tests: memoization       **
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
**   studentUnitTestStage3          run all        **
**   studentUnitTestStage3 N        start from N   **
**   studentUnitTestStage3 N only   run N and stop **
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
		// an example of the format: E4 (three tables) with and without memoization must give
		// the same cost, and memoization must cut the number of calls to the recursive step.
		// Three tables have 7 non-empty subsets, so a memoized optimizer makes 7 calls
		cout << "TEST 1...E4 with and without memoization..." << flush;
		Optimized with (allTables, query ("E4").sql, true);
		Optimized without (allTables, query ("E4").sql, false);
		QUNIT_IS_TRUE (with.ok () && without.ok ());
		if (with.ok () && without.ok ()) {
			cout << with.calls << " calls with, " << without.calls << " without" << endl;
			QUNIT_IS_TRUE (closeEnough (with.cost, without.cost, 1e-6));
			QUNIT_IS_TRUE (with.calls == 7);
			QUNIT_IS_TRUE (without.calls > with.calls);
		}
	}
	FALLTHROUGH_INTENDED;
	if (runOne) break;

	default:
		break;
	}

	return qunit.errors ();
}
