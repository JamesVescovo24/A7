#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "ParserTypes.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>

// The query optimizer: Stage 1 (the exhaustive enumeration algorithm) and Stage 3 (memoization).
// Your code goes in this file.  The interface, with comments, is in headers/SFWQuery.h; the
// algorithm is in the handout; the plan data structures are in MyDB_LogicalOps.h, and the
// costing is done for you by MyDB_Stats (MyDB_Stats.h, MyDB_Stats.cc).

// the number of calls to the recursive step that did their own work (that is, were not
// answered from the memo table).  Increment numCalls at the start of every such call
static long numCalls = 0;
long SFWQuery :: getNumCalls () {return numCalls;}
void SFWQuery :: resetNumCalls () {numCalls = 0;}

namespace {

using TablesByAlias = map <string, MyDB_TablePtr>;

void collectAttributes (const ExprTreePtr &expression, set <string> &attributes) {
	if (expression == nullptr)
		return;
	if (expression->isId ()) {
		attributes.insert (expression->getId ());
		return;
	}
	collectAttributes (expression->getLHS (), attributes);
	collectAttributes (expression->getRHS (), attributes);
	collectAttributes (expression->getChild (), attributes);
}

MyDB_SchemaPtr projectSchema (const TablesByAlias &tables, const set <string> &attributes) {
	auto schema = make_shared <MyDB_Schema> ();
	for (const auto &table : tables)
		for (const auto &attribute : table.second->getSchema ()->getAtts ())
			if (attributes.count (attribute.first))
				schema->appendAtt (attribute);
	return schema;
}

bool referencesAny (const ExprTreePtr &predicate, const TablesByAlias &tables) {
	return any_of (tables.begin (), tables.end (), [&predicate] (const auto &table) {
		return predicate->referencesTable (table.first);
	});
}

MyDB_TablePtr makeOutputTable (const TablesByAlias &tables, MyDB_SchemaPtr schema,
	const vector <pair <string, string>> &fromTables) {
	string name = tables.size () == 1 ? "scan" : "join";
	for (const auto &table : tables) {
		// Number occurrences of the same base table in FROM order.
		size_t occurrence = 1;
		for (const auto &fromTable : fromTables) {
			if (fromTable.second == table.first)
				break;
			if (fromTable.first == table.second->getName ())
				++occurrence;
		}
		name += "_" + table.first + "_" + to_string (occurrence);
	}
	return make_shared <MyDB_Table> (name, name + ".bin", schema);
}

string caseInsensitiveName (string name) {
	for (char &letter : name)
		letter = static_cast <char> (tolower (static_cast <unsigned char> (letter)));
	return name;
}

// Only reserve names for operators in the winning plan. The recursive search creates
// multiple candidates for the same aliases, all with the same provisional output name.
vector <string> reserveOutputNames (const LogicalOpPtr &plan, set <string> &names,
	set <string> &files) {
	auto output = plan->getOutputTable ();
	vector <string> aliasParts;
	if (plan->isTableScan ()) {
		aliasParts.push_back (output->getName ().substr (5)); // Remove "scan_".
	} else {
		auto join = plan->toJoin ();
		aliasParts = reserveOutputNames (join->getLeftInput (), names, files);
		auto rightParts = reserveOutputNames (join->getRightInput (), names, files);
		aliasParts.insert (aliasParts.end (), rightParts.begin (), rightParts.end ());
		sort (aliasParts.begin (), aliasParts.end ());
	}

	string name = output->getName ();
	auto used = [&names, &files] (const string &candidate) {
		string key = caseInsensitiveName (candidate);
		return names.count (key) || files.count (key + ".bin");
	};
	if (used (name)) {
		string encoded = plan->isTableScan () ? "scan_encoded" : "join_encoded";
		for (const string &part : aliasParts)
			encoded += "_" + to_string (part.size ()) + "_" + part;
		name = encoded;
		for (size_t suffix = 1; used (name); ++suffix)
			name = encoded + "_" + to_string (suffix);
		output->getName () = name;
		output->getStorageLoc () = name + ".bin";
	}
	names.insert (caseInsensitiveName (name));
	files.insert (caseInsensitiveName (name + ".bin"));
	return aliasParts;
}

}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
// and its cost.  allTables is every table in the database, by table NAME; the query's tables
// are in tablesToProcess as (name, alias) pairs, and its predicates in getAllPredicates ()
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables) {

	TablesByAlias aliasedTables;
	set <string> aliasedAttributes;
	for (const auto &table : tablesToProcess) {
		auto input = allTables.at (table.first)->alias (table.second);
		for (const auto &attribute : input->getSchema ()->getAtts ()) {
			if (!aliasedAttributes.insert (attribute.first).second) {
				cerr << "Optimizer error: multiple attributes map to " << attribute.first << '\n';
				return {nullptr, -1.0};
			}
		}
		aliasedTables.emplace (table.second, input);
	}

	set <string> attributes;
	for (const auto &value : valuesToSelect)
		collectAttributes (value, attributes);
	for (const auto &value : groupingClauses)
		collectAttributes (value, attributes);
	auto predicates = getAllPredicates ();
	auto result = optimizeQueryPlan (aliasedTables, projectSchema (aliasedTables, attributes), predicates);
	if (result.first != nullptr) {
		set <string> names, files;
		for (const auto &table : allTables) {
			names.insert (caseInsensitiveName (table.second->getName ()));
			const string &location = table.second->getStorageLoc ();
			files.insert (caseInsensitiveName (location.substr (location.find_last_of ("/\\") + 1)));
		}
		reserveOutputNames (result.first, names, files);
	}
	return result;
}

// the recursive step
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables,
	MyDB_SchemaPtr totSchema, vector <ExprTreePtr> &allDisjunctions) {

	++numCalls;
	if (allTables.empty ())
		return {nullptr, -1.0};
	if (allTables.size () == 1) {
		auto input = allTables.begin ()->second;
		auto stats = MyDB_Stats (input).costSelection (allDisjunctions);
		auto output = makeOutputTable (allTables, totSchema, tablesToProcess);
		return {make_shared <LogicalTableScan> (input, output, stats, allDisjunctions),
			stats->getTupleCount ()};
	}

	vector <string> aliases;
	for (const auto &table : allTables)
		aliases.push_back (table.first);
	const string &smallestAlias = aliases.front ();
	// Use one bit per alias so the subset mask can represent any query size.
	vector <bool> mask (aliases.size (), false);
	mask.front () = true;
	LogicalOpPtr bestPlan;
	double bestCost = 0.0;
	auto output = makeOutputTable (allTables, totSchema, tablesToProcess);
	while (find (mask.begin (), mask.end (), false) != mask.end ()) {
		TablesByAlias leftTables, rightTables;
		for (size_t index = 0; index < aliases.size (); ++index) {
			auto &side = mask[index] ? leftTables : rightTables;
			side.emplace (aliases[index], allTables.at (aliases[index]));
		}

		vector <ExprTreePtr> leftCNF, rightCNF, topCNF;
		for (const auto &predicate : allDisjunctions) {
			bool refersLeft = referencesAny (predicate, leftTables);
			bool refersRight = referencesAny (predicate, rightTables);
			if (refersLeft && refersRight) {
				topCNF.push_back (predicate);
			} else {
				bool goesLeft = refersLeft || (!refersRight && leftTables.count (smallestAlias));
				(goesLeft ? leftCNF : rightCNF).push_back (predicate);
			}
		}

		set <string> needed;
		for (const auto &attribute : totSchema->getAtts ())
			needed.insert (attribute.first);
		for (const auto &predicate : topCNF)
			collectAttributes (predicate, needed);
		auto left = optimizeQueryPlan (leftTables, projectSchema (leftTables, needed), leftCNF);
		auto right = optimizeQueryPlan (rightTables, projectSchema (rightTables, needed), rightCNF);
		auto stats = left.first->getStats ()->costJoin (topCNF, right.first->getStats ());
		double cost = stats->getTupleCount () + left.second + right.second;
		if (bestPlan == nullptr || cost < bestCost || (isnan (bestCost) && !isnan (cost))) {
			bestPlan = make_shared <LogicalJoin> (left.first, right.first, output, topCNF, stats);
			bestCost = cost;
		}

		// Increment the mask in binary. The loop handles the all-set mask as its end.
		size_t bit = 0;
		while (mask[bit]) {
			mask[bit] = false;
			++bit;
		}
		mask[bit] = true;
	}
	return {bestPlan, bestCost};
}

#endif
