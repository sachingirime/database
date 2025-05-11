#ifndef _REL_OP_H
#define _REL_OP_H

#include <iostream>

#include "Schema.h"
#include "Record.h"
#include "DBFile.h"
#include "Function.h"
#include "Comparison.h"
#include <list>
#include <unordered_map>
#include "Map.h"
#include "BTreeIndex.h"

using namespace std;
//project2 = implement constructor, destructor and printing for each class - scan, select, project, join, duplicate removal, sum, group by, write out
// in RelOp.cc file

class RelationalOp {
protected:
	// the number of pages that can be used by the operator in execution
	int noPages;
public:
	// empty constructor & destructor
	RelationalOp() : noPages(-1) {}
	virtual ~RelationalOp() {}

	// set the number of pages the operator can use
	void SetNoPages(int _noPages) {noPages = _noPages;}

	// every operator has to implement this method
	virtual bool GetNext(Record& _record) = 0;

	/* Virtual function for polymorphic printing using operator<<.
	 * Each operator has to implement its specific version of print.
	 */
    virtual ostream& print(ostream& _os) = 0;

    /* Overload operator<< for printing.
     */
    friend ostream& operator<<(ostream& _os, RelationalOp& _op);
};

class Scan : public RelationalOp {
private:
	// schema of records in operator
	Schema schema;

	// physical file where data to be scanned are stored
	DBFile file;

	// table name used in the query
	string tblName; 

public:
	Scan(Schema& _schema, DBFile& _file, string _tblName);
	virtual ~Scan();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class Select : public RelationalOp {
private:
	// schema of records in operator
	Schema schema;

	// selection predicate in conjunctive normal form
	CNF predicate;
	// constant values for attributes in predicate
	Record constants;

	// operator generating data
	RelationalOp* producer;

public:
	Select(Schema& _schema, CNF& _predicate, Record& _constants,
		RelationalOp* _producer);
	virtual ~Select();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class ScanIndex : public RelationalOp {
	private:
		Schema schema;
		CNF predicate;
		Record constants;
	
		string tableName;
		BTreeIndex index;
		File dataFile;
	
		bool initialized;
		vector<int> matchingRecordIds;
		size_t currentIndex;
	
	public:
		ScanIndex(Schema& _schema,
				  CNF& _predicate,
				  Record& _constants,
				  const string& _tableName,
				  const string& indexFileName,
				  Catalog* catalog);
	
		virtual ~ScanIndex() = default;
	
		bool GetNext(Record& _record) override;
		ostream& print(ostream& _os) override;
		void Reset();
	};

class Project : public RelationalOp {
private:
	// schema of records input to operator
	Schema schemaIn;
	// schema of records output by operator
	Schema schemaOut;

	// number of attributes in input records
	int numAttsInput;
	// number of attributes in output records
	int numAttsOutput;
	// index of records from input to keep in output
	// size given by numAttsOutput
	int* keepMe;

	// operator generating data
	RelationalOp* producer;

public:
	Project(Schema& _schemaIn, Schema& _schemaOut, int _numAttsInput,
		int _numAttsOutput, int* _keepMe, RelationalOp* _producer);
	virtual ~Project();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class Join : public RelationalOp {
protected:
	// schema of records in left operand
	Schema schemaLeft;
	// schema of records in right operand
	Schema schemaRight;
	// schema of records output by operator
	Schema schemaOut;

	// selection predicate in conjunctive normal form
	CNF predicate;

	// operators generating data
	RelationalOp* left;
	RelationalOp* right;

public:
	Join(Schema& _schemaLeft, Schema& _schemaRight, Schema& _schemaOut,
		CNF& _predicate, RelationalOp* _left, RelationalOp* _right);
	virtual ~Join();

	virtual bool GetNext(Record& _record) = 0;

	virtual ostream& print(ostream& _os);
};

class NestedLoopJoin : public Join {
protected:
	RecordList list;
	Record lRec;

public:
	NestedLoopJoin(Schema& _schemaLeft, Schema& _schemaRight, Schema& _schemaOut,
		CNF& _predicate, RelationalOp* _left, RelationalOp* _right);

	virtual ~NestedLoopJoin();

	virtual bool GetNext(Record& _record);
};

// --------- HashJoin -----------------------------------------------------
class HashJoin : public Join {
	protected:
		OrderMaker leftOrder;
		OrderMaker rightOrder;
	
	public:
		HashJoin(Schema& _schemaLeft,
				 Schema& _schemaRight,
				 Schema& _schemaOut,
				 CNF& _predicate,
				 RelationalOp* _left,
				 RelationalOp* _right,
				 const OrderMaker& _leftOrder,
				 const OrderMaker& _rightOrder);
		virtual ~HashJoin() ;
		virtual bool GetNext(Record& _record);
	};
	
	// --------- SymmetricHashJoin ---------------------------------------------
	class SymmetricHashJoin : public Join {
		protected:
			OrderMaker            leftOrder;
			OrderMaker            rightOrder;

		
		public:
			SymmetricHashJoin(
				Schema& _schemaLeft,
				Schema& _schemaRight,
				Schema& _schemaOut,
				CNF& _predicate,
				RelationalOp* _left,
				RelationalOp* _right,
				const OrderMaker& _leftOrder,
				const OrderMaker& _rightOrder
			);
		virtual ~SymmetricHashJoin() ;
		virtual bool GetNext(Record& _record) ;
	};


class DuplicateRemoval : public RelationalOp {
private:
	// schema of records in operator
	Schema schema;

	// operator generating data
	RelationalOp* producer;

public:
	DuplicateRemoval(Schema& _schema, RelationalOp* _producer);
	virtual ~DuplicateRemoval();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class Sum : public RelationalOp {
private:
	// schema of records input to operator
	Schema schemaIn;
	// schema of records output by operator
	Schema schemaOut;

	// function to compute
	Function compute;

	// operator generating data
	RelationalOp* producer;

public:
	Sum(Schema& _schemaIn, Schema& _schemaOut, Function& _compute,
		RelationalOp* _producer);
	virtual ~Sum();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class GroupBy : public RelationalOp {
private:
	// schema of records input to operator
	Schema schemaIn;
	// schema of records output by operator
	Schema schemaOut;

	// grouping attributes
	OrderMaker groupingAtts;
	// function to compute
	Function compute;

	// operator generating data
	RelationalOp* producer;

public:
	GroupBy(Schema& _schemaIn, Schema& _schemaOut, OrderMaker& _groupingAtts,
		Function& _compute,	RelationalOp* _producer);
	virtual ~GroupBy();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};

class WriteOut : public RelationalOp {
private:
	// schema of records in operator
	Schema schema;

	// output file where to write the result records
	string outFile;

	// operator generating data
	RelationalOp* producer;

public:
	WriteOut(Schema& _schema, string& _outFile, RelationalOp* _producer);
	virtual ~WriteOut();

	virtual bool GetNext(Record& _record);

	virtual ostream& print(ostream& _os);
};


class QueryExecutionTree {
private:
	RelationalOp* root;

public:
	QueryExecutionTree() : root(0) {}
	virtual ~QueryExecutionTree() {}

	void SetRoot(RelationalOp& _root) {root = &_root;}
	RelationalOp& GetRoot() {return *root;}

	void ExecuteQuery();

    friend ostream& operator<<(ostream& _os, QueryExecutionTree& _op);
};

#endif //_REL_OP_H
