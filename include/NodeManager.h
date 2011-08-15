/*
 *  NodeManager.h
 *  aomdd
 *
 *  Created by William Lam on May 11, 2011
 *  Copyright 2011 UC Irvine. All rights reserved.
 *
 */

#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

#include "base.h"
#include "graphbase.h"
#include "MetaNode.h"

namespace aomdd {

enum Operator {
    PROD, SUM, MAX, REDUCE, MARGINALIZE, NONE
};

typedef boost::unordered_multiset<size_t> ParamSet;

class Operation {
    Operator op;
    ParamSet params;
    int varid;

public:
    Operation() :
        op(NONE) {
    }
    Operation(Operator o, MetaNodePtr arg, int vid = 0) :
        op(o), varid(vid) {
        params.insert(size_t(arg.get()));
    }
    Operation(Operator o, MetaNodePtr arg1, const std::vector<MetaNodePtr> &arg2) :
        op(o) {
        params.insert(size_t(arg1.get()));
        for (unsigned int i = 0; i < arg2.size(); ++i) {
            params.insert(size_t(arg2[i].get()));
        }
    }

    inline const Operator &GetOperator() const {
        return op;
    }

    inline const ParamSet &GetParamSet() const {
        return params;
    }

    inline int GetVarID() const {
        return varid;
    }
};

size_t hash_value(const Operation &o);
bool operator==(const Operation &lhs, const Operation &rhs);

typedef boost::unordered_set<MetaNodePtr> UniqueTable;
typedef boost::unordered_map<Operation, MetaNodePtr> OperationCache;
typedef std::pair<MetaNodePtr, std::vector<MetaNodePtr> > ApplyParamSet;

class NodeManager {
    UniqueTable ut;
    OperationCache opCache;
    NodeManager() {
    }
    NodeManager(NodeManager const&) {
    }
    NodeManager& operator=(NodeManager const&) {
        return *this;
    }

    static bool initialized;
    static NodeManager *singleton;

    MetaNodePtr NormalizeHelper(MetaNodePtr root);

public:
    static NodeManager *GetNodeManager();
    // Create a metanode from a variable with a children list
    MetaNodePtr CreateMetaNode(const Scope &var,
            const std::vector<ANDNodePtr> &ch, double weight = 1);

    MetaNodePtr CreateMetaNode(int varid, unsigned int card,
            const std::vector<ANDNodePtr> &ch, double weight = 1);

    // Create a metanode based on a tabular form of the function
    // Variable ordering is defined by the scope
    MetaNodePtr CreateMetaNode(const Scope &vars,
            const std::vector<double> &vals, double weight = 1);

    // Be sure the input node is a root!
    // Returns a vector of pointers since ANDNodes can have multiple
    // MetaNode children
    std::vector<MetaNodePtr> FullReduce(MetaNodePtr node, double &w, bool isRoot=false);

    // Driver function that returns a dummy root if needed (to combine
    // multiple MetaNodes as a single output
    MetaNodePtr FullReduce(MetaNodePtr node);

    // Same as above, but single level version, it assumes all the decision
    // diagrams rooted by the children are already fully reduced
    std::vector<MetaNodePtr> SingleLevelFullReduce(MetaNodePtr node, double &w, bool isRoot=false);
    MetaNodePtr SingleLevelFullReduce(MetaNodePtr node);


    MetaNodePtr Apply(MetaNodePtr lhs, const std::vector<MetaNodePtr> &rhs, Operator op,
            const DirectedGraph &embeddedPT, double w = 1);

    std::vector<ApplyParamSet> GetParamSets(const DirectedGraph &tree,
            const std::vector<MetaNodePtr> &lhs,
            const std::vector<MetaNodePtr> &rhs) const;

    MetaNodePtr Marginalize(MetaNodePtr root, const Scope &s, const DirectedGraph &embeddedPT);
    MetaNodePtr Condition(MetaNodePtr root, const Assignment &cond);

    MetaNodePtr Maximize(MetaNodePtr root, const Scope &s, const DirectedGraph &embeddedPT);
    MetaNodePtr Normalize(MetaNodePtr root);


    unsigned int GetNumberOfNodes() const;

    void PrintUniqueTable(std::ostream &out) const;
    void PrintReferenceCount(std::ostream &out) const;

};

} // end of aomdd namespace

#endif /* NODEMANAGER_H_ */
