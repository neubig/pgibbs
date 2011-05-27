#ifndef DIST_PYLM_H___
#define DIST_PYLM_H___

#include "dist-py.h"
#include <map>

namespace pgibbs {

typedef map<int,int> ChildMap;

class PYLMNode {

public:
    int wid_, parent_;
    ChildMap children_;
    PyDist<PySparseIndex> dist_;

    PYLMNode(int wid, int parent, double stren, double disc) : wid_(wid), parent_(parent), dist_(stren,disc) { }

};

class PYLM {

protected:
    int n_;

    vector<PYLMNode*> nodes_;
    vector<int> reuse_;
    vector<double> strens_, discs_;

public:

    PYLM() {
        nodes_.push_back(new PYLMNode(-1,-1,1.0,0.0));
    }

    PYLM(const PYLM & lm) : n_(lm.n_), nodes_(lm.nodes_), reuse_(lm.reuse_),
                            strens_(lm.strens_), discs_(lm.discs_) {
        for(int i = 0; i < (int)nodes_.size(); i++)
            if(nodes_[i])
                nodes_[i] = new PYLMNode(*nodes_[i]);
    }

    ~PYLM() {
        for(int i = 0; i < (int)nodes_.size(); i++)
            if(nodes_[i])
                delete nodes_[i];
    }

    int getNodeLevel(int node) {
        if(node == 0) return 0;
        return getNodeLevel(nodes_[node]->parent_)+1;
    }

    int getLevelSize(int lev) {
        int ret = 0;
        for(int i = 0; i < (int)nodes_.size(); i++)
            if(nodes_[i] && getNodeLevel(i) == lev)
                ret++;
        return ret;
    }

    void sampleParameters(double sa, double sb, double da, double db) {
        vector< vector< PyDist<PySparseIndex>* > > dists(n_);
        for(int i = 0; i < (int)nodes_.size(); i++) 
            if(nodes_[i] != 0) {
                // cerr << "pushing back at " << getNodeLevel(i) << "/"<<n_ << endl;
                dists[getNodeLevel(i)].push_back(&nodes_[i]->dist_);
            }
        for(int i = 0; i < n_; i++) {
            pair<double,double> newPar = PyDist<PySparseIndex>::sampleParameters(dists[i],sa,sb,da,db);
            strens_[i] = newPar.first; discs_[i] = newPar.second;
        }
    }

    int getN() const { return n_; }
    void setN(int n) { 
        n_ = n;
        strens_.resize(n_,1.0);
        discs_.resize(n_,0.0);
    }
    
    int getNext(int node, int wid) const {
        return const_cast<PYLM*>(this)->getNext(node,wid,false);
    }

    // get a pointer to the next node in order
    int getNext(int node, int wid, bool add) { 
        if(wid < 0) return 0;
        list<int> context;
        while(node != 0) {
            context.push_front(nodes_[node]->wid_);
            node = nodes_[node]->parent_;
        }
        context.push_front(wid);
        while((int)context.size() > n_-1)
            context.pop_back();
        int lev = 0;
        for(list<int>::iterator it = context.begin(); it != context.end(); it++) {
            int next = getChild(node,*it,add,strens_[lev],discs_[lev]);
            lev++;
            if(next < 0) return node;
            node = next;
        }
        return node;
    }

    // add a child node
    int getChild(int node, int wid, bool add, double stren, double disc) {
#ifdef DEBUG_ON
        if(node < 0 || node >= (int)nodes_.size())
            THROW_ERROR("Getting invalid node: "<<node<<" (nodes.size="<<nodes_.size()<<")");
#endif
        ChildMap & myMap = nodes_[node]->children_;
        ChildMap::const_iterator it = myMap.find(wid);
        if(it == myMap.end()) {
            if(!add) return -1;
            int ret;
            if(reuse_.size() != 0) {
                ret = reuse_.back(); reuse_.pop_back();      
            } else {
                ret = nodes_.size(); nodes_.push_back(0);
            }
            myMap.insert(pair<int,int>(wid,ret));
            nodes_[ret] = new PYLMNode(wid,node,stren,disc);
            return ret;
        }
        return it->second;
    }

    // TODO: BAD IMPLEMENTATION, cache the probabilities
    double addCust(int node, int wid, double base) {
#ifdef DEBUG_ON
        if(node < 0 || node >= (int)nodes_.size())
            THROW_ERROR("Adding customer to invalid node: "<<node<<" (nodes.size="<<nodes_.size()<<")");
#endif
        // cerr << "added to "<<wid<<"@"<<node<<": ";
        if(node != 0) {
            double newBase = getProb(nodes_[node]->parent_,wid,base);
            double prob = nodes_[node]->dist_.add(wid,newBase);
            if(nodes_[node]->dist_.tableAdded())
                addCust(nodes_[node]->parent_,wid,base);
            return prob;
        } else {
            return nodes_[node]->dist_.add(wid,base);
        }
    }

    void removeChild(int node, int wid) {
        if(nodes_[node] == NULL) return;
        nodes_[node]->children_.erase(wid);
    }

    double removeCust(int node, int wid, double base) {
#ifdef DEBUG_ON
        if(node < 0 || node >= (int)nodes_.size())
            THROW_ERROR("Removing customer from invalid node: "<<node<<" (nodes.size="<<nodes_.size()<<")");
#endif
        // cerr << "removeCust("<<node<<","<<wid<<","<<base<<")"<<endl;
        if(node != 0) {
            double newBase = getProb(nodes_[node]->parent_,wid,base);
            double prob = nodes_[node]->dist_.remove(wid,newBase);
            if(nodes_[node]->dist_.tableRemoved()) {
                newBase = removeCust(nodes_[node]->parent_,wid,base);
                prob = nodes_[node]->dist_.getProb(wid,newBase);
            }
            if(nodes_[node]->dist_.isEmpty() && nodes_[node]->children_.size() == 0) {
                removeChild(nodes_[node]->parent_,nodes_[node]->wid_);
                delete nodes_[node];
                nodes_[node] = NULL;
                reuse_.push_back(node);
            }
            return prob;
        } else {
            return nodes_[node]->dist_.remove(wid,base);
        }

    }

    // get the probability
    double getProb(int node, int wid, double base) const {
#ifdef DEBUG_ON
        if(node < 0 || node >= (int)nodes_.size())
            THROW_ERROR("Getting probability of invalid node: "<<node<<" (nodes.size="<<nodes_.size()<<")");
        if(nodes_[node] == 0)
            THROW_ERROR("Attempting to use null node at "<<node);
#endif
        if(node != 0)
            base = getProb(nodes_[node]->parent_,wid,base);
        return nodes_[node]->dist_.getProb(wid,base);
    }

    bool isEmpty() const {
        return nodes_.size() == reuse_.size();
    }

    double getStrength(int i) { return strens_[i]; }
    double getDisc(int i) { return discs_[i]; }
    void setStrength(int i, double s) { strens_[i] = s; }
    void setDisc(int i, double s) { discs_[i] = s; }

    // get the number of customers for an ID
    int getCustCount(int node, int wid) const {
        return nodes_[node]->dist_.getTotal(wid);
    }
    // int getTableCount(int node) const { return dist_.getTableCount(); }

    int getArraySize() {
        return nodes_.size();
    }

};

}

#endif
