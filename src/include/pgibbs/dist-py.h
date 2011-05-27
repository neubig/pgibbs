#ifndef PYDIST_H__
#define PYDIST_H__

#include "gng/samp-gen.h"
#include "definitions.h"
#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace pgibbs {

class PyTableSet : public std::vector< int > {

public:
    int total;
    PyTableSet() : std::vector< int >(), total(0) { }

};

class PyDenseIndex {
protected:
    std::vector< PyTableSet > idx_;
public:
    typedef std::vector< PyTableSet >::iterator iterator;
    iterator begin() { return idx_.begin(); }
    iterator end() { return idx_.end(); }
    PyTableSet & iterTableSet(iterator it) { return *it; }
    
    const PyTableSet * getTableSet(int id) const { 
        return (int)idx_.size() > id ? &(idx_[id]) : 0; 
    }
    int getTotal(int id) const {
        return (int)idx_.size() > id ? (idx_[id]).total : 0; 
    }
    PyTableSet & addTableSet(int id) {
        if((int)idx_.size() <= id) idx_.resize(id+1);
        return idx_[id];
    }
    void removeTableSet(int id) { } // do not remove, we're dense, remember?
};

class PySparseIndex {
protected:
    std::tr1::unordered_map< int, PyTableSet > idx_;
public:
    typedef std::tr1::unordered_map< int, PyTableSet >::const_iterator const_iterator;
    typedef std::tr1::unordered_map< int, PyTableSet >::iterator iterator;
    iterator begin() { return idx_.begin(); }
    iterator end() { return idx_.end(); }
    PyTableSet & iterTableSet(iterator it) { return it->second; }

    const PyTableSet * getTableSet(int id) const {
        const_iterator it = idx_.find(id);
        return it == idx_.end() ? 0 : & it->second;
    }
    int getTotal(int id) const {
        const_iterator it = idx_.find(id);
        return it == idx_.end() ? 0 : it->second.total;
    }
    PyTableSet & addTableSet(int id) {
        iterator it = idx_.find(id);
        if(it == idx_.end()) 
            it = idx_.insert( std::pair< int, PyTableSet >(id,PyTableSet()) ).first;
        return it->second;
    }
    void removeTableSet(int id) {
        idx_.erase(id);
    }
};

template < class Index >
class PyDist {

private:

    typedef PyTableSet::iterator PyTableSetIter;
    typedef PyTableSet::const_iterator PyTableSetCIter;

    int customers_, tables_;
    Index counts_;
    // double spAlpha_, spBeta_, dpAlpha_, dpBeta_;

    double stren_, disc_;
    bool tableAdded_, tableRemoved_;

public:

    PyDist(double stren, double disc) : customers_(0), tables_(0), counts_(), 
        stren_(stren), disc_(disc) { }

    double getProb(int id, double base) const {
        const PyTableSet * tab = counts_.getTableSet(id);
        double myCount = tab ? tab->total-disc_*tab->size() : 0;
        // if(debug)
        //     cerr << " getProb("<<id<<","<<base<<"): ( "<<myCount<<"+"<<base<<"*("<<stren_<<"+"<<tables_<<"*"<<disc_<<") )/("<<customers_<<"+"<<stren_<<")";
        return ( myCount+base*(stren_+tables_*disc_) )/(customers_+stren_);
    }
    int getTotal(int id) const {
        return counts_.getTotal(id);
    }

    double getFallbackProb() const {
        return (stren_+tables_*disc_)/(customers_+stren_);
    }

    std::vector<double> getAllProbs(const std::vector<double> & bases) const {
#ifdef DEBUG_ON
        if(bases.size() != counts_.size())
            throw std::runtime_error("Mismatched bases and probs");
#endif
        std::vector<double> ret(bases.size());
        for(int i = 0; i < ret.size(); i++)
            ret[i] = getProb(i,bases[i]);
        return ret;
    }

    std::vector<double> getAllLogProbs(const std::vector<double> & bases) const {
        std::vector<double> ret = getAllProbs(bases);
        for(int i = 0; i < ret.size(); i++)
            ret[i] = log(ret[i]);
        return ret;
    }

    void addExisting(int id) {
        PyTableSet & set = counts_.addTableSet(id);
#ifdef DEBUG_ON
        if(set.total == 0)
            throw std::runtime_error("PyDist::addExisting with no tables");
#endif
        int mySize = set.size();
        PyTableSetIter it = set.begin();
        if(mySize > 1) {
            double left = rand()*(set.total-mySize*disc_)/RAND_MAX;
            while((left -= (*it)-disc_) > 0) {
                if(it + 1 == set.end()) break;
                it++;
            }
        }
        (*it)++;
        set.total++;
        customers_++;
        tableAdded_ = false;
    }

    void addNew(int id) {
        PyTableSet & set = counts_.addTableSet(id);
        set.push_back(1);
        set.total++;
        tables_++;
        customers_++;
        tableAdded_ = true;
    }

    // add one and return the probability
    double add(int id, double base) {
        double genProb = getProb(id,0), baseProb = base*getFallbackProb(), totProb = genProb+baseProb;
        tableAdded_ = !bernoulliSample(genProb/totProb);
        if(tableAdded_)
            addNew(id);
        else
            addExisting(id);
        return totProb;
    }
     
    double remove(int id, double base) {
#ifdef DEBUG_ON
        if(id < 0)
            THROW_ERROR("Illegal value " << id << " in PyDist::remove");
        if(!counts_.getTableSet(id))
            THROW_ERROR("Overflow in PyDist::remove for " << id); 
#endif
        tableRemoved_ = false;
        PyTableSet & set = counts_.addTableSet(id);
#ifdef DEBUG_ON
        if(set.total == 0)
            THROW_ERROR("Overflow in PyDist::remove for " << id); 
#endif
        int mySize = set.size();
        PyTableSetIter it = set.begin();
        
        if(mySize > 1) {
            int left = rand() % set.total;
            while((left -= (*it)) >= 0)
                it++;
        }
        (*it)--;
        set.total--;
        customers_--;
        if((*it) == 0) {
            tableRemoved_ = true;
            tables_--;
            set.erase(it);
        }
        if(set.total == 0)
            counts_.removeTableSet(id);
        return getProb(id,base);
    }

    // --------- tied sampling for multiple distributions ---------------

protected:
    // functions to gather counts from multiple distributions
    typedef unordered_map<int, int> CountMap;
    void addCount(CountMap & map, int place) {
        pair<CountMap::iterator,bool> p = map.insert(pair<int,int>(place,0));
        p.first->second++;
    }
    void gatherCounts(CountMap & nodeCustCounts,CountMap & nodeTableCounts,CountMap & tableCustCounts, int & totalCustCount, int & totalTableCount) {
        totalCustCount += customers_;
        totalTableCount += tables_;
        pair<CountMap::iterator,bool> ret;
        if(tables_ > 1) {
            addCount(nodeTableCounts,tables_);
            addCount(nodeCustCounts,customers_);
        }
        for(typename Index::iterator it = counts_.begin(); it != counts_.end(); it++) {
            const PyTableSet & tabs = counts_.iterTableSet(it);
            const int tabsize = tabs.size();
            for(int i = 0; i < tabsize; i++)
                if(tabs[i] > 1)
                    addCount(tableCustCounts,tabs[i]);
        }
    }

public:


    // sample parameters for several distributions with tied parameters
    //  use da, db, sa, sb as prior probabilities
    static std::pair<double,double> sampleParameters(std::vector<PyDist*> & dists, double sa, double sb, double da, double db) { 
        // gather the counts
        CountMap nodeTableCounts, nodeCustCounts, tableCustCounts;
        int totalCustCount = 0, totalTableCount = 0;
        double stren = dists[0]->getStrength(), disc = dists[0]->getDiscount();
        for(int i = 0; i < (int)dists.size(); i++) {
#ifdef DEBUG_ON
            if(dists[i]->getStrength() != stren || dists[i]->getDiscount() != disc)
                THROW_ERROR("Attempted to sample for a mixture where the strength/disc are not the same");
#endif
            dists[i]->gatherCounts(nodeCustCounts,nodeTableCounts,tableCustCounts,totalCustCount,totalTableCount);
        }
        // calculate and add the auxiliary variables
        int yui = 0;
        for(CountMap::const_iterator it = nodeTableCounts.begin(); it != nodeTableCounts.end(); it++) {
            for(int j = 1; j < it->first; j++) {
                for(int k = 0; k < it->second; k++) {
                    yui = bernoulliSample(stren/(stren+disc*j));
                    da += (1-yui);
                    sa += yui;
                }
            }
        }
        for(CountMap::const_iterator it = nodeCustCounts.begin(); it != nodeCustCounts.end(); it++)
            for(int k = 0; k < it->second; k++)
                sb -= log(betaSample(stren+1,it->first-1));
        for(CountMap::const_iterator it = tableCustCounts.begin(); it != tableCustCounts.end(); it++)
            for(int j = 1; j < it->first; j++)
                for(int k = 0; k < it->second; k++)
                    db += (1-bernoulliSample((j-1)/(j-disc)));
        // sample the new parameters and re-set the parameters of the distributions
        pair<double,double> ret( gammaSample(sa,1/sb), betaSample(da,db) );
        for(int i = 0; i < (int)dists.size(); i++) {
            dists[i]->setStrength(ret.first);
            dists[i]->setDiscount(ret.second);
        }
        return ret;
    }
    
    double getStrength() const { return stren_; }
    void setStrength(double stren) { stren_ = stren; }
    double getDiscount() const { return disc_; }
    void setDiscount(double disc) { disc_ = disc; }
    int getTableCount() const { return tables_; }
    bool tableAdded() const { return tableAdded_; }
    bool tableRemoved() const { return tableRemoved_; }
    bool isEmpty() const { return tables_ == 0; }

};

}

#endif
