#ifndef PLAN_CHECKER_CC
#define PLAN_CHECKER_CC

#include "ParserTypes.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>

namespace {

using Attribute = pair <string, string>; // alias and original attribute name
using Attributes = set <Attribute>;

string aliasedName (const Attribute &attribute) {
	return attribute.first + "_" + attribute.second;
}

// Check the expression before calling toString() or MyDB_Stats, which dereference
// the operands of comparisons, OR expressions, and other operators.
bool readReferences (const ExprTreePtr &expression, Attributes &references, set <const ExprTree *> &active) {
	if (!expression || !active.insert (expression.get ()).second)
		return false;

	bool valid = true;
	switch (expression->kind ()) {
	case IDENTIFIER_NODE:
		references.emplace (expression->getTable (), expression->getAtt ());
		break;
	case MINUS_OP: case PLUS_OP: case TIMES_OP: case DIVIDE_OP:
	case GT_OP: case LT_OP: case NEQ_OP: case EQ_OP: case OR_OP:
	{
		bool left = readReferences (expression->getLHS (), references, active);
		bool right = readReferences (expression->getRHS (), references, active);
		valid = left && right;
		break;
	}
	case NOT_OP: case SUM_OP: case AVG_OP:
		valid = readReferences (expression->getChild (), references, active);
		break;
	case BOOL_LITERAL: case DOUBLE_LITERAL: case INT_LITERAL: case STRING_LITERAL:
		break;
	}

	active.erase (expression.get ());
	return valid;
}

bool readReferences (const ExprTreePtr &expression, Attributes &references) {
	set <const ExprTree *> active;
	return readReferences (expression, references, active);
}

bool sameType (const MyDB_AttTypePtr &left, const MyDB_AttTypePtr &right) {
	return left && right && left->toString () == right->toString ();
}

bool matchesScan (const MyDB_TablePtr &input, const MyDB_TablePtr &expected) {
	if (!expected || input->getName () != expected->getName ()
		|| input->getStorageLoc () != expected->getStorageLoc ())
		return false;
	const auto &actual = input->getSchema ()->getAtts ();
	const auto &wanted = expected->getSchema ()->getAtts ();
	return equal (actual.begin (), actual.end (), wanted.begin (), wanted.end (),
		[] (const auto &a, const auto &b) {return a.first == b.first && sameType (a.second, b.second);});
}

struct ExpectedScan {
	MyDB_TablePtr table;
	size_t count = 0;
};

struct ExpectedPredicate {
	vector <size_t> positions;
	size_t count = 0;
};

struct PlanNode {
	unique_ptr <PlanNode> left, right;
	set <string> aliases;
	Attributes predicateAttributes;
	MyDB_SchemaPtr schema;
	MyDB_StatsPtr stats;
	double cost = 0.0;
};

class PlanChecker {
	map <string, ExpectedScan> scans;
	map <string, ExpectedPredicate> queryPredicates;
	set <const LogicalOp *> active;
	bool valid = true;

	void report (const string &message) {
		cerr << "Plan check: " << message << '\n';
		valid = false;
	}

	void compareNumber (const string &description, double actual, double expected) {
		if (fabs (actual - expected) <= 0.0001)
			return;
		ostringstream message;
		message << description << " is " << setprecision (numeric_limits <double>::max_digits10)
			<< actual << ", expected " << expected;
		report (message.str ());
	}

	MyDB_AttTypePtr catalogAttributeType (const Attribute &attribute) const {
		auto scan = scans.find (attribute.first);
		if (scan == scans.end () || !scan->second.table)
			return nullptr;
		string name = aliasedName (attribute);
		for (const auto &field : scan->second.table->getSchema ()->getAtts ())
			if (field.first == name)
				return field.second;
		return nullptr;
	}

	MyDB_TablePtr checkScan (PlanNode &node, const MyDB_TablePtr &input, const string &location) {
		if (!input || !input->getSchema ()) {
			report (location + " scan is missing its input table or schema");
			return nullptr;
		}
		auto match = scans.end ();
		for (auto scan = scans.begin (); scan != scans.end (); ++scan) {
			if (!matchesScan (input, scan->second.table))
				continue;
			if (match != scans.end ()) {
				report (location + " scan input matches multiple FROM aliases");
				return nullptr;
			}
			match = scan;
		}
		if (match == scans.end ()) {
			report (location + " scan of '" + input->getName () + "' at '" + input->getStorageLoc ()
				+ "' does not match a catalog table under a FROM alias");
			return nullptr;
		}
		node.aliases.insert (match->first);
		if (++match->second.count > 1)
			report (location + " repeats scan of FROM alias '" + match->first + "'");
		return match->second.table;
	}

	vector <ExprTreePtr> checkPredicates (PlanNode &node, const vector <ExprTreePtr> &predicates,
		const string &location, bool &canCost) {
		vector <ExprTreePtr> forCosting = predicates;
		vector <pair <size_t, size_t>> queryAndPlanPositions;
		map <string, size_t> localCounts;
		size_t nextPosition = 0;
		bool outOfOrder = false;

		for (size_t i = 0; i < predicates.size (); ++i) {
			Attributes references;
			if (!readReferences (predicates[i], references)) {
				report (location + " has a malformed predicate at position " + to_string (i + 1));
				canCost = false;
				continue;
			}
			const string text = predicates[i]->toString ();
			for (const auto &attribute : references) {
				if (node.aliases.count (attribute.first) && catalogAttributeType (attribute)) {
					node.predicateAttributes.insert (attribute);
				} else {
					report (location + " predicate " + text + " references unavailable attribute "
						+ attribute.first + "." + attribute.second);
					canCost = false;
				}
			}

			auto expected = queryPredicates.find (text);
			if (expected == queryPredicates.end ()) {
				report (location + " applies an unexpected predicate " + text);
				continue;
			}
			const auto &positions = expected->second.positions;
			if (++expected->second.count > positions.size ())
				report (location + " applies predicate " + text + " too many times");

			// Identical disjunctions can occur at multiple positions in the query.
			size_t occurrence = localCounts[text]++;
			if (occurrence >= positions.size ())
				continue;
			queryAndPlanPositions.emplace_back (positions[occurrence], i);
			auto position = lower_bound (positions.begin (), positions.end (), nextPosition);
			if (position == positions.end ()) {
				report (location + " applies predicate " + text + " out of query order");
				outOfOrder = true;
			} else {
				nextPosition = *position + 1;
			}
		}

		if (outOfOrder) {
			// Recompute in query order even when the plan listed the predicates out of order.
			auto sorted = queryAndPlanPositions;
			sort (sorted.begin (), sorted.end ());
			for (size_t i = 0; i < sorted.size (); ++i)
				forCosting[queryAndPlanPositions[i].second] = predicates[sorted[i].second];
		}
		return forCosting;
	}

	unique_ptr <PlanNode> inspect (const LogicalOpPtr &plan, const string &location) {
		if (!plan) {
			report (location + " is missing an operator");
			return nullptr;
		}
		if (!active.insert (plan.get ()).second) {
			report (location + " forms a cycle in the plan");
			return nullptr;
		}

		auto node = make_unique <PlanNode> ();
		auto output = plan->getOutputTable ();
		if (output)
			node->schema = output->getSchema ();
		if (!node->schema)
			report (location + " is missing its output table or schema");

		bool canCost = true;
		if (auto scan = plan->toTableScan ()) {
			auto base = checkScan (*node, scan->getInputTable (), location);
			auto predicates = checkPredicates (*node, scan->getPredicates (), location, canCost);
			if (base && canCost)
				node->stats = MyDB_Stats (base).costSelection (predicates);
		} else if (auto join = plan->toJoin ()) {
			node->left = inspect (join->getLeftInput (), location + ".left");
			node->right = inspect (join->getRightInput (), location + ".right");
			for (const auto *child : {node->left.get (), node->right.get ()})
				if (child)
					node->aliases.insert (child->aliases.begin (), child->aliases.end ());
			auto predicates = checkPredicates (*node, join->getPredicates (), location, canCost);
			if (canCost && node->left && node->left->stats && node->right && node->right->stats)
				node->stats = node->left->stats->costJoin (predicates, node->right->stats);
		} else {
			report (location + " has an unsupported operator");
		}

		auto actual = plan->getStats ();
		if (!actual)
			report (location + " is missing its statistics");
		if (node->stats) {
			node->cost = node->stats->getTupleCount ();
			if (node->left && node->right)
				node->cost = node->cost + node->left->cost + node->right->cost;
			if (actual) {
				compareNumber (location + " tuple count", actual->getTupleCount (), node->stats->getTupleCount ());
				if (node->schema) {
					set <string> compared;
					for (const auto &attribute : node->schema->getAtts ())
						if (compared.insert (attribute.first).second)
							compareNumber (location + " distinct-value count for " + attribute.first,
								actual->getAttVals (attribute.first), node->stats->getAttVals (attribute.first));
				}
			}
		}

		active.erase (plan.get ());
		return node;
	}

	void checkSchemas (const PlanNode &node, const Attributes &required, const string &location) {
		if (node.schema) {
			map <string, MyDB_AttTypePtr> expected;
			for (const auto &attribute : required) {
				string name = aliasedName (attribute);
				if (!expected.emplace (name, catalogAttributeType (attribute)).second)
					report (location + " requires multiple attributes named " + name);
			}
			set <string> seen;
			for (const auto &attribute : node.schema->getAtts ()) {
				if (!seen.insert (attribute.first).second)
					report (location + " output repeats attribute " + attribute.first);
				auto wanted = expected.find (attribute.first);
				if (wanted == expected.end ())
					report (location + " output has extra attribute " + attribute.first);
				else if (!attribute.second || (wanted->second && !sameType (attribute.second, wanted->second)))
					report (location + " output attribute " + attribute.first + " has the wrong type");
			}
			for (const auto &attribute : expected)
				if (!seen.count (attribute.first))
					report (location + " output is missing attribute " + attribute.first);
		}

		Attributes needed = required;
		needed.insert (node.predicateAttributes.begin (), node.predicateAttributes.end ());
		auto checkChild = [&] (const unique_ptr <PlanNode> &child, const string &side) {
			if (!child)
				return;
			Attributes childRequired;
			for (const auto &attribute : needed)
				if (child->aliases.count (attribute.first))
					childRequired.insert (attribute);
			checkSchemas (*child, childRequired, location + side);
		};
		checkChild (node.left, ".left");
		checkChild (node.right, ".right");
	}

public:
	bool check (SFWQuery &query, const LogicalOpPtr &plan, double claimedCost,
		const map <string, MyDB_TablePtr> &allTables) {
		for (const auto &from : query.getTables ()) {
			auto &scan = scans[from.second];
			auto table = allTables.find (from.first);
			if (table == allTables.end () || !table->second || !table->second->getSchema ())
				report ("catalog is missing the table or schema for FROM alias '" + from.second + "'");
			else
				scan.table = table->second->alias (from.second);
		}

		auto predicates = query.getAllPredicates ();
		for (size_t i = 0; i < predicates.size (); ++i) {
			Attributes references;
			if (readReferences (predicates[i], references))
				queryPredicates[predicates[i]->toString ()].positions.push_back (i);
			else
				report ("query contains a malformed predicate at position " + to_string (i + 1));
		}
		Attributes rootRequired;
		for (const auto *clause : {&query.getValuesToSelect (), &query.getGroupingClauses ()})
			for (const auto &expression : *clause)
				if (!readReferences (expression, rootRequired))
					report ("query contains a malformed SELECT or GROUP BY expression");

		auto root = inspect (plan, "root");
		for (const auto &scan : scans)
			if (scan.second.count == 0)
				report ("plan is missing a scan for FROM alias '" + scan.first + "'");
		for (const auto &predicate : queryPredicates) {
			if (predicate.second.count < predicate.second.positions.size ()) {
				size_t missing = predicate.second.positions.size () - predicate.second.count;
				report ("plan is missing " + to_string (missing) + " occurrence(s) of predicate " + predicate.first);
			}
		}
		if (root) {
			checkSchemas (*root, rootRequired, "root");
			if (root->stats)
				compareNumber ("claimed cost", claimedCost, root->cost);
		}
		return valid;
	}
};

}

bool SFWQuery :: checkPlan (LogicalOpPtr plan, double claimedCost, map <string, MyDB_TablePtr> &allTables) {
	return PlanChecker ().check (*this, plan, claimedCost, allTables);
}

#endif
