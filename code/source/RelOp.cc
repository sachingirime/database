#include <iostream>
#include <cstring>
#include <sstream>
#include "RelOp.h"
#include<fstream>
#include <map>     // for std::map
#include <vector>   // For std::vector in Group by, for string and int types

#include <deque>
#include <list>  // include the standard list header
#include <unordered_map>
#include "Map.h"      // for class Map<…> and KeyInt

#include <unordered_set>





#include "Map.cc"


using namespace std;


// Deep copy a Record: allocates new bits and assigns to dst
// Attempts to deep copy a Record. Returns false if src is null or uninitialized.
bool DeepCopyRecordSafe(Record& src, Record& dst) {
    char* bits = src.GetBits();
    if (!bits) {
        // Defensive warning: record is uninitialized
        cerr << "[WARN] DeepCopyRecordSafe: Tried to copy from null Record. Skipping." << endl;
        return false;
    }

    int len = ((int*)bits)[0];
    char* copy = new char[len];
    memcpy(copy, bits, len);
    dst.Consume(copy);
    return true;
}

// bool DeepCopyRecordSafe(Record& src, Record& dst) {
//     if (src.GetBits() == nullptr) return false;
//     dst.CopyBits(src.GetBits(), src.GetSize());
//     return true;
// }




// Utility function to replace the first occurrence of a substring with another string.
// Returns true if a replacement occurred, false otherwise.
bool modifyString(std::string& inputStr, const std::string& searchStr, const std::string& replaceStr) {
    // Avoid processing an empty search string.
    if (searchStr.empty()) {
        return false;
    }

    // Locate the position of the first occurrence of the search string.
    size_t foundPos = inputStr.find(searchStr);
    if (foundPos == std::string::npos) {
        return false;
    }

    // Build a new string by concatenating the parts before and after the found substring with the replacement.
    std::string newStr = inputStr.substr(0, foundPos) + replaceStr + inputStr.substr(foundPos + searchStr.length());

    // Update the original string.
    inputStr = newStr;

    return true;
}


ostream& operator<<(ostream& _os, RelationalOp& _op) {
	return _op.print(_os);
}


Scan::Scan(Schema& _schema, DBFile& _file, string _tblName) {
	// implement the scan
	schema = _schema;
	file = _file;
	tblName = _tblName;
	file.MoveFirst();

}

Scan::~Scan() {

}

bool Scan::GetNext(Record& _record) {
	//file.MoveFirst();
	return file.GetNext(_record);
		//cout << "INSIDE SCAN FILE.GetNext() --> " << file.GetNext(_record) <<endl;




}

ostream& Scan::print(ostream& _os) {
	_os << "SCAN " << tblName << " {" << schema << "}";
	return _os;
}


Select::Select(Schema& _schema, CNF& _predicate, Record& _constants,
	RelationalOp* _producer) {
		//implement the select
		schema = _schema;
		predicate = _predicate;
		constants = _constants;
		producer = _producer;
}

Select::~Select() {
	//delete producer;


}

bool Select::GetNext(Record& _record) {
	while (true) {
		bool ret = producer->GetNext(_record);
		if (false == ret) return false;

    // check the condition or evaluate the predicate
		if (true == predicate.Run(_record, constants)) return true;
	}
	return false;
	// if(producer->GetNext(_record)){return true;}
	// return false;
}

// Helper: Convert a constant from a record into its string representation.
std::string convertConstantToStr(Record& constants, Type attType, int index) {
    std::string result;
    if (attType == Integer) {
        // Retrieve the integer value and convert it to string.
        int value = *reinterpret_cast<int*>(constants.GetColumn(index));
        result = std::to_string(value);
    } else if (attType == String) {
        // Wrap the string value in single quotes.
        result = std::string("'") + constants.GetColumn(index) + "'";
    } else if (attType == Float) {
        // Retrieve the floating-point value and convert it to string.
        double value = *reinterpret_cast<double*>(constants.GetColumn(index));
        result = std::to_string(value);
    }
    return result;
}

// Helper: Convert a comparison operator into its corresponding string.
std::string convertOperatorToStr(CompOperator op) {
    std::string opStr;
    if (op == Equals) {
        opStr = " = ";
    } else if (op == LessThan) {
        opStr = " &lt; ";
    } else if (op == GreaterThan) {
        opStr = " &gt; ";
    }
    return opStr;
}


ostream& Select::print(ostream& _os) {
	// Generate the node's identifier using the Greek letter sigma and embed the predicate.
stringstream node_name;
node_name << "\"\u03C3_{" << predicate << "}\"";

// Lambda to convert a single comparison into a string representation.
auto convertComparison = [&] (const Comparison &comp, int idx) -> std::string {
    std::string result;
    // For the first operand, ensure both outcomes are std::string.
    result += (comp.operand1 != Literal)
                ? std::string(schema.GetAtts()[comp.whichAtt1].name)
                : convertConstantToStr(constants, comp.attType, idx);
    // Append the operator string.
    result += convertOperatorToStr(comp.op);
    // For the second operand, similarly convert attribute name to std::string if needed.
    result += (comp.operand2 != Literal)
                ? std::string(schema.GetAtts()[comp.whichAtt2].name)
                : convertConstantToStr(constants, comp.attType, idx);
    return result;
};


// Build a human-readable predicate string from the CNF structure.
string predicates;
if (predicate.numAnds > 0) {
    // Process the first comparison.
    predicates = convertComparison(predicate.andList[0], 0);
    // Process any additional comparisons, concatenating them with " AND ".
    for (int i = 1; i < predicate.numAnds; i++) {
        predicates += " AND " + convertComparison(predicate.andList[i], i);
    }
}

// Form the label for the Graphviz node using subscript formatting.
string label = "<\u03C3<SUB>" + predicates + "</SUB>>";
// Output the node definition to the Graphviz dot file with plaintext shape.
//queryGraphDotFile << node_name.str() << "[label=" << label << ", shape=plaintext]" << endl;

// Replace HTML-escaped tokens with the actual characters for terminal display.
modifyString(predicates, "&lt;", "<");
modifyString(predicates, "&gt;", ">");

// Print the SELECT operation details to the terminal.
_os << "SELECT (" << predicates << " ) {" << schema << "} " << *producer << endl;

	return _os;
}


Project::Project(Schema& _schemaIn, Schema& _schemaOut, int _numAttsInput,
	int _numAttsOutput, int* _keepMe, RelationalOp* _producer) {
		schemaIn = _schemaIn;
		schemaOut = _schemaOut;
		numAttsInput = _numAttsInput;
		numAttsOutput = _numAttsOutput;
		keepMe = _keepMe;
		producer = _producer;


}

Project::~Project() {
 delete[] keepMe;
    //delete producer;
    delete producer;
}


bool Project::GetNext(Record& _record) {
	if (producer->GetNext(_record)){
		_record.Project(keepMe,numAttsOutput,numAttsInput);

		return true;

	}
	return false;
}

ostream& Project::print(ostream& _os) {

	// Create a comma-separated string of attribute names from schemaOut.
		string atts;
		int totalAtts = schemaOut.GetAtts().Length();
		if (totalAtts > 0) {
			// Start with the first attribute to avoid a leading comma.
			atts = schemaOut.GetAtts()[0].name;
			// Append the remaining attribute names, each preceded by a comma and space.
			for (int i = 1; i < totalAtts; ++i) {
				atts += ", " + std::string(schemaOut.GetAtts()[i].name);
			}
		}

		// Assemble the node identifier using the Unicode character for π.
		stringstream node_name;
		node_name << "\"\u03C0_{" << atts << "}\"";

		// Construct the label using subscript formatting to highlight the attribute list.
		string label = "<\u03C0<SUB>" + atts + "</SUB>>";

		// Write the node information to the Graphviz dot file with a plaintext shape.
		//queryGraphDotFile << node_name.str() << "[label=" << label << ", shape=plaintext]" << endl;

		// Display the PROJECT operation along with its schema details on the terminal.
		_os << "PROJECT {" << schemaOut << "} " << *producer << endl;



		return _os;
}


Join::Join(Schema& _schemaLeft, Schema& _schemaRight, Schema& _schemaOut,
	CNF& _predicate, RelationalOp* _left, RelationalOp* _right) {
		schemaLeft = _schemaLeft;
		schemaRight = _schemaRight;
		schemaOut = _schemaOut;
		predicate = _predicate;
		left = _left;
		right = _right;

}

Join::~Join() {
    delete left;
    delete right;



}

ostream& Join::print(ostream& _os) {

		// Establish the node identifier for the join operation using the bowtie symbol.
	stringstream node_name;

	// Build the join predicate string by iterating through each comparison in the predicate.
	string predicates;
	for (int i = 0; i < predicate.numAnds; i++) {
		int left_att_position, right_att_position;
		// Determine which operand comes from the right relation.
		if (predicate.andList[i].operand1 == Right) {
			right_att_position = predicate.andList[i].whichAtt1;
			left_att_position = predicate.andList[i].whichAtt2;
		} else {
			right_att_position = predicate.andList[i].whichAtt2;
			left_att_position = predicate.andList[i].whichAtt1;
		}

		// Retrieve attribute names from the corresponding schemas.
		string left_name = schemaLeft.GetAtts()[left_att_position].name;
		string right_name = schemaRight.GetAtts()[right_att_position].name;

		// Formulate the condition string for this comparison.
		string condition = left_name + " = " + right_name;

		// Append conditions together, separated by " AND " if not the first.
		if (i > 0) {
			predicates += " AND ";
		}
		predicates += condition;
	}

	// Set up the node's identifier for the dot file using the bowtie symbol (Unicode U+22C8).
	node_name << "\"\u22C8_{" << predicates << "}\"";

	// Create a label for the node with subscript formatting displaying the predicate.
	string label = "<\u22C8<SUB>" + predicates + "</SUB>>";

	// Output the node to the Graphviz dot file.
	//queryGraphDotFile << node_name.str() << "[label=" << label << ", shape=plaintext]" << endl;

	// Replace HTML escape sequences with their literal characters for terminal output.
	modifyString(predicates, "&lt;", "<");
	modifyString(predicates, "&gt;", ">");

	// Print out the join operator details to the terminal.
	_os << "JOIN (" << predicates << ") " << schemaOut << " { Left: " << *left
		<< " Right: " << *right << " }";


    return _os;
}






NestedLoopJoin::NestedLoopJoin(Schema& _schemaLeft, Schema& _schemaRight, Schema& _schemaOut,
	CNF& _predicate, RelationalOp* _left, RelationalOp* _right)
	: Join(_schemaLeft, _schemaRight, _schemaOut, _predicate, _left, _right) {

}

NestedLoopJoin::~NestedLoopJoin() {
    delete left;
    delete right;

}

bool NestedLoopJoin::GetNext(Record& _record) {
    // per‑instance storage keyed by this pointer
    static std::unordered_map<const NestedLoopJoin*, RecordList> listMap;
    static std::unordered_map<const NestedLoopJoin*, bool>       doneMap;
    static std::unordered_map<const NestedLoopJoin*, Record>     leftMap;

    RecordList &list    = listMap[this];
    bool        firstTime = !doneMap[this];
    Record     &lRec     = leftMap[this];
    //     static RecordList list;
    // static bool firstTime = true;
    // static Record lRec;

    // BUILD PHASE: load entire right side once
    if (firstTime) {
        std::cout << "[NestedLoopJoin] Entering build phase." << std::endl;
        Record temp;
        while (right->GetNext(temp)) {
            list.Append(temp);
        }
        doneMap[this] = true;
        if (!left->GetNext(lRec)) {
            return false;
        }
        list.MoveToStart();
    }

    // PROBE PHASE
    while (true) {
        // scan inner list for this outer tuple
        while (!list.AtEnd()) {
            Record &rRec = list.Current();

            if (predicate.Run(lRec, rRec)) {
                // we found the first match for this outer tuple
                list.Advance();
               // cout << "match found" << endl;
                int nL = schemaLeft .GetNumAtts();
                int nR = schemaRight.GetNumAtts();
              //  std::cout << "num attributes left --> " << nL << std::endl;
//                 int numrightAtts = schemaRight.GetNumAtts();
              //  std::cout << "num attributes right --> " << nR << std::endl;
               // std::cout << schemaLeft <<endl;
                //std::cout << schemaRight <<endl;
               // std::cout << "Schemas printed above -- left and right record below" << endl;
                lRec.print(std::cout, schemaLeft);
               // std::cout << "Record  left above" << endl;
                rRec.print(std::cout, schemaRight);
              //  std::cout << "Record  right above" << endl;

                _record.AppendRecords(lRec, rRec, nL, nR);
                _record.print(std::cout, schemaOut);


                return true;
            }
            list.Advance();
        }

        // no match for this outer tuple, advance to next
        if (!left->GetNext(lRec)) {
            return false;
        }
        list.MoveToStart();
    }
}


  //------------------------------------------------------------------------------


// --------- HashJoin -----------------------------------------------------

HashJoin::HashJoin(Schema& _schemaLeft,
    Schema& _schemaRight,
    Schema& _schemaOut,
    CNF& _predicate,
    RelationalOp* _left,
    RelationalOp* _right,
    const OrderMaker& _leftOrder,
    const OrderMaker& _rightOrder)
: Join(_schemaLeft, _schemaRight, _schemaOut, _predicate, _left, _right),
leftOrder(_leftOrder),
rightOrder(_rightOrder)
{}

HashJoin::~HashJoin() {
// The base Join destructor already deletes left & right
delete left;
delete right;
}

bool HashJoin::GetNext(Record& outputRec) {
    // Per-instance state
    static std::unordered_map<const HashJoin*, bool> builtMap;
    static std::unordered_map<const HashJoin*, Map<Record, List<Record>>> hashMap;
    static std::unordered_map<const HashJoin*, Record> probeMap;
    static std::unordered_map<const HashJoin*, List<Record>*> matchListMap;
    static std::unordered_map<const HashJoin*, bool> inMatchingMap;

    bool& built = builtMap[this];
    Map<Record, List<Record>>& map = hashMap[this];
    Record& leftRec = probeMap[this];
    List<Record>*& matchList = matchListMap[this];
    bool& inMatching = inMatchingMap[this];

    // Emit buffered matches from previous GetNext call
    if (inMatching && matchList && !matchList->AtEnd()) {
        Record& rightRec = matchList->Current();
        matchList->Advance();
        int nl = schemaLeft.GetNumAtts();
        int nr = schemaRight.GetNumAtts();
        outputRec.AppendRecords(leftRec, rightRec, nl, nr);
        return true;
    }

    inMatching = false;

    // Build hash table from right side (only once)
    if (!built) {
        Record temp;
        while (right->GetNext(temp)) {
            temp.SetOrderMaker(&rightOrder);

            Record key;
            key.CopyBits(temp.GetBits(), temp.GetSize());
            key.SetOrderMaker(&rightOrder);

            if (map.IsThere(key)) {
                map.CurrentData().Append(temp);
            } else {
                List<Record> lst;
                lst.Append(temp);
                map.Insert(key, lst);
            }
        }
        built = true;
    }

    // Probe left side
    while (left->GetNext(leftRec)) {
        leftRec.SetOrderMaker(&leftOrder);

        if (map.IsThere(leftRec)) {
            matchList = &map.CurrentData();
            matchList->MoveToStart();
            inMatching = true;

            // Emit first match now
            Record& rightRec = matchList->Current();
            matchList->Advance();

            int nl = schemaLeft.GetNumAtts();
            int nr = schemaRight.GetNumAtts();
            outputRec.AppendRecords(leftRec, rightRec, nl, nr);
            return true;
        }
    }

    return false;  // no more matches
}





// --------- SymmetricHashJoin ---------------------------------------------

SymmetricHashJoin::SymmetricHashJoin(Schema& _schemaLeft,
                      Schema& _schemaRight,
                      Schema& _schemaOut,
                      CNF& _predicate,
                      RelationalOp* _left,
                      RelationalOp* _right,
                      const OrderMaker& _leftOrder,
                      const OrderMaker& _rightOrder)
: Join(_schemaLeft, _schemaRight, _schemaOut, _predicate, _left, _right),
leftOrder(_leftOrder),
rightOrder(_rightOrder)
{}

SymmetricHashJoin::~SymmetricHashJoin() {
    delete left;
    delete right;
// The base Join destructor handles deleting children
}

bool SymmetricHashJoin::GetNext(Record& outputRec) {
    const int BATCH_SIZE = 200;

    static unordered_map<const SymmetricHashJoin*, Map<Record, List<Record>>> lMap, rMap;
    static unordered_map<const SymmetricHashJoin*, bool> lDoneMap, rDoneMap;
    static unordered_map<const SymmetricHashJoin*, int> sideMap;
    static unordered_map<const SymmetricHashJoin*, bool> inMatchingMap;
    static unordered_map<const SymmetricHashJoin*, List<Record>*> matchListMap;
    static unordered_map<const SymmetricHashJoin*, Record> probeMap;
    static unordered_map<const SymmetricHashJoin*, bool> probingLeftMap;

    auto& lHash = lMap[this];
    auto& rHash = rMap[this];
    bool& lDone = lDoneMap[this];
    bool& rDone = rDoneMap[this];
    int& whichSide = sideMap[this]; // 0 = LEFT, 1 = RIGHT
    bool& inMatching = inMatchingMap[this];
    List<Record>*& matchList = matchListMap[this];
    Record& probe = probeMap[this];
    bool& probingLeft = probingLeftMap[this];

    // Emit any remaining matches
    if (inMatching && matchList && !matchList->AtEnd()) {
        Record& match = matchList->Current();
        matchList->Advance();

        int nLeft = schemaLeft.GetNumAtts();
        int nRight = schemaRight.GetNumAtts();
        outputRec.AppendRecords(probingLeft ? probe : match, probingLeft ? match : probe, nLeft, nRight);
        return true;
    }
    inMatching = false;

    // Pull from alternating side
    while (true) {
        whichSide = 1 - whichSide;
        int loaded = 0;

        while (loaded < BATCH_SIZE) {
            Record rec;
            if ((whichSide == 0 && (lDone || !left->GetNext(rec))) ||
                (whichSide == 1 && (rDone || !right->GetNext(rec)))) {
                if (whichSide == 0) lDone = true;
                else rDone = true;
                break;
            }

            rec.SetOrderMaker(whichSide == 0 ? &leftOrder : &rightOrder);
            Record key;
            key.CopyBits(rec.GetBits(), rec.GetSize());
            key.SetOrderMaker(whichSide == 0 ? &leftOrder : &rightOrder);

            Map<Record, List<Record>>& probeMap = (whichSide == 0 ? rHash : lHash);
            // if (probeMap.IsThere(key)) {
            //     matchList = &probeMap.CurrentData();
            //     matchList->MoveToStart();
            //     probe.CopyBits(rec.GetBits(), rec.GetSize());
            //     probe.SetOrderMaker(whichSide == 0 ? &leftOrder : &rightOrder);
            //     probingLeft = (whichSide == 0);
            //     inMatching = true;
            //     return GetNext(outputRec);
            // }

            if (probeMap.IsThere(key)) {
                matchList = &probeMap.CurrentData();
                matchList->MoveToStart();
                probe.CopyBits(rec.GetBits(), rec.GetSize());
                probe.SetOrderMaker(whichSide == 0 ? &leftOrder : &rightOrder);
                probingLeft = (whichSide == 0);
                inMatching = true;
            
                // Emit the first match now
                if (matchList && !matchList->AtEnd()) {
                    Record& match = matchList->Current();
                    matchList->Advance();
            
                    int nLeft = schemaLeft.GetNumAtts();
                    int nRight = schemaRight.GetNumAtts();
                    outputRec.AppendRecords(probingLeft ? probe : match,
                                            probingLeft ? match : probe,
                                            nLeft, nRight);
                    return true;
                }
            }
            

            Map<Record, List<Record>>& insertMap = (whichSide == 0 ? lHash : rHash);
            if (insertMap.IsThere(key)) {
                insertMap.CurrentData().Append(rec);
            } else {
                List<Record> lst;
                lst.Append(rec);
                insertMap.Insert(key, lst);
            }

            loaded++;
        }

        if (lDone && rDone) return false;
    }
}




DuplicateRemoval::DuplicateRemoval(Schema& _schema, RelationalOp* _producer) {
	schema = _schema;
	producer = _producer;


}

DuplicateRemoval::~DuplicateRemoval() {



}


bool DuplicateRemoval::GetNext(Record& _record) {
    // Create a local alias for the vector to avoid fully qualified names.
    typedef std::vector<Record*> RecordVector;

    // The following static variables persist across calls within one query.
    static RecordVector uniqueRecords;
    static size_t currentIndex = 0;
    static bool accumulated = false;

    // ------------------------------
    // Phase 1: Accumulation Phase
    // ------------------------------
    if (!accumulated) {
        // Reset (clear any leftover state from previous queries).
        uniqueRecords.clear();
        currentIndex = 0;

        // Use an OrderMaker for comparing records
        OrderMaker order(schema);
        Record temp;
        // Loop to drain the producer; DBFile or prior operator will output a record each call.
        while (producer->GetNext(temp)) {
            // Set the order maker on the incoming record so that comparison works.
            temp.SetOrderMaker(&order);
            bool duplicate = false;
            // Compare with all previously accumulated records.
            for (size_t i = 0; i < uniqueRecords.size(); i++) {
                if (order.Run(*uniqueRecords[i], temp) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                // Deep-copy the record from temp.
                Record* copy = new Record();
                char* bits = temp.GetBits();
                int len = ((int*)bits)[0];
                char* newBits = new char[len];
                memcpy(newBits, bits, len);
                copy->Consume(newBits);
                // Set the order maker on the copied record so it can be used later for comparisons.
                copy->SetOrderMaker(&order);
                uniqueRecords.push_back(copy);
            }
        }
        accumulated = true;
    }

    // ------------------------------
    // Phase 2: Return Distinct Records
    // ------------------------------
    if (currentIndex < uniqueRecords.size()) {
        Record* distinctRecord = uniqueRecords[currentIndex];
        // Deep copy the stored record into _record.
        char* bits = distinctRecord->GetBits();
        int len = ((int*)bits)[0];
        char* newBits = new char[len];
        memcpy(newBits, bits, len);
        _record.Consume(newBits);
        currentIndex++;
        return true;
    } else {
        // At end, free all memory allocated in the accumulation phase.
        for (size_t i = 0; i < uniqueRecords.size(); i++) {
            delete uniqueRecords[i];
        }
        uniqueRecords.clear();
        // Reset static variables so that subsequent queries can accumulate fresh data.
        accumulated = false;
        currentIndex = 0;
        return false;
    }
}


ostream& DuplicateRemoval::print(ostream& _os) {
	_os << "DISTINCT {" << schema << "} " << *producer << endl;

	return _os;
}


Sum::Sum(Schema& _schemaIn, Schema& _schemaOut, Function& _compute,
	RelationalOp* _producer) {
	schemaIn = _schemaIn;
	schemaOut = _schemaOut;
	compute = _compute;
	producer = _producer;


}

Sum::~Sum() {
//delete producer;



}

bool Sum::GetNext(Record& _record) {
	static bool flag = false;
	if (flag){return false;}
	double runningSum = 0;
    int iRes;
    double dRes;

    while (producer->GetNext(_record)) {
        Type returnType = compute.Apply(_record, iRes, dRes);

        if (returnType == Integer) {
            runningSum += static_cast<double>(iRes);
        } else if (returnType == Float) {
            runningSum += dRes;
        } else {
            cerr << "Unknown return type from compute.Apply" << endl;
            return false;
        }
    }
	// build output record from runningSum
	// Construct a raw binary record manually: 16 bytes
    const int recSize = 16;
    char* recSpace = new char[recSize];

    int totalSize = recSize;
    int attrOffset = 8;  // first (and only) attribute is at byte 8

    memcpy(recSpace, &totalSize, sizeof(int));            // [0] = total size
    memcpy(recSpace + 4, &attrOffset, sizeof(int));       // [4] = offset to attr
    memcpy(recSpace + 8, &runningSum, sizeof(double));    // [8] = double value

    _record.Consume(recSpace); // Transfer memory ownership

	flag = true;
	return true;
}

ostream& Sum::print(ostream& _os) {
	_os << "SUM {" << schemaOut << "} " << *producer << endl;

	return _os;
}


GroupBy::GroupBy(Schema& _schemaIn, Schema& _schemaOut, OrderMaker& _groupingAtts,
	Function& _compute,	RelationalOp* _producer) {
		schemaIn = _schemaIn;
		schemaOut = _schemaOut;
		groupingAtts = _groupingAtts;
		compute = _compute;
		producer = _producer;

}

GroupBy::~GroupBy() {

	delete producer;


}


// bool GroupBy::GetNext(Record& _record) {
//     // Determine once whether the grouping attribute is a string.
//     static bool initialized = false;
//     static bool keyIsString = false;
//     if (!initialized) {
//         // Get the attribute corresponding to the grouping key.
//         Attribute groupAttr = schemaIn.GetAtts()[ groupingAtts.whichAtts[0] ];
//         cout << "Type of grouping attribute --> " << groupAttr.type << endl;
//         cout << " grouping attr" << endl;
//         // Assume type String means a string attribute.
//         if (groupAttr.type == String)
//             keyIsString = true;
//         else
//             keyIsString = false;
//         initialized = true;
//     }

//     // Declare static maps and iterators for accumulating and then outputting groups.
//     // For integer grouping keys:
//     static std::map<int, double> groupMapInt;
//     static std::map<int, double>::iterator itInt;
//     // For string grouping keys:
//     static std::map<std::string, double> groupMapStr;
//     static std::map<std::string, double>::iterator itStr;

//     //  run the accumulation phase only once.
//     static bool isDone = false;
//     if (!isDone) {
//         if (!keyIsString) {
//             groupMapInt.clear();
//             Record inputRec;
//             int iRes;
//             double dRes;
//             while (producer->GetNext(inputRec)) {
//                 double value = 0.0;
//                 cout<< "iRes/dRes in group by: " << iRes << dRes << endl;
//                 cout << "Input record in group by: " << endl;
//                 inputRec.print(std::cout, schemaIn);
//                 cout<< "Input record above" << endl ;
//                 Type retType = compute.Apply(inputRec, iRes, dRes);
//                 if (retType == Integer)
//                     value = static_cast<double>(iRes);
//                 else if (retType == Float)
//                     value = dRes;
//                 // Extract the grouping key as an integer.
//                 char* colData = inputRec.GetColumn(groupingAtts.whichAtts[0]);
//                 int key = *reinterpret_cast<int*>(colData);
//                 groupMapInt[key] += value;
//             }
//             itInt = groupMapInt.begin();
//         } else {
//             groupMapStr.clear();
//             Record inputRec;
//             int iRes;
//             double dRes;
//             while (producer->GetNext(inputRec)) {
//                 double value = 0.0;
//                 cout<< "iRes/dRes in group by:  in else cond" << iRes << dRes << endl;
//                 cout << "Input record in group by: in else cond" << endl;

//                 inputRec.print(std::cout, schemaOut);
//                 cout<< "Input record above" << endl ;
//                 Type retType = compute.Apply(inputRec, iRes, dRes);
//                 if (retType == Integer)
//                     value = static_cast<double>(iRes);
//                 else if (retType == Float)
//                     value = dRes;
//                 // Extract the grouping key as a string.
//                 char* colData = inputRec.GetColumn(groupingAtts.whichAtts[0]);
//                 std::string key(colData);
//                 groupMapStr[key] += value;
//             }
//             itStr = groupMapStr.begin();
//         }
//         isDone = true;
//     }

//     // Output phase:
//     if (!keyIsString) {
//         if (itInt != groupMapInt.end()) {
//             int groupKey = itInt->first;
//             double groupSum = itInt->second;
//             ++itInt;

//             // Build a record with two attributes in this order:
//             // Attribute 1: aggregated sum (double, 8 bytes)
//             // Attribute 2: grouping key (int, 4 bytes)
//             // Use a header of 3 integers (each 4 bytes): total record size, offset for attr1, offset for attr2.
//             const int headerSize = 12;
//             const int sizeSum = 8;
//             const int sizeKey = 4;
//             const int recSize = headerSize + sizeSum + sizeKey;  // 24 bytes
//             char* recSpace = new char[recSize];
//             *reinterpret_cast<int*>(recSpace) = recSize;
//             *reinterpret_cast<int*>(recSpace + 4) = headerSize;
//             *reinterpret_cast<int*>(recSpace + 8) = headerSize + sizeSum;
//             memcpy(recSpace + headerSize, &groupSum, sizeSum);
//             memcpy(recSpace + headerSize + sizeSum, &groupKey, sizeKey);
//             _record.Consume(recSpace);
//             return true;
//         } else {
//             return false;
//         }
//     } else { // grouping key is a string
//         if (itStr != groupMapStr.end()) {
//             std::string groupKey = itStr->first;
//             double groupSum = itStr->second;
//             ++itStr;
//             int strLen = static_cast<int>(groupKey.length()) + 1;  // including null terminator
//             const int headerSize = 12;
//             const int sizeSum = 8;
//             const int sizeKey = strLen;
//             const int recSize = headerSize + sizeSum + sizeKey;
//             char* recSpace = new char[recSize];
//             *reinterpret_cast<int*>(recSpace) = recSize;
//             *reinterpret_cast<int*>(recSpace + 4) = headerSize;
//             *reinterpret_cast<int*>(recSpace + 8) = headerSize + sizeSum;
//             memcpy(recSpace + headerSize, &groupSum, sizeSum);
//             memcpy(recSpace + headerSize + sizeSum, groupKey.c_str(), sizeKey);
//             _record.Consume(recSpace);
//             return true;
//         } else {
//             return false;
//         }
//     }
// }

bool GroupBy::GetNext(Record& _record) {
    // --- 1. Initialization: figure out key type exactly once ---

    static bool initialized = false;
    static bool keyIsString;
    if (!initialized) {
        Attribute grp = schemaIn.GetAtts()[ groupingAtts.whichAtts[0] ];
        keyIsString = (grp.type == String);
        initialized  = true;
    }

    // --- 2. Single unified map from string→double ---
    static std::map<std::string,double> agg;
    static std::map<std::string,double>::iterator it;

    // --- 3. Accumulation phase, only once ---
    static bool done = false;
    if (!done) {
        agg.clear();
        Record rec;
        while (producer->GetNext(rec)) {
            // compute the aggregate value
            int    iRes;
            double dRes;
            cout << schemaIn <<endl;
            cout << " Groupby schema above" << endl;
            rec.print(std::cout,schemaIn);
            cout << "printed input record in group by get next" << endl;
            Type   t = compute.Apply(rec, iRes, dRes);
            double val = (t == Integer ? iRes : dRes);

            // extract raw column
            char* colData = rec.GetColumn(groupingAtts.whichAtts[0]);

            // unify into a string key
            std::string key = keyIsString
                ? std::string(colData)
                : std::to_string(*reinterpret_cast<int*>(colData));

            agg[key] += val;
        }
        it   = agg.begin();
        done = true;
    }

    // --- 4. Output phase: one group per call ---
    if (it == agg.end()) return false;

    // pull out this group
    std::string keyStr = it->first;
    double      sumVal = it->second;
    ++it;

    // build output record: ( sumVal, key )
    // header has 3 ints: [ totalSize, offset0, offset1 ]
    // then 8‑byte double followed by key bytes
    int    sizeKey    = keyIsString ? keyStr.size()+1 : sizeof(int);
    int    headerSize = 3 * sizeof(int);
    int    recSize    = headerSize + sizeof(double) + sizeKey;
    char*  buf        = new char[recSize];
    int*   hdr        = (int*)buf;
    hdr[0] = recSize;
    hdr[1] = headerSize;            // offset of sumVal
    hdr[2] = headerSize + sizeof(double); // offset of key

    // copy sumVal
    memcpy(buf + hdr[1], &sumVal, sizeof(double));

    // write key
    if (keyIsString) {
        memcpy(buf + hdr[2], keyStr.c_str(), sizeKey);
    } else {
        int k = std::stoi(keyStr);
        memcpy(buf + hdr[2], &k, sizeof(int));
    }

    _record.Consume(buf);
    return true;
}



ostream& GroupBy::print(ostream& _os) {
	_os << "GROUP BY {" << schemaOut << "} {grouping atts: " << groupingAtts << "}" << *producer << endl;

	return _os;
}


WriteOut::WriteOut(Schema& _schema, string& _outFile, RelationalOp* _producer) {
	schema = _schema;
	outFile = _outFile;
	producer = _producer;

}

WriteOut::~WriteOut() {
	if (producer) {
		delete producer;
	}

}


bool WriteOut::GetNext(Record& _record) {
	static bool initialized = false;
	static std::ofstream fileStream;
	static bool isProducerDone = false;

	if (isProducerDone) return false;

	if (!initialized) {
		fileStream.open(outFile);
		if (!fileStream.is_open()) {
			std::cerr << "Error: Cannot open file " << outFile << " for writing." << std::endl;
			return false;
		}
		initialized = true;
	}

 //Get one record at a time from producer
	if (producer->GetNext(_record)) {
		std::cout << "Writing record to file: ";
		if (_record.GetBits() == nullptr) return false;
		_record.print(fileStream, schema);
		fileStream << std::endl;
		return true;
	} else {
		isProducerDone = true;
		fileStream.close();
		return false;
	}
}


ostream& WriteOut::print(ostream& _os) {
	_os << "OUTPUT {" << schema << "} " << *producer << endl;

	return _os;
}


void QueryExecutionTree::ExecuteQuery() {
	Record r;
	while (root->GetNext(r)) {
		// this loop ends when the query has processed all the records
		cout << "Record inside execution tree - ExecuteQuery -->  "<< root   <<endl;

	}
}


ostream& operator<<(ostream& _os, QueryExecutionTree& _op) {
	_os << "QUERY EXECUTION TREE: " << *_op.root << endl;

	return _os;
}
