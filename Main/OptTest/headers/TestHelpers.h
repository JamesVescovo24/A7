#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "Lexer.h"
#include "Parser.h"
#include "ParserTypes.h"
#include "MyDB_Catalog.h"
#include "MyDB_Table.h"
#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <string>

// Helpers shared by the A7 unit tests.
//
//   loadCatalog ("myCat")             -> the map from table name to table, from the catalog
//   parseQuery ("SELECT ...")         -> a parsed statement (nullptr on a parse error)
//   Optimized (allTables, sql)        -> parses, checks, and optimizes sql; holds the plan, the
//                                        cost, the number of optimizer calls, and the time
//
// Every test constructs its own Optimized, so nothing is shared between tests.

using namespace std;

inline map <string, MyDB_TablePtr> loadCatalog (string catalogFile) {
	MyDB_CatalogPtr myCatalog = make_shared <MyDB_Catalog> (catalogFile);
	return MyDB_Table :: getAllTables (myCatalog);
}

// parses one SQL statement.  Returns nullptr (and fills in errorMessage) on a parse error
inline shared_ptr <SQLStatement> parseQuery (string sql, string &errorMessage) {
	string parseMe = sql;
	parseMe.push_back ('\0');
	yyscan_t scanner;
	LexerExtra extra { "" };
	yylex_init_extra (&extra, &scanner);
	const YY_BUFFER_STATE buffer { yy_scan_string (parseMe.data (), scanner) };
	SQLStatement *final = nullptr;
	const int parseFailed { yyparse (scanner, &final) };
	yy_delete_buffer (buffer, scanner);
	yylex_destroy (scanner);
	if (parseFailed) {
		errorMessage = extra.errorMessage;
		if (final != nullptr)
			delete final;
		return nullptr;
	}
	return shared_ptr <SQLStatement> (final);
}

inline shared_ptr <SQLStatement> parseQuery (string sql) {
	string err;
	return parseQuery (sql, err);
}

// the result of optimizing one query
struct Optimized {

	shared_ptr <SQLStatement> stmt;	// the parsed statement; nullptr if parsing failed
	string error;			// the parse or semantic error, if any
	LogicalOpPtr plan;		// the plan (nullptr if there was an error)
	double cost = -1;		// its cost
	long calls = 0;			// the number of recursive optimizer calls it took
	double seconds = 0;		// and how long

	Optimized (map <string, MyDB_TablePtr> &allTables, string sql, bool memoize = true) {
		stmt = parseQuery (sql, error);
		if (stmt == nullptr) {
			error = "parse error: " + error;
			return;
		}
		if (!stmt->isSFWQuery ()) {
			error = "not a SELECT query";
			return;
		}
		error = stmt->check (allTables);
		if (error != "")
			return;
		stmt->getQuery ().setMemoization (memoize);
		SFWQuery :: resetNumCalls ();
		auto start = chrono :: steady_clock :: now ();
		pair <LogicalOpPtr, double> res = stmt->optimizeQuery (allTables);
		auto stop = chrono :: steady_clock :: now ();
		plan = res.first;
		cost = res.second;
		calls = SFWQuery :: getNumCalls ();
		seconds = chrono :: duration <double> (stop - start).count ();
	}

	bool ok () {return error == "" && plan != nullptr;}

	// runs checkPlan () on this plan (or on another plan/cost for this query)
	bool check (map <string, MyDB_TablePtr> &allTables) {
		return ok () && stmt->checkPlan (plan, cost, allTables);
	}
	bool check (LogicalOpPtr otherPlan, double otherCost, map <string, MyDB_TablePtr> &allTables) {
		return stmt != nullptr && stmt->checkPlan (otherPlan, otherCost, allTables);
	}
};

// relative comparison of two costs
inline bool closeEnough (double a, double b, double rel = 1e-6) {
	double m = max (1.0, max (fabs (a), fabs (b)));
	return fabs (a - b) <= rel * m;
}

#endif
