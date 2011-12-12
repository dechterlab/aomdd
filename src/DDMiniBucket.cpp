/*
 *  DDMiniBucket.cpp
 *  aomdd
 *
 *  Created by William Lam on Nov 1, 2011
 *  Copyright 2011 UC Irvine. All rights reserved.
 *
 */

#include "DDMiniBucket.h"
using namespace std;

typedef pair<unsigned long, int> LongIntPair;

namespace aomdd {

bool CompareLongIntPair (LongIntPair first, LongIntPair second) {
    return first.first > second.first || (first.first == second.first && first.second > second.second);
}

DDMiniBucket::DDMiniBucket() : metric(DIAGRAM_SIZE), bound(0) {
    // note: 0 bound means no limit
}

void DDMiniBucket::AddFunction(const AOMDDFunction *f) {
    functions.push_back(f);
    s = s + f->GetScope();
}

void DDMiniBucket::PurgeFunctions() {
    for (unsigned int i = 0; i < functions.size(); ++i) {
        delete functions[i];
    }
    functions.clear();
}

vector<AOMDDFunction*> DDMiniBucket::GenerateMessages() {
    vector<AOMDDFunction*> messages;
    if (functions.size() == 0) {
        return messages;
    }

    // TO DO
    if (metric == I_BOUND) {
        // check function scopes
    }
    else if (metric == DIAGRAM_SIZE) {
        // calculate upper bound on non-partitioned diagram size
        unsigned long size = 1;

        list<LongIntPair> functionSizes;

        for (unsigned int i = 0; i < functions.size(); ++i) {
            int numMeta, numAND;
            tie(numMeta, numAND) = functions[i]->Size();
            functionSizes.push_back(LongIntPair(numMeta+numAND, i));
            cout << "Function " << i << " size" << numMeta + numAND << endl;
            size *= numMeta + numAND;
        }
        functionSizes.sort(CompareLongIntPair);

        cout << "Estimated combined bucket size: " << size << endl;
        cout << "Bound: " << bound << endl;

        // partition if size is > bound
        if (bound > 0 && size > bound) {
            // do standard largest first greedy partitioning
            map<int,unsigned int> partitionSizes;
            vector< vector<int> > partitions;
            partitions.push_back(vector<int>());
            int curPartition = 0;
            partitionSizes[curPartition] = 1;
            while (!functionSizes.empty()) {
                list<LongIntPair>::iterator it = functionSizes.begin();
                if (it == functionSizes.end()) break;
                int currentSize, idx;

                bool found = false;

                // if including this function fits under the bound...
                do {
	                currentSize = it->first;
	                idx = it->second;
	                if (currentSize * partitionSizes[curPartition] <= bound || partitions[curPartition].empty()) {
	                    partitions[curPartition].push_back(idx);
	                    partitionSizes[curPartition] *= currentSize;
	                    functionSizes.erase(it);
	                    found = true;
	                }
	                else {
	                    ++it;
	                }
                } while (!found && it != functionSizes.end());

                if (!found) {
                    cerr << "Creating a new partition." << endl;
                    ++curPartition;
                    partitions.push_back(vector<int>());
                }
            }

            // generate messages for each partition
            for (unsigned int i = 0; i < partitions.size(); ++i) {
                messages.push_back(new AOMDDFunction(*functions[partitions[i][0]]));
                // for each function in the partition
                for (unsigned int j = 1; j < partitions[i].size(); ++j) {
                    messages[i]->Multiply(*functions[partitions[i][j]]);
                }
            }
            cerr << "Created " << partitions.size() << " partitions." << endl;
        }
        else {
            // just do the standard apply loop
            messages.push_back(new AOMDDFunction(*functions[0]));
            for (unsigned int i = 1; i < functions.size(); ++i) {
                messages[0]->Multiply(*functions[i]);
            }
        }
    }
    return messages;
}


void DDMiniBucket::PrintFunctionScopes(ostream &out) const {
    BOOST_FOREACH(const AOMDDFunction *f, functions) {
        f->GetScope().Save(out); out << endl;
    }
    out << endl;
}

void DDMiniBucket::PrintFunctionTables(ostream &out) const {
    BOOST_FOREACH(const AOMDDFunction *f, functions) {
        f->PrintAsTable(out); out << endl;
    }
    out << endl;
}

void DDMiniBucket::PrintDiagrams(ostream &out) const {
    BOOST_FOREACH(const AOMDDFunction *f, functions) {
        f->Save(out); out << endl;
    }
    out << endl;
}

void DDMiniBucket::PrintDiagramSizes(ostream &out) const {
    BOOST_FOREACH(const AOMDDFunction *f, functions) {
        unsigned int numOR;
        unsigned int numAND;
        tie(numOR, numAND) = f->Size();
        out << "m:" << numOR << ", a:" << numAND << endl;
    }
    out << endl;
}

DDMiniBucket::~DDMiniBucket() {
    // TODO Auto-generated destructor stub
}

} /* namespace aomdd */