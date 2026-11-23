
#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <iostream>
#include <stdlib.h>
#include "ExprTree.h"
#include "MyDB_LogicalOps.h"
#include "MyDB_Catalog.h"
#include "MyDB_Schema.h"
#include "MyDB_Table.h"
#include <string>
#include <utility>

using namespace std;

/*************************************************/
/** HERE WE DEFINE ALL OF THE STRUCTS THAT ARE **/
/** PASSED AROUND BY THE PARSER                **/
/*************************************************/

// structure that encapsulates a parsed computation that returns a value
struct Value {

private:

        // this points to the expression tree that computes this value
        ExprTreePtr myVal;

public:
        ~Value () {}

        Value (ExprTreePtr useMe) {
                myVal = useMe;
        }

        Value () {
                myVal = nullptr;
        }
	
	friend struct CNF;
	friend struct ValueList;
	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// structure that encapsulates a parsed CNF computation
struct CNF {

private:

        // this points to the expression tree that computes this value
        vector <ExprTreePtr> disjunctions;

public:
        ~CNF () {}

        CNF (struct Value *useMe) {
              	disjunctions.push_back (useMe->myVal); 
        }

        CNF () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// structure that encapsulates a parsed list of value computations
struct ValueList {

private:

        // this points to the expression tree that computes this value
        vector <ExprTreePtr> valuesToCompute;

public:
        ~ValueList () {}

        ValueList (struct Value *useMe) {
              	valuesToCompute.push_back (useMe->myVal); 
        }

        ValueList () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};


// structure to encapsulate a create table
struct CreateTable {

private:

	// the name of the table to create
	string tableName;

	// the list of atts to create... the string is the att name
	vector <pair <string, MyDB_AttTypePtr>> attsToCreate;

	// true if we create a B+-Tree
	bool isBPlusTree;

	// the attribute to organize the B+-Tree on
	string sortAtt;

public:
	string addToCatalog (string storageDir, MyDB_CatalogPtr addToMe) {

		// make the schema
		MyDB_SchemaPtr mySchema = make_shared <MyDB_Schema>();
		for (auto a : attsToCreate) {
			mySchema->appendAtt (a);
		}

		// now, make the table
		MyDB_TablePtr myTable;

		// just a regular file
		if (!isBPlusTree) {
			myTable =  make_shared <MyDB_Table> (tableName, 
				storageDir + "/" + tableName + ".bin", mySchema);	

		// creating a B+-Tree
		} else {
			
			// make sure that we have the attribute
			if (mySchema->getAttByName (sortAtt).first == -1) {
				cout << "B+-Tree not created.\n";
				return "nothing";
			}
			myTable =  make_shared <MyDB_Table> (tableName, 
				storageDir + "/" + tableName + ".bin", mySchema, "bplustree", sortAtt);	
		}

		// and add to the catalog
		myTable->putInCatalog (addToMe);

		return tableName;
	}

	CreateTable () {}

	CreateTable (string tableNameIn, vector <pair <string, MyDB_AttTypePtr>> atts) {
		tableName = tableNameIn;
		attsToCreate = atts;
		isBPlusTree = false;
	}

	CreateTable (string tableNameIn, vector <pair <string, MyDB_AttTypePtr>> atts, string sortAttIn) {
		tableName = tableNameIn;
		attsToCreate = atts;
		isBPlusTree = true;
		sortAtt = sortAttIn;
	}
	
	~CreateTable () {}

	#include "FriendDecls.h"
};

// structure that stores a list of attributes
struct AttList {

private:

	// the list of attributes
	vector <pair <string, MyDB_AttTypePtr>> atts;

public:
	AttList (string attName, MyDB_AttTypePtr whichType) {
		atts.push_back (make_pair (attName, whichType));
	}

	~AttList () {}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// one thing in the FROM clause: either a single table with its alias, or a
// join of two FromItems with an ON condition.  So a FROM clause like
//    a AS a, (b AS b JOIN c AS c ON b.x = c.x) JOIN d AS d ON c.y = d.y
// is a list of two FromItems; the second is a join whose left side is itself
// a join
struct FromItem;
typedef shared_ptr <FromItem> FromItemPtr;

struct FromItem {

private:

	// true if this is a single table
	bool isTable;

	// if a table: the table's name and its alias
	string tableName;
	string alias;

	// if a join: the two sides, and the ON condition (a list of disjunctions,
	// ANDed together, just like a WHERE clause)
	FromItemPtr lhs;
	FromItemPtr rhs;
	vector <ExprTreePtr> onDisjunctions;

public:

	FromItem (string tableNameIn, string aliasIn) {
		isTable = true;
		tableName = tableNameIn;
		alias = aliasIn;
	}

	FromItem (FromItem *lhsIn, FromItem *rhsIn, vector <ExprTreePtr> &onIn) {
		isTable = false;
		lhs = FromItemPtr (lhsIn);
		rhs = FromItemPtr (rhsIn);
		onDisjunctions = onIn;
	}

	~FromItem () {}

	bool isSingleTable () {return isTable;}
	string getTableName () {return tableName;}
	string getAlias () {return alias;}
	FromItemPtr getLHS () {return lhs;}
	FromItemPtr getRHS () {return rhs;}
	vector <ExprTreePtr> &getOnDisjunctions () {return onDisjunctions;}

	// collects every (table, alias) pair under this item, left to right
	void collectTables (vector <pair <string, string>> &into) {
		if (isTable) {
			into.push_back (make_pair (tableName, alias));
		} else {
			lhs->collectTables (into);
			rhs->collectTables (into);
		}
	}

	// collects every ON disjunction under this item
	void collectOnDisjunctions (vector <ExprTreePtr> &into) {
		if (!isTable) {
			lhs->collectOnDisjunctions (into);
			rhs->collectOnDisjunctions (into);
			for (auto a : onDisjunctions)
				into.push_back (a);
		}
	}

	string toString () {
		if (isTable)
			return tableName + " AS " + alias;
		string res = "(" + lhs->toString () + " JOIN " + rhs->toString () + " ON";
		bool first = true;
		for (auto a : onDisjunctions) {
			res += (first ? " " : " AND ") + a->toString ();
			first = false;
		}
		return res + ")";
	}

	friend struct SFWQuery;
	#include "FriendDecls.h"
};

struct FromList {

private:

	// the list of items in the FROM clause
	vector <FromItemPtr> items;

public:
	FromList (FromItemPtr item) {
		items.push_back (item);
	}

	FromList () {}

	~FromList () {}
	
	friend struct SFWQuery;
	#include "FriendDecls.h"
};

// structure that stores an entire SFW query
#include "SFWQuery.h"

// structure that stores an entire SQL statement
struct SQLStatement {

private:

	// in case we are a SFW query
	SFWQuery myQuery;
	bool isQuery;

	// in case we a re a create table
	CreateTable myTableToCreate;
	bool isCreate;

public:
	SQLStatement (struct SFWQuery* useMe) {
		myQuery = *useMe;
		isQuery = true;
		isCreate = false;
	}

	SQLStatement (struct CreateTable *useMe) {
		myTableToCreate = *useMe;
		isQuery = false;
		isCreate = true;
	}

	bool isCreateTable () {
		return isCreate;
	}

	bool isSFWQuery () {
		return isQuery;
	}

	string addToCatalog (string storageDir, MyDB_CatalogPtr addToMe) {
		return myTableToCreate.addToCatalog (storageDir, addToMe);
	}		
	
	void printSFWQuery () {
		myQuery.print ();
	}

	// semantic check; "" if OK, else the error message
	string check (map <string, MyDB_TablePtr> &allTables) {
		return myQuery.check (allTables);
	}

	// builds the best logical plan for the query (see SFWQuery.h)
	pair <LogicalOpPtr, double> optimizeQuery (map <string, MyDB_TablePtr> &allTables) {
		return myQuery.optimizeQueryPlan (allTables);
	}

	// checks a plan for the query (see SFWQuery.h)
	bool checkPlan (LogicalOpPtr plan, double claimedCost, map <string, MyDB_TablePtr> &allTables) {
		return myQuery.checkPlan (plan, claimedCost, allTables);
	}

	// the query itself, for tests that want to look inside
	SFWQuery &getQuery () {
		return myQuery;
	}

	#include "FriendDecls.h"
};

#endif
