/*
 *  NodeManager.cpp
 *  aomdd
 *
 *  Created by William Lam on May 13, 2011
 *  Copyright 2011 UC Irvine. All rights reserved.
 *
 */

#include "MetaNode.h"
#include "NodeManager.h"
#include "utils.h"

#define MAX(a, b) (a > b ? a : b)

namespace aomdd {
using namespace std;

// Be sure to test to see if sets give the same hash if items are inserted
// in different orders
size_t hash_value(const Operation &o) {
    size_t seed = 0;
    boost::hash_combine(seed, o.GetOperator());
    BOOST_FOREACH(ParamSet::value_type i, o.GetParamSet()) {
        boost::hash_combine(seed, i);
    }
    return seed;

}
bool operator==(const Operation &lhs, const Operation &rhs) {
    return lhs.GetOperator() == rhs.GetOperator() && lhs.GetParamSet()
            == rhs.GetParamSet();
}

// =================

NodeManager *NodeManager::GetNodeManager() {
    if (!initialized) {
        singleton = new NodeManager();
        initialized = true;
        return singleton;
    }
    else {
        return singleton;
    }
}

vector<MetaNodePtr> NodeManager::ReweighNodes(const vector<MetaNodePtr> &nodes, double w) {
    vector<MetaNodePtr> ret;
    BOOST_FOREACH(MetaNodePtr i, nodes) {
        if (i.get() != MetaNode::GetZero().get() &&
                i.get() != MetaNode::GetOne().get()) {
            MetaNodePtr newNode(new MetaNode(*i));
            newNode->SetWeight(newNode->GetWeight() * w);
            ret.push_back(newNode);
        }
        else {
            ret.push_back(i);
        }
    }
    return ret;
}

vector<MetaNodePtr> NodeManager::CopyMetaNodes(const vector<MetaNodePtr> &nodes) {
    vector<MetaNodePtr> ret;
    BOOST_FOREACH(MetaNodePtr i, nodes) {
        if (i.get() != MetaNode::GetZero().get() &&
                i.get() != MetaNode::GetOne().get()) {
            MetaNodePtr newNode(new MetaNode(*i));
            ret.push_back(newNode);
        }
        else {
            ret.push_back(i);
        }
    }
    return ret;
}

vector<ANDNodePtr> NodeManager::CopyANDNodes(const vector<ANDNodePtr> &nodes) {
    vector<ANDNodePtr> ret;
    BOOST_FOREACH(ANDNodePtr i, nodes) {
        ANDNodePtr newNode(new MetaNode::ANDNode(*i));
        ret.push_back(newNode);
    }
    return ret;
}

vector<ApplyParamSet> NodeManager::GetParamSets(const DirectedGraph &tree,
        const vector<MetaNodePtr> &lhs, const vector<MetaNodePtr> &rhs) const {
    vector<ApplyParamSet> ret;
    // First check if rhs is terminal and lhs is size 1
    if (lhs.size() == 1 && rhs.size() == 1 && rhs[0]->IsTerminal()) {
        ret.push_back(make_pair<MetaNodePtr, vector<MetaNodePtr> >(lhs[0], rhs));
        return ret;
    }

    unordered_map<int, MetaNodePtr> lhsMap;
    unordered_map<int, MetaNodePtr> rhsMap;

    BOOST_FOREACH(MetaNodePtr i, lhs) {
        lhsMap[i->GetVarID()] = i;
    }
    BOOST_FOREACH(MetaNodePtr i, rhs) {
        rhsMap[i->GetVarID()] = i;
    }

    unordered_map<int, int> hiAncestor;
    BOOST_FOREACH(MetaNodePtr i, lhs) {
        int varid = i->GetVarID();
        hiAncestor[varid] = varid;
        if (varid < 0) continue;

        int parent = varid;
        DInEdge ei, ei_end;
        tie(ei, ei_end) = in_edges(parent, tree);
        while (ei != ei_end) {
            parent = source(*ei, tree);
            if (rhsMap.find(parent) != rhsMap.end()) {
                hiAncestor[varid] = parent;
            }
            tie(ei, ei_end) = in_edges(parent, tree);
        }
    }

    BOOST_FOREACH(MetaNodePtr i, rhs) {
        int varid = i->GetVarID();
        hiAncestor[varid] = varid;
        if (varid < 0) continue;

        int parent = varid;
        DInEdge ei, ei_end;
        tie(ei, ei_end) = in_edges(parent, tree);
        while (ei != ei_end) {
            parent = source(*ei, tree);
            if (lhsMap.find(parent) != lhsMap.end()) {
                hiAncestor[varid] = parent;
            }
            tie(ei, ei_end) = in_edges(parent, tree);
        }
    }

    unordered_map<int, vector<int> > descendants;
    unordered_map<int, int>::iterator it = hiAncestor.begin();

    for (; it != hiAncestor.end(); ++it) {
        descendants[it->second].push_back(it->first);
    }

    unordered_map<int, vector<int> >::iterator dit = descendants.begin();
    for (; dit != descendants.end(); ++dit) {
        MetaNodePtr paramLHS;
        vector<MetaNodePtr> paramRHS;
        int anc = dit->first;
        const vector<int> &dList = dit->second;
        bool fromLHS = false;
        unordered_map<int, MetaNodePtr>::iterator mit;
        if ( (mit = lhsMap.find(anc)) != lhsMap.end() ) {
            paramLHS = mit->second;
            fromLHS = true;
            lhsMap.erase(mit);
        }
        else if ( (mit = rhsMap.find(anc)) != rhsMap.end() ) {
            paramLHS = mit->second;
            rhsMap.erase(mit);
        }
        else {
            // Problem if it gets here
            assert(false);
        }
        BOOST_FOREACH(int i, dList) {
            if ( fromLHS && (mit = rhsMap.find(i)) != rhsMap.end() ) {
                paramRHS.push_back(mit->second);
                rhsMap.erase(mit);
            }
            else if ( (mit = lhsMap.find(i)) != lhsMap.end() ) {
                paramRHS.push_back(mit->second);
                lhsMap.erase(mit);
            }
        }
        ret.push_back(make_pair<MetaNodePtr, vector<MetaNodePtr> >(paramLHS, paramRHS));
    }
    return ret;

}


// Public functions below here

MetaNodePtr NodeManager::CreateMetaNode(const Scope &var,
        const vector<ANDNodePtr> &ch, double weight) {
    MetaNodePtr temp(new MetaNode(var, ch));
    temp->SetWeight(weight);
    UniqueTable::iterator it = ut.find(temp);
    if (it != ut.end()) {
        return *it;
    }
    else {
        ut.insert(temp);
        return temp;
    }
}

MetaNodePtr NodeManager::CreateMetaNode(int varid, unsigned int card,
        const vector<ANDNodePtr> &ch, double weight) {
    Scope var;
    var.AddVar(varid, card);
    return CreateMetaNode(var, ch, weight);
}

MetaNodePtr NodeManager::CreateMetaNode(const Scope &vars,
        const vector<double> &vals, double weight) {
    assert(vars.GetCard() == vals.size());
    int rootVarID = vars.GetOrdering().front();
    unsigned int card = vars.GetVarCard(rootVarID);
    Scope rootVar;
    rootVar.AddVar(rootVarID, card);
    vector<ANDNodePtr> children;

    // Need to split the values vector if we are not at a leaf
    if (card != vals.size()) {
        Scope v(vars);
        Scope restVars = v - rootVar;

        // Dummy variable case
        if (card == 1) {
            vector<MetaNodePtr> ANDch;
            ANDch.push_back(CreateMetaNode(restVars, vals, weight));
            ANDNodePtr newNode(new MetaNode::ANDNode(1.0, ANDch));
            children.push_back(newNode);
        }
        // General case
        else {
            vector<vector<double> > valParts = SplitVector(vals, card);
            for (unsigned int i = 0; i < card; i++) {
                vector<MetaNodePtr> ANDch;
                ANDch.push_back(CreateMetaNode(restVars, valParts[i], weight));
                ANDNodePtr newNode(new MetaNode::ANDNode(1.0, ANDch));
                children.push_back(newNode);
            }
        }
    }
    // Otherwise we are at the leaves
    else {
        for (unsigned int i = 0; i < card; i++) {
            const MetaNodePtr &terminal = ((vals[i] == 0) ? MetaNode::GetZero()
                    : MetaNode::GetOne());
            vector<MetaNodePtr> ANDch;
            ANDch.push_back(terminal);
            ANDNodePtr newNode(new MetaNode::ANDNode(vals[i], ANDch));
            children.push_back(newNode);
        }
    }
    return CreateMetaNode(rootVar, children, weight);
}

vector<MetaNodePtr> NodeManager::FullReduce(MetaNodePtr node, double &w) {
    // terminal check
    if (node.get() == MetaNode::GetZero().get() || node.get()
            == MetaNode::GetOne().get()) {
        return vector<MetaNodePtr> (1, node);
    }

    const vector<ANDNodePtr> &ch = node->GetChildren();
    vector<ANDNodePtr> newCh;
    for (unsigned int i = 0; i < ch.size(); ++i) {
        const vector<MetaNodePtr> &andCh = ch[i]->GetChildren();
        vector<MetaNodePtr> reduced;
        for (unsigned int j = 0; j < andCh.size(); ++j) {
            double wr = 1.0;
            vector<MetaNodePtr> temp = FullReduce(andCh[j], wr);
            BOOST_FOREACH(MetaNodePtr mn, temp) {
                if ( mn.get() == MetaNode::GetZero().get() ) {
                    wr = 0;
                    reduced.clear();
                    reduced.push_back(mn);
                    break;
                }
                if ( !reduced.empty() ) {
                    if ( reduced.back().get() == MetaNode::GetOne().get() ) {
                        reduced.pop_back();
                    }
                    else if ( mn.get() == MetaNode::GetOne().get() ) {
                        continue;
                    }
                }
                reduced.push_back(mn);
            }
            w *= wr;
            if (w == 0) break;
        }
        ANDNodePtr rAND(new MetaNode::ANDNode(w * ch[i]->GetWeight(), reduced));
        newCh.push_back(rAND);
        w = 1.0;
    }

    bool redundant = true;
    ANDNodePtr temp = newCh[0];
    if (newCh.size() == 1) redundant = false;
    for (unsigned int i = 1; i < newCh.size(); ++i) {
        if (temp != newCh[i]) {
            redundant = false;
            break;
        }
    }

    if ( redundant ) {
        w *= newCh[0]->GetWeight();
        return newCh[0]->GetChildren();
    }
    else {
        MetaNodePtr ret = CreateMetaNode(node->GetVarID(), node->GetCard(), newCh);
        return vector<MetaNodePtr>(1, ret);
    }
}

MetaNodePtr NodeManager::FullReduce(MetaNodePtr root) {
    double w = 1.0;

    Operation ocEntry(REDUCE, root);
    OperationCache::iterator ocit = opCache.find(ocEntry);
    if ( ocit != opCache.end() ) {
        //Found result in cache
        return ocit->second;
    }

    vector<MetaNodePtr> nodes = FullReduce(root, w);

    // Reduced to a single root and weight was not reweighted
    if (nodes.size() == 1 && fabs(w - 1.0) < 1e-10) {
        return nodes[0];
    }

    int varid = root->GetVarID();
    ANDNodePtr newAND(new MetaNode::ANDNode(w, nodes));
    MetaNodePtr newMeta = CreateMetaNode(varid, 1, vector<ANDNodePtr>(1, newAND));
    Operation entryKey(REDUCE, root);
    opCache.insert(make_pair<Operation, MetaNodePtr>(entryKey, newMeta));
    return newMeta;
}

MetaNodePtr NodeManager::Apply(MetaNodePtr lhs,
        const vector<MetaNodePtr> &rhs,
        Operator op,
        const DirectedGraph &embeddedPT,
        double w) {
    int varid = lhs->GetVarID();
    int card = lhs->GetCard();

    // Handle if lhs is dummy and rhs is not the dummy version
    if ( lhs->IsDummy() &&
            (!rhs.empty()) &&
            lhs->GetVarID() == rhs[0]->GetVarID() &&
            !(rhs[0]->IsDummy()) ) {
        vector<MetaNodePtr> newrhs(lhs->GetChildren()[0]->GetChildren());
        return Apply(rhs[0], newrhs, op, embeddedPT, w);
    }


    Operation ocEntry(op, lhs, rhs);
    OperationCache::iterator ocit = opCache.find(ocEntry);
    if ( ocit != opCache.end() ) {
        //Found result in cache
        return ocit->second;
    }

    // Base cases
    switch(op) {
        case PROD:
            // If its a terminal the rhs must be same terminals
            if ( lhs->IsTerminal() ) {
                return lhs;
            }
            /*
            if ( lhs.get() == MetaNode::GetZero().get() ) {
                return MetaNode::GetZero();
            }
            */
            // No rhs, so result is just lhs
            else if ( rhs.size() == 0 ) {
                return lhs;
            }
            // Look for any zeros on the rhs. Result is zero if found
            else {
                for (unsigned int i = 0; i < rhs.size(); ++i) {
                    if ( rhs[i].get() == MetaNode::GetZero().get() ) {
                        return MetaNode::GetZero();
                    }
                }
            }

            /*
            if ( lhs.get() == MetaNode::GetOne().get() ) {
                return MetaNode::GetOne();
            }
            */
            break;
        case SUM:
            if ( rhs.size() == 0 || lhs->IsTerminal() ) {
                return lhs;
            }
            break;
        case MAX:
            if ( rhs.size() == 0 || lhs->IsTerminal() ) {
                return lhs;
            }
        default:
            assert(false);
    }

    // Should have detected terminals
    assert(varid >= 0);


    vector<ANDNodePtr> children;

    // For each value of lhs
    for (int k = 0; k < card; ++k) {
        // Get original weight
        vector<MetaNodePtr> newChildren;
        double weight = w;
        weight *= lhs->GetChildren()[k]->GetWeight() * lhs->GetWeight();
        vector<MetaNodePtr> lhsChildren =
                lhs->GetChildren()[k]->GetChildren();
        vector<MetaNodePtr> tempChildren;

        if ( rhs.size() == 1 && varid == rhs[0]->GetVarID() ) {
            // Same variable, single roots case
            int chid = k;
            if (rhs[0]->IsDummy()) {
                chid = 0;
            }
            tempChildren = rhs[0]->GetChildren()[chid]->GetChildren();
            double rhsWeight = rhs[0]->GetChildren()[chid]->GetWeight() * rhs[0]->GetWeight();
            switch(op) {
                case PROD:
                    weight *= rhsWeight;
                    break;
                case SUM:
                    weight += rhsWeight;
                    break;
                case MAX:
                    weight = MAX(weight, rhsWeight);
                    break;
                default:
                    assert(false);
            }
        }
        else {
            // Not the same variable, prepare to push rhs down
            tempChildren = rhs;
        }

        vector<ApplyParamSet> paramSets = GetParamSets(embeddedPT, lhsChildren, tempChildren);

        // For each parameter set
        for (unsigned int i = 0; i < paramSets.size(); ++i) {
            MetaNodePtr subDD = Apply(paramSets[i].first, paramSets[i].second, op, embeddedPT, w);
            if ( op == PROD && subDD.get() == MetaNode::GetZero().get() ) {
                newChildren.clear();
                newChildren.push_back(MetaNode::GetZero());
                break;
            }
            else {
                if( !newChildren.empty() && newChildren.back()->IsTerminal() ) {
                    newChildren.pop_back();
                }
                else if ( !newChildren.empty() && subDD->IsTerminal() ) {
                    continue;
                }
                newChildren.push_back(subDD);
            }
            if ((op == SUM || op == MAX) && !subDD->IsTerminal()) {
                cout << "Not at leaves, weight was "<< weight << endl;
                weight = 1;
            }
        }
        /*
        cout << "value=" << k << endl;
        cout << "Old weight = " << lhs->GetChildren()[k]->GetWeight() << endl;
        cout << "New weight = " << weight << endl;
        */

        if (weight == 0) {
            newChildren.clear();
            newChildren.push_back(MetaNode::GetZero());
        }
        ANDNodePtr newNode(new MetaNode::ANDNode(weight, newChildren));
        children.push_back(newNode);
    }
    // Redundancy can be resolved outside
    Scope var;
    var.AddVar(varid, card);
    MetaNodePtr u = CreateMetaNode(var, children);
    Operation entryKey(op, lhs, rhs);
    opCache.insert(make_pair<Operation, MetaNodePtr>(entryKey, u));
    return u;
}

// Sets each AND node of variables to marginalize to be the result of summing
// the respective MetaNode children of each AND node. Redundancy can be
// resolved outside.
MetaNodePtr NodeManager::Marginalize(MetaNodePtr root, const Scope &s,
        const DirectedGraph &embeddedpt) {
    if (root.get() == MetaNode::GetZero().get()) {
        return MetaNode::GetZero();
    }
    else if (root.get() == MetaNode::GetOne().get()) {
        return MetaNode::GetOne();
    }
    int varid = root->GetVarID();
    int card = root->GetCard();

    Operation ocEntry(MARGINALIZE, root, varid);
    OperationCache::iterator ocit = opCache.find(ocEntry);
    if ( ocit != opCache.end() ) {
        //Found result in cache
        return ocit->second;
    }

    // Marginalize each subgraph
    const vector<ANDNodePtr> &andNodes = root->GetChildren();
    vector<ANDNodePtr> newANDNodes;
    BOOST_FOREACH(ANDNodePtr i, andNodes) {
        const vector<MetaNodePtr> &metaNodes = i->GetChildren();
        vector<MetaNodePtr> newMetaNodes;
        BOOST_FOREACH(MetaNodePtr j, metaNodes) {
            MetaNodePtr newMetaNode = Marginalize(j, s, embeddedpt);
            if ( !newMetaNodes.empty() && newMetaNodes.back().get() == MetaNode::GetOne().get() ) {
                newMetaNodes.pop_back();
            }
            else if ( !newMetaNodes.empty() && newMetaNode.get() == MetaNode::GetOne().get() ) {
                continue;
            }
            newMetaNodes.push_back(newMetaNode);
        }
        ANDNodePtr newANDNode(new MetaNode::ANDNode(i->GetWeight(), newMetaNodes));
        newANDNodes.push_back(newANDNode);
    }

    // If the root is to be marginalized
    if (s.VarExists(varid)) {
        // Assume node resides at bottom
        double weight = 0;
        for (unsigned int i = 0; i < newANDNodes.size(); ++i) {
            weight += newANDNodes[i]->GetWeight();
        }
        newANDNodes.clear();
        vector<MetaNodePtr> newMetaNodes;
        if (weight == 0) {
            newMetaNodes.push_back(MetaNode::GetZero());
        } else {
            newMetaNodes.push_back(MetaNode::GetOne());
        }
        ANDNodePtr newAND(new MetaNode::ANDNode(weight, newMetaNodes));
        for (unsigned int i = 0; i < andNodes.size(); ++i) {
            newANDNodes.push_back(newAND);
        }
    }
    Scope var;
    var.AddVar(varid, card);
    MetaNodePtr ret = CreateMetaNode(var, newANDNodes, root->GetWeight());
    Operation entryKey(MARGINALIZE, root, varid);
    opCache.insert(make_pair<Operation, MetaNodePtr>(entryKey, ret));
    return ret;
}

// Sets each AND node of variables to marginalize to be the result of maximizing
// the respective MetaNode children of each AND node. Redundancy can be
// resolved outside.
MetaNodePtr NodeManager::Maximize(MetaNodePtr root, const Scope &s,
        const DirectedGraph &embeddedpt) {
    if (root.get() == MetaNode::GetZero().get()) {
        return MetaNode::GetZero();
    }
    else if (root.get() == MetaNode::GetOne().get()) {
        return MetaNode::GetOne();
    }
    int varid = root->GetVarID();
    int card = root->GetCard();

    Operation ocEntry(MAX, root, varid);
    OperationCache::iterator ocit = opCache.find(ocEntry);
    if ( ocit != opCache.end() ) {
        //Found result in cache
        return ocit->second;
    }

    // Maximize each subgraph
    const vector<ANDNodePtr> &andNodes = root->GetChildren();
    vector<ANDNodePtr> newANDNodes;
    BOOST_FOREACH(ANDNodePtr i, andNodes) {
        const vector<MetaNodePtr> &metaNodes = i->GetChildren();
        vector<MetaNodePtr> newMetaNodes;
        BOOST_FOREACH(MetaNodePtr j, metaNodes) {
            MetaNodePtr newMetaNode = Maximize(j, s, embeddedpt);
            if ( !newMetaNodes.empty() && newMetaNodes.back().get() == MetaNode::GetOne().get() ) {
                newMetaNodes.pop_back();
            }
            else if ( !newMetaNodes.empty() && newMetaNode.get() == MetaNode::GetOne().get() ) {
                continue;
            }
            newMetaNodes.push_back(newMetaNode);
        }
        ANDNodePtr newANDNode(new MetaNode::ANDNode(i->GetWeight(), newMetaNodes));
        newANDNodes.push_back(newANDNode);
    }

    // If the root is to be maximized
    if (s.VarExists(varid)) {
        // Assume node resides at bottom
        double weight = DOUBLE_MIN;
        for (unsigned int i = 0; i < newANDNodes.size(); ++i) {
            double temp = newANDNodes[i]->GetWeight();
            if (temp > weight) {
                weight = temp;
            }
        }
        newANDNodes.clear();
        vector<MetaNodePtr> newMetaNodes;
        if (weight == 0) {
            newMetaNodes.push_back(MetaNode::GetZero());
        } else {
            newMetaNodes.push_back(MetaNode::GetOne());
        }
        ANDNodePtr newAND(new MetaNode::ANDNode(weight, newMetaNodes));
        for (unsigned int i = 0; i < andNodes.size(); ++i) {
            newANDNodes.push_back(newAND);
        }
    }
    Scope var;
    var.AddVar(varid, card);
    MetaNodePtr ret = CreateMetaNode(var, newANDNodes, root->GetWeight());
    Operation entryKey(MAX, root, varid);
    opCache.insert(make_pair<Operation, MetaNodePtr>(entryKey, ret));
    return ret;
}

MetaNodePtr NodeManager::Condition(MetaNodePtr root, const Assignment &cond) {
    if (root.get() == MetaNode::GetZero().get() ||
            root.get() == MetaNode::GetOne().get()) {
        return root;
    }

    vector<ANDNodePtr> newANDNodes;
    const vector<ANDNodePtr> &rootCh = root->GetChildren();
    int val;
    if ( (val = cond.GetVal(root->GetVarID())) != ERROR_VAL ) {
        vector<MetaNodePtr> newMeta;
        BOOST_FOREACH(MetaNodePtr j, rootCh[val]->GetChildren()) {
            newMeta.push_back(Condition(j, cond));
        }
        ANDNodePtr newAND(new MetaNode::ANDNode(rootCh[val]->GetWeight(), newMeta));
        for (unsigned int i = 0; i < rootCh.size(); ++i) {
            newANDNodes.push_back(newAND);
        }
    }
    else {
        BOOST_FOREACH(ANDNodePtr i, root->GetChildren()) {
            vector<MetaNodePtr> newMeta;
            BOOST_FOREACH(MetaNodePtr j, i->GetChildren()) {
                newMeta.push_back(Condition(j, cond));
            }
            ANDNodePtr newAND(new MetaNode::ANDNode(i->GetWeight(), newMeta));
            newANDNodes.push_back(newAND);
        }
    }
    MetaNodePtr ret = CreateMetaNode(root->GetVarID(), root->GetCard(), newANDNodes);
    return ret;
}

MetaNodePtr NodeManager::Normalize(MetaNodePtr root) {
    MetaNodePtr ret = NormalizeHelper(root);
    ut.insert(ret);
    return ret;
}

MetaNodePtr NodeManager::NormalizeHelper(MetaNodePtr root) {
    if (root.get() == MetaNode::GetZero().get() ||
            root.get() == MetaNode::GetOne().get()) {
        return root;
    }
    double normConstant = 0;
    vector<ANDNodePtr> children;
    BOOST_FOREACH(ANDNodePtr i, root->GetChildren()) {
        const vector<MetaNodePtr> &achildren = i->GetChildren();
        vector<MetaNodePtr> newANDChildren;
        double w = i->GetWeight();
        BOOST_FOREACH(MetaNodePtr j, achildren) {
            MetaNodePtr newMeta = NormalizeHelper(j);
            w *= newMeta->GetWeight();
            newMeta->SetWeight(1);
            newANDChildren.push_back(newMeta);
            ut.insert(newMeta);
        }
        normConstant += w;
        ANDNodePtr newANDNode(new MetaNode::ANDNode(w, newANDChildren));
        children.push_back(newANDNode);
    }

    BOOST_FOREACH(ANDNodePtr i, children) {
        i->SetWeight(i->GetWeight() / normConstant);
    }
    double newMetaWeight = root->GetWeight() * normConstant;
    MetaNodePtr ret(new MetaNode(root->GetVarID(), root->GetCard(),
            children));
    ret->SetWeight(newMetaWeight);
    return ret;
}

unsigned int NodeManager::GetNumberOfNodes() const {
    return ut.size();
}

void NodeManager::PrintUniqueTable(ostream &out) const {
    BOOST_FOREACH (MetaNodePtr i, ut) {
        i->Save(out); out << endl;
    }
}

void NodeManager::PrintReferenceCount(ostream &out) const {
    BOOST_FOREACH (MetaNodePtr i, ut) {
        out << i.get() << ":" << i.use_count() - 1 << endl;
    }
}

bool NodeManager::initialized = false;
NodeManager *NodeManager::singleton = NULL;

} // end of aomdd namespace
