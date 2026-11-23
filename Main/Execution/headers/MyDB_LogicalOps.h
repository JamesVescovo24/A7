#ifndef LOG_OP_H
#define LOG_OP_H

#include "MyDB_Stats.h"
#include "MyDB_TableReaderWriter.h"
#include "ExprTree.h"

// create a smart pointer for database tables
using namespace std;
class LogicalOp;
class LogicalTableScan;
class LogicalJoin;
typedef shared_ptr <LogicalOp> LogicalOpPtr;

// this is a pure virtual class that corresponds to a particular relational operation to be run.
// A logical query plan is a tree of these: LogicalTableScans at the leaves, LogicalJoins above
class LogicalOp {

public:

	// the statistics (tuple count, distinct values per attribute) describing the output of this op
	virtual MyDB_StatsPtr getStats () = 0;

	// the table that this op produces; its schema is the op's output schema
	virtual MyDB_TablePtr getOutputTable () = 0;

	// print the logical op, including the entire tree
	void print () {
		vector <MyDB_TablePtr> outputs;
		print (0, outputs);
		cout << "\nAll tables created:\n";
		for (auto &a: outputs) {
			cout << "Table: " << a << "\n";
		}
	}

	// implemented by each subclass, to print
	virtual void print (int depth, vector <MyDB_TablePtr> &outputs) = 0;

	// what kind of op this is, and a way to get at the subclass.  These let code that is handed
	// a plan (a plan checker, a test) walk the tree without knowing how it was built
	virtual bool isTableScan () {return false;}
	virtual bool isJoin () {return false;}
	virtual LogicalTableScan *toTableScan () {return nullptr;}
	virtual LogicalJoin *toJoin () {return nullptr;}

	virtual ~LogicalOp () {}
};

// a logical aggregation operation--in practice, this is going to be implemented using an Aggregate operation,
// followed by a RegularSelection operation to de-scramble the output attributes (since the Aggregate always has
// the grouping atts first, followed by the aggregates, and this might not be the order that is given in
// exprsToCompute)... to be added in A8
class LogicalAggregate : public LogicalOp {

public:

private:

};

// a logical join operation---will be implemented with a ScanJoin or a SortMergeJoin
class LogicalJoin  : public LogicalOp {

public:

	//
	// leftInputOp: this is the input operation that we are reading from on the left
	// rightInputOp: this is the input operation that we are reading from on the right
	// outputSpec: this is the table that we are going to create by running the operation
	// outputSelectionPredicate: the selection predicates to execute using the join
	// outputStats: the satistics describing the relaation created by this join
	//
	LogicalJoin (LogicalOpPtr leftInputOp, LogicalOpPtr rightInputOp, MyDB_TablePtr outputSpec,
		vector <ExprTreePtr> &outputSelectionPredicate, MyDB_StatsPtr outputStats) : leftInputOp (leftInputOp),
		rightInputOp (rightInputOp), outputSpec (outputSpec), outputSelectionPredicate (outputSelectionPredicate),
		outputStats (outputStats) {}

	// print this tree out
	void print (int depth, vector <MyDB_TablePtr> &outputs) override {
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "****** JOIN returning " << outputStats->getTupleCount () << " tuples.\n";
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Output table: " << outputSpec->getName () << "\n";
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Predicates:\n";
		for (auto &a: outputSelectionPredicate) {
			for (int i = 0; i < depth; i++) cout << "  ";
			cout << "    " << a->toString () << "\n";
		}
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Left tree:\n";
		leftInputOp->print (depth + 1, outputs);
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Right tree:\n";
		rightInputOp->print (depth + 1, outputs);
		outputs.push_back (outputSpec);
	}

	MyDB_StatsPtr getStats () override {return outputStats;}
	MyDB_TablePtr getOutputTable () override {return outputSpec;}

	// the two inputs and the predicates applied by this join
	LogicalOpPtr getLeftInput () {return leftInputOp;}
	LogicalOpPtr getRightInput () {return rightInputOp;}
	vector <ExprTreePtr> &getPredicates () {return outputSelectionPredicate;}

	bool isJoin () override {return true;}
	LogicalJoin *toJoin () override {return this;}

private:

	LogicalOpPtr leftInputOp;
	LogicalOpPtr rightInputOp;
	MyDB_TablePtr outputSpec;
	vector <ExprTreePtr> outputSelectionPredicate;
	MyDB_StatsPtr outputStats;

};

// a logical table scan operation---will be implemented with a BPlusSelection or a RegularSelection... note that
// we assume that we only operate table scans over base tables, and not tables that are created as the result of
// running another logical operation
class LogicalTableScan : public LogicalOp {

public:

	//
	// inputSpec: this is the input table that we are operating over (already aliased: see MyDB_Table :: alias ())
	// outputSpec: this is the table that we are going to create by running the operation
	// outputStats: the complete set of output statistics
	// selectionPred: the selection predicates to execute while we scan the input
	//
	LogicalTableScan (MyDB_TablePtr inputSpec, MyDB_TablePtr outputSpec, MyDB_StatsPtr outputStats,
		vector <ExprTreePtr> &selectionPred) : inputSpec (inputSpec), outputSpec (outputSpec),
		outputStats (outputStats), selectionPred (selectionPred) {}

	// print this tree out
	void print (int depth, vector <MyDB_TablePtr> &outputs) override {
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "****** TABLE SCAN returning " << outputStats->getTupleCount () << " tuples.\n";
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Output table: " << outputSpec->getName () << "\n";
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Input table: " << inputSpec->getName () << "\n";
		for (int i = 0; i < depth; i++) cout << "  ";
		cout << "  ** Predicates:\n";
		for (auto &a: selectionPred) {
			for (int i = 0; i < depth; i++) cout << "  ";
			cout << "    " << a->toString () << "\n";
		}
		outputs.push_back (outputSpec);
		outputs.push_back (inputSpec);
	}

	MyDB_StatsPtr getStats () override {return outputStats;}
	MyDB_TablePtr getOutputTable () override {return outputSpec;}

	// the (aliased) input table and the predicates applied by this scan
	MyDB_TablePtr getInputTable () {return inputSpec;}
	string getInputTableName () {return inputSpec->getName ();}
	vector <ExprTreePtr> &getPredicates () {return selectionPred;}

	bool isTableScan () override {return true;}
	LogicalTableScan *toTableScan () override {return this;}

private:

	MyDB_TablePtr inputSpec;
	MyDB_TablePtr outputSpec;
	MyDB_StatsPtr outputStats;
        vector <ExprTreePtr> selectionPred;
};

#endif
