
#include "Lexer.h"
#include "Parser.h"
#include "ParserTypes.h"
#include "MyDB_Catalog.h"
#include "MyDB_Table.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unistd.h>

// The A7 driver.  Reads SQL statements (terminated by ';') from standard input, and for each
// SELECT query: checks it, optimizes it, prints the cost and the plan, and runs checkPlan () on
// the result.  Usage:
//
//     sqlUnitTest catalogFile tableDirectory [--no-memo] [--no-check] [--calls]
//
// When standard input is a terminal you get a prompt; when it is a file or a pipe you do not,
// so that    sqlUnitTest myCat . < queries.sql    produces clean output.
//
//   --no-memo    turn off memoization in the optimizer (SFWQuery :: setMemoization (false))
//   --no-check   do not run checkPlan () on each plan
//   --calls      after each query, print the number of recursive optimizer calls and the time

using namespace std;

string toLower (string data) {
	transform (data.begin (), data.end (), data.begin (), ::tolower);
	return data;
}

int main (int numArgs, char **args) {

	// make sure we have the correct arguments
	if (numArgs < 3) {
		cout << "args: catalog_file directory_for_tables [--no-memo] [--no-check] [--calls]\n";
		return 0;
	}
	bool useMemo = true, runCheck = true, showCalls = false;
	for (int i = 3; i < numArgs; i++) {
		string a = args[i];
		if (a == "--no-memo") useMemo = false;
		else if (a == "--no-check") runCheck = false;
		else if (a == "--calls") showCalls = true;
		else {
			cout << "unknown option " << a << "\n";
			return 1;
		}
	}
	bool interactive = isatty (0);

	// open up the catalog file and get every table in it.  Note that the optimizer never reads
	// any data; it needs only the catalog, which holds the statistics
	MyDB_CatalogPtr myCatalog = make_shared <MyDB_Catalog> (args [1]);
	map <string, MyDB_TablePtr> allTables = MyDB_Table :: getAllTables (myCatalog);

	// print out the intro notification
	if (interactive) {
		cout << "\n          Welcome to MyDB v0.1\n\n";
		cout << "\"Not the worst database in the world\" (tm) \n\n";
	}

	// and repeatedly accept queries
	while (true) {

		if (interactive)
			cout << "MyDB> ";

		// this will be used to collect the query
		stringstream ss;
		bool gotOne = false;

		// get a line
		for (string line; getline (cin, line);) {

			// strip SQL comments
			size_t c = line.find ("--");
			if (c != string :: npos)
				line.resize (c);

			// see if it has a ";" at the end
			size_t pos = line.find (';');

			// it does!!  so we are ready to parse
			if (pos != string :: npos) {

				gotOne = true;

				// append the last line
				line.resize (pos);
				ss << line;

				// see if we got a "quit" or "exit"
				string whole = ss.str ();
				vector <string> tokens {istream_iterator <string> {ss}, istream_iterator <string> {}};
				if (tokens.size () == 1 && (toLower (tokens[0]) == "exit" || toLower (tokens[0]) == "quit")) {
					if (interactive)
						cout << "OK, goodbye.\n";
					return 0;
				}
				if (tokens.size () == 0)
					break;

				// get the string to parse
				string parseMe = whole;

				// add an extra zero at the end; needed by lexer
				parseMe.push_back ('\0');

				// now parse it
				yyscan_t scanner;
				LexerExtra extra { "" };
				yylex_init_extra (&extra, &scanner);
				const YY_BUFFER_STATE buffer { yy_scan_string (parseMe.data(), scanner) };
				SQLStatement *final = nullptr;
				const int parseFailed { yyparse (scanner, &final) };
				yy_delete_buffer (buffer, scanner);
				yylex_destroy (scanner);

				// if we did not parse correctly
				if (parseFailed) {

					// print out the parse error
					cout << "Parse error: " << extra.errorMessage << "\n";

					// get outta here
					if (final != nullptr)
						delete final;
					break;

				// if we did parse correctly
				} else {

					// see if we got a create table
					if (final->isCreateTable ()) {

						string tableName = final->addToCatalog (args[2], myCatalog);
						MyDB_TablePtr temp = make_shared <MyDB_Table> ();
						temp->fromCatalog (tableName, myCatalog);
						if (tableName != "nothing") {
							allTables[tableName] = temp;
  							cout << "Added table " << tableName << "\n";
						}

					} else if (final->isSFWQuery ()) {

						// check it
						string err = final->check (allTables);
						if (err != "") {
							cout << "Semantic error: " << err << "\n";
						} else {

							// optimize it
							final->getQuery ().setMemoization (useMemo);
							SFWQuery :: resetNumCalls ();
							auto start = chrono :: steady_clock :: now ();
							pair <LogicalOpPtr, double> myPlan = final->optimizeQuery (allTables);
							auto stop = chrono :: steady_clock :: now ();
							cout << "Cost: " << myPlan.second << "\n";
							if (myPlan.first != nullptr)
								myPlan.first->print ();
							else
								cout << "(no plan was produced)\n";
							if (showCalls)
								cout << "Optimizer calls: " << SFWQuery :: getNumCalls () << ", time: "
									<< chrono :: duration <double> (stop - start).count () << " seconds\n";

							// and check it
							if (runCheck && myPlan.first != nullptr) {
								bool ok = final->checkPlan (myPlan.first, myPlan.second, allTables);
								cout << "Plan check: " << (ok ? "passed" : "FAILED") << "\n";
							}
						}
					}

					// get outta here
					if (final != nullptr)
						delete final;
					break;
				}

			} else {
				ss << line << "\n";
				if (interactive)
					cout << "    > ";
			}
		}

		// end of input
		if (!gotOne) {
			if (interactive)
				cout << "\n";
			return 0;
		}
		if (!interactive)
			cout << "\n";
	}
}
