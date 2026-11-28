#ifndef STAGE2_TEST_HELPERS_H
#define STAGE2_TEST_HELPERS_H

#include "QUnit.h"
#include "TestHelpers.h"
#include <algorithm>
#include <initializer_list>
#include <sstream>

namespace stage2Test {

using Catalog = map <string, MyDB_TablePtr>;

inline MyDB_TablePtr output (initializer_list <const char *> attributes) {
	auto schema = make_shared <MyDB_Schema> ();
	for (auto name : attributes)
		schema->appendAtt ({name, make_shared <MyDB_IntAttType> ()});
	return make_shared <MyDB_Table> ("result", "result.bin", schema);
}

inline MyDB_TablePtr fixtureTable (const string &name, size_t tuples, vector <size_t> distinct) {
	auto table = output ({"k", "v", "x"});
	table->getName () = name;
	table->getStorageLoc () = name + ".bin";
	table->setTupleCount (tuples);
	table->setDistinctValues (distinct);
	return table;
}

inline Catalog fixtureCatalog () {
	return {{"alpha", fixtureTable ("alpha", 12, {4, 6, 12})},
		{"beta", fixtureTable ("beta", 6, {3, 2, 6})},
		{"gamma", fixtureTable ("gamma", 2, {2, 1, 2})}};
}

inline LogicalOpPtr scan (MyDB_TablePtr input, initializer_list <const char *> attributes,
	vector <ExprTreePtr> predicates = {}, MyDB_StatsPtr stats = nullptr) {
	if (!stats)
		stats = MyDB_Stats (input).costSelection (predicates);
	return make_shared <LogicalTableScan> (input, output (attributes), stats, predicates);
}

inline LogicalOpPtr join (LogicalOpPtr left, LogicalOpPtr right, initializer_list <const char *> attributes,
	vector <ExprTreePtr> predicates = {}, MyDB_StatsPtr stats = nullptr) {
	if (!stats)
		stats = left->getStats ()->costJoin (predicates, right->getStats ());
	return make_shared <LogicalJoin> (left, right, output (attributes), predicates, stats);
}

inline shared_ptr <SQLStatement> checkedQuery (QUnit::UnitTest &qunit, Catalog &tables, const string &sql) {
	string error;
	auto query = parseQuery (sql, error);
	if (query && query->isSFWQuery ())
		error = query->check (tables);
	else if (error.empty ())
		error = "expected a SELECT statement";
	QUNIT_IS_EQUAL (error, string ());
	return error.empty () ? query : nullptr;
}

inline bool check (const shared_ptr <SQLStatement> &query, LogicalOpPtr plan, double cost, Catalog &tables) {
	return query && query->checkPlan (plan, cost, tables);
}

struct Diagnostics {
	ostringstream text;
	streambuf *savedOut = cout.rdbuf (text.rdbuf ());
	streambuf *savedErr = cerr.rdbuf (text.rdbuf ());

	~Diagnostics () {
		cout.rdbuf (savedOut);
		cerr.rdbuf (savedErr);
	}
};

inline size_t rejectionLines (QUnit::UnitTest &qunit, const shared_ptr <SQLStatement> &query,
	LogicalOpPtr plan, double cost, Catalog &tables) {
	bool accepted;
	string messages;
	{
		Diagnostics diagnostics;
		accepted = check (query, plan, cost, tables);
		messages = diagnostics.text.str ();
	}
	QUNIT_IS_FALSE (accepted);
	return count (messages.begin (), messages.end (), '\n');
}

inline ExprTreePtr id (const string &alias, const string &name) {
	return make_shared <Identifier> (alias, name);
}

inline ExprTreePtr keyEquality () {
	return make_shared <EqOp> (id ("a", "k"), id ("b", "k"));
}

}

#endif
