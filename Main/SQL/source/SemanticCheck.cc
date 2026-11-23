
// REFERENCE SOLUTION -- NOT FOR DISTRIBUTION
//
// The semantic check of a parsed SFW query against the catalog.  check ()
// returns "" if the query is OK, and otherwise a message describing the
// first problem found.

#include "ParserTypes.h"
#include <map>
#include <set>
#include <string>

using namespace std;

// the types an expression can have
enum ExprType {T_INT, T_DOUBLE, T_STRING, T_BOOL};

static string typeName (ExprType t) {
	switch (t) {
	case T_INT: return "int";
	case T_DOUBLE: return "double";
	case T_STRING: return "string";
	default: return "bool";
	}
}

static bool isNumeric (ExprType t) {
	return t == T_INT || t == T_DOUBLE;
}

// thrown when a check fails
struct SemanticError {
	string message;
	SemanticError (string m) : message (m) {}
};

// the aliases that an expression may refer to, and the table behind each one
typedef map <string, MyDB_TablePtr> Scope;

// type-checks an expression, resolving attribute references in the given
// scope.  Throws SemanticError on any problem
static ExprType typeOf (ExprTreePtr e, Scope &scope, bool aggsAllowed, bool insideAgg) {

	ExprTreePtr lhs = e->getLHS (), rhs = e->getRHS (), child = e->getChild ();

	switch (e->kind ()) {

	case BOOL_LITERAL: return T_BOOL;
	case DOUBLE_LITERAL: return T_DOUBLE;
	case INT_LITERAL: return T_INT;
	case STRING_LITERAL: return T_STRING;

	case IDENTIFIER_NODE: {
		string alias = e->getTable (), att = e->getAtt ();
		if (scope.count (alias) == 0)
			throw SemanticError ("there is no table with alias " + alias + " in scope for " + alias + "." + att);
		MyDB_TablePtr table = scope[alias];
		for (auto &a : table->getSchema ()->getAtts ()) {
			if (a.first == att) {
				string t = a.second->toString ();
				if (t == "int") return T_INT;
				if (t == "double") return T_DOUBLE;
				if (t == "string") return T_STRING;
				return T_BOOL;
			}
		}
		throw SemanticError ("table " + table->getName () + " (alias " + alias + ") has no attribute " + att);
	}

	case PLUS_OP: {
		ExprType l = typeOf (lhs, scope, aggsAllowed, insideAgg), r = typeOf (rhs, scope, aggsAllowed, insideAgg);
		if (isNumeric (l) && isNumeric (r))
			return (l == T_INT && r == T_INT) ? T_INT : T_DOUBLE;
		if (l == T_STRING && r == T_STRING)
			return T_STRING;
		throw SemanticError ("cannot add " + typeName (l) + " and " + typeName (r) + " in " + e->toString ());
	}

	case MINUS_OP:
	case TIMES_OP:
	case DIVIDE_OP: {
		ExprType l = typeOf (lhs, scope, aggsAllowed, insideAgg), r = typeOf (rhs, scope, aggsAllowed, insideAgg);
		if (isNumeric (l) && isNumeric (r))
			return (l == T_INT && r == T_INT && e->kind () != DIVIDE_OP) ? T_INT : T_DOUBLE;
		throw SemanticError ("arithmetic on " + typeName (l) + " and " + typeName (r) + " in " + e->toString ());
	}

	case GT_OP:
	case LT_OP: {
		ExprType l = typeOf (lhs, scope, aggsAllowed, insideAgg), r = typeOf (rhs, scope, aggsAllowed, insideAgg);
		if ((isNumeric (l) && isNumeric (r)) || (l == T_STRING && r == T_STRING))
			return T_BOOL;
		throw SemanticError ("cannot compare " + typeName (l) + " with " + typeName (r) + " in " + e->toString ());
	}

	case EQ_OP:
	case NEQ_OP: {
		ExprType l = typeOf (lhs, scope, aggsAllowed, insideAgg), r = typeOf (rhs, scope, aggsAllowed, insideAgg);
		if ((isNumeric (l) && isNumeric (r)) || (l == T_STRING && r == T_STRING) || (l == T_BOOL && r == T_BOOL))
			return T_BOOL;
		throw SemanticError ("cannot compare " + typeName (l) + " with " + typeName (r) + " in " + e->toString ());
	}

	case OR_OP: {
		ExprType l = typeOf (lhs, scope, aggsAllowed, insideAgg), r = typeOf (rhs, scope, aggsAllowed, insideAgg);
		if (l == T_BOOL && r == T_BOOL)
			return T_BOOL;
		throw SemanticError ("OR of non-boolean values in " + e->toString ());
	}

	case NOT_OP: {
		ExprType c = typeOf (child, scope, aggsAllowed, insideAgg);
		if (c == T_BOOL)
			return T_BOOL;
		throw SemanticError ("NOT of a non-boolean value in " + e->toString ());
	}

	case SUM_OP:
	case AVG_OP: {
		if (!aggsAllowed)
			throw SemanticError ("aggregate not allowed here: " + e->toString ());
		if (insideAgg)
			throw SemanticError ("aggregate inside an aggregate: " + e->toString ());
		ExprType c = typeOf (child, scope, aggsAllowed, true);
		if (!isNumeric (c))
			throw SemanticError ("aggregate of a " + typeName (c) + " in " + e->toString ());
		return (e->kind () == SUM_OP) ? c : T_DOUBLE;
	}
	}
	throw SemanticError ("unknown expression " + e->toString ());
}

// true if the expression is a legal thing to select from a grouped query:
// every attribute reference in it is inside an aggregate, or inside a
// subexpression that is one of the grouping expressions
static bool okForGrouping (ExprTreePtr e, set <string> &groupingExprs) {
	if (e->kind () == SUM_OP || e->kind () == AVG_OP)
		return true;
	if (groupingExprs.count (e->toString ()) > 0)
		return true;
	if (e->kind () == IDENTIFIER_NODE)
		return false;
	for (ExprTreePtr c : {e->getLHS (), e->getRHS (), e->getChild ()})
		if (c != nullptr && !okForGrouping (c, groupingExprs))
			return false;
	return true;
}

// checks the join tree under a FromItem: every ON condition is type-checked
// in the scope of the two sides it joins, and may not contain an aggregate
static void checkJoins (FromItemPtr item, Scope &everything) {
	if (item->isSingleTable ())
		return;
	checkJoins (item->getLHS (), everything);
	checkJoins (item->getRHS (), everything);

	// the scope of the ON is just the aliases under its two sides
	vector <pair <string, string>> tables;
	item->collectTables (tables);
	Scope scope;
	for (auto &t : tables)
		scope[t.second] = everything[t.second];

	for (auto a : item->getOnDisjunctions ()) {
		ExprType t = typeOf (a, scope, false, false);
		if (t != T_BOOL)
			throw SemanticError ("ON condition is not boolean: " + a->toString ());
	}
}

string SFWQuery :: check (map <string, MyDB_TablePtr> &allTables) {

	try {

		// every table has to exist, and every alias has to be unique
		Scope scope;
		for (auto &t : tablesToProcess) {
			if (allTables.count (t.first) == 0)
				throw SemanticError ("table " + t.first + " does not exist");
			if (scope.count (t.second) > 0)
				throw SemanticError ("alias " + t.second + " is used twice");
			scope[t.second] = allTables[t.first];
		}

		// the ON conditions, each in its own scope
		for (auto item : fromItems)
			checkJoins (item, scope);

		// the WHERE clause: everything is in scope, no aggregates
		for (auto a : allDisjunctions) {
			ExprType t = typeOf (a, scope, false, false);
			if (t != T_BOOL)
				throw SemanticError ("WHERE condition is not boolean: " + a->toString ());
		}

		// the GROUP BY clause: no aggregates
		set <string> groupingExprs;
		for (auto a : groupingClauses) {
			typeOf (a, scope, false, false);
			groupingExprs.insert (a->toString ());
		}

		// the SELECT clause: aggregates are allowed
		bool anyAgg = false;
		for (auto a : valuesToSelect) {
			typeOf (a, scope, true, false);
			if (a->hasAgg ())
				anyAgg = true;
		}

		// if this is an aggregate query, everything selected has to be an
		// aggregate or a function of the grouping expressions
		if (anyAgg || groupingClauses.size () > 0) {
			for (auto a : valuesToSelect) {
				if (!okForGrouping (a, groupingExprs))
					throw SemanticError ("selected value " + a->toString () +
						" is neither an aggregate nor a function of the grouping attributes");
			}
		}

	} catch (SemanticError &e) {
		return e.message;
	}

	return "";
}
