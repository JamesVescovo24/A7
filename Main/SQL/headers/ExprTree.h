
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// the kinds of nodes in an expression tree
enum ExprKind {BOOL_LITERAL, DOUBLE_LITERAL, INT_LITERAL, STRING_LITERAL, IDENTIFIER_NODE,
	MINUS_OP, PLUS_OP, TIMES_OP, DIVIDE_OP, GT_OP, LT_OP, NEQ_OP, EQ_OP, OR_OP, NOT_OP, SUM_OP, AVG_OP};

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

public:

	// prints the expression, for debugging
	virtual string toString () = 0;
	virtual ~ExprTree () {}

	// what kind of node this is
	virtual ExprKind kind () = 0;

	// the children: lhs and rhs for a binary operator, child for a unary one;
	// nullptr where there is no such child
	virtual ExprTreePtr getLHS () {return nullptr;}
	virtual ExprTreePtr getRHS () {return nullptr;}
	virtual ExprTreePtr getChild () {return nullptr;}

	// for an identifier: the alias and the attribute name
	virtual string getTable () {return "";}
	virtual string getAtt () {return "";}

	// true if there is a SUM or an AVG anywhere in this expression
	bool hasAgg () {
		if (kind () == SUM_OP || kind () == AVG_OP)
			return true;
		for (ExprTreePtr c : {getLHS (), getRHS (), getChild ()})
			if (c != nullptr && c->hasAgg ())
				return true;
		return false;
	}

	// --- the questions the optimizer and MyDB_Stats ask (see MyDB_Stats.cc) ---

	// is this node an identifier, i.e. a reference to an attribute?
	bool isId () {return kind () == IDENTIFIER_NODE;}

	// for an identifier, the name it has in a query plan: the alias, an underscore,
	// and the attribute name ("l_l_shipdate" for l.l_shipdate).  "" otherwise
	string getId () {return isId () ? getTable () + "_" + getAtt () : "";}

	// the various comparisons.  isComp () is true for any of =, <>, <, >, and
	// NOT applied to one of them; isLTGT () for < and >; isNotEq () for <>
	bool isEq () {return kind () == EQ_OP;}
	bool isNotEq () {return kind () == NEQ_OP;}
	bool isLTGT () {return kind () == LT_OP || kind () == GT_OP;}
	bool isComp () {return isEq () || isNotEq () || isLTGT () || kind () == NOT_OP;}
	bool isOr () {return kind () == OR_OP;}
	bool isSum () {return kind () == SUM_OP;}
	bool isAvg () {return kind () == AVG_OP;}

	// true if some identifier in this expression refers to the table with the
	// given alias
	bool referencesTable (string alias) {
		if (isId ())
			return getTable () == alias;
		for (ExprTreePtr c : {getLHS (), getRHS (), getChild ()})
			if (c != nullptr && c->referencesTable (alias))
				return true;
		return false;
	}

	// true if some identifier in this expression is exactly alias.attName
	bool referencesAtt (string alias, string attName) {
		if (isId ())
			return getTable () == alias && getAtt () == attName;
		for (ExprTreePtr c : {getLHS (), getRHS (), getChild ()})
			if (c != nullptr && c->referencesAtt (alias, attName))
				return true;
		return false;
	}
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:

	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}

	ExprKind kind () {return BOOL_LITERAL;}
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}

	ExprKind kind () {return DOUBLE_LITERAL;}

	~DoubleLiteral () {}
};

class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	ExprKind kind () {return INT_LITERAL;}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	ExprKind kind () {return STRING_LITERAL;}

	~StringLiteral () {}
};

class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
	}

	Identifier (string tableNameIn, string attNameIn) {
		tableName = tableNameIn;
		attName = attNameIn;
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}

	ExprKind kind () {return IDENTIFIER_NODE;}
	string getTable () {return tableName;}
	string getAtt () {return attName;}

	~Identifier () {}
};

// all of the binary operators look the same, so they share this
class BinaryOp : public ExprTree {

protected:

	ExprTreePtr lhs;
	ExprTreePtr rhs;

public:

	BinaryOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	ExprTreePtr getLHS () {return lhs;}
	ExprTreePtr getRHS () {return rhs;}

	virtual string opName () = 0;

	string toString () {
		return opName () + " (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	virtual ~BinaryOp () {}
};

class MinusOp : public BinaryOp {
public:
	MinusOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "-";}
	ExprKind kind () {return MINUS_OP;}
};

class PlusOp : public BinaryOp {
public:
	PlusOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "+";}
	ExprKind kind () {return PLUS_OP;}
};

class TimesOp : public BinaryOp {
public:
	TimesOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "*";}
	ExprKind kind () {return TIMES_OP;}
};

class DivideOp : public BinaryOp {
public:
	DivideOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "/";}
	ExprKind kind () {return DIVIDE_OP;}
};

class GtOp : public BinaryOp {
public:
	GtOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return ">";}
	ExprKind kind () {return GT_OP;}
};

class LtOp : public BinaryOp {
public:
	LtOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "<";}
	ExprKind kind () {return LT_OP;}
};

class NeqOp : public BinaryOp {
public:
	NeqOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "!=";}
	ExprKind kind () {return NEQ_OP;}
};

class EqOp : public BinaryOp {
public:
	EqOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "==";}
	ExprKind kind () {return EQ_OP;}
};

class OrOp : public BinaryOp {
public:
	OrOp (ExprTreePtr l, ExprTreePtr r) : BinaryOp (l, r) {}
	string opName () {return "||";}
	ExprKind kind () {return OR_OP;}
};

// and the unary operators share this
class UnaryOp : public ExprTree {

protected:

	ExprTreePtr child;

public:

	UnaryOp (ExprTreePtr childIn) {
		child = childIn;
	}

	ExprTreePtr getChild () {return child;}

	virtual ~UnaryOp () {}
};

class NotOp : public UnaryOp {
public:
	NotOp (ExprTreePtr c) : UnaryOp (c) {}
	string toString () {return "!(" + child->toString () + ")";}
	ExprKind kind () {return NOT_OP;}
};

class SumOp : public UnaryOp {
public:
	SumOp (ExprTreePtr c) : UnaryOp (c) {}
	string toString () {return "sum(" + child->toString () + ")";}
	ExprKind kind () {return SUM_OP;}
};

class AvgOp : public UnaryOp {
public:
	AvgOp (ExprTreePtr c) : UnaryOp (c) {}
	string toString () {return "avg(" + child->toString () + ")";}
	ExprKind kind () {return AVG_OP;}
};

#endif
