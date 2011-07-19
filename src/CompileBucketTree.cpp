/*
 *  CompileBucketTree.cpp
 *  aomdd
 *
 *  Created by William Lam on Jul 15, 2011
 *  Copyright 2011 UC Irvine. All rights reserved.
 *
 */

#include "CompileBucketTree.h"
#include "TableFunction.h"
using namespace std;

namespace aomdd {

CompileBucketTree::CompileBucketTree() : compiled(false) {
}

CompileBucketTree::CompileBucketTree(const Model &m, const PseudoTree *ptIn,
        const list<int> &orderIn, bool fr) : pt(ptIn), ordering(orderIn),
        fullReduce(fr), compiled(false) {
    const vector<TableFunction> &functions = m.GetFunctions();

    int numBuckets = ordering.size();
    if (pt->HasDummy()) {
        ordering.push_front(numBuckets++);
    }
    buckets.resize(numBuckets);
    for (unsigned int i = 0; i < functions.size(); i++) {
        int idx = functions[i].GetScope().GetOrdering().back();
        AOMDDFunction *f = new AOMDDFunction(functions[i].GetScope(), pt, functions[i].GetValues(), fr);
        buckets[idx].AddFunction(f);
    }
}

AOMDDFunction CompileBucketTree::Compile() {
    if (!compiled) {
        list<int>::reverse_iterator rit = ordering.rbegin();

        const DirectedGraph &tree = pt->GetTree();

        for (; rit != ordering.rend(); ++rit) {
            AOMDDFunction *message = buckets[*rit].Flatten();
            message->SetScopeOrdering(ordering);
            DInEdge ei, ei_end;
            tie(ei, ei_end) = in_edges(*rit, tree);
            // Not at root
            if (ei != ei_end) {
                int parent = source(*ei, tree);
                buckets[parent].AddFunction(message);
            }
            // At root
            else {
                compiledDD = *message;
            }
        }
    }
    return compiledDD;
}

void CompileBucketTree::PrintBucketFunctionScopes(ostream &out) const {
    for (unsigned int i = 0; i < buckets.size(); ++i) {
        out << "Bucket " << i << ":" << endl;
        buckets[i].PrintFunctionScopes(out);
        out << endl;
    }
}

CompileBucketTree::~CompileBucketTree() {
    // TODO Auto-generated destructor stub
}

} // end of aomdd namespace
