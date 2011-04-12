#ifndef PYDIST_H__
#define PYDIST_H__

#include "gng/samp-gen.h"
#include "definitions.h"
#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>

#define PRIOR_SA 2.0
#define PRIOR_SB 1.0
#define PRIOR_DA 2.0
#define PRIOR_DB 2.0

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
};

template < class Index >
class PyDist {

private:

    typedef PyTableSet::iterator PyTableSetIter;
    typedef PyTableSet::const_iterator PyTableSetCIter;

    int total_, tables_;
    Index counts_;
    double spAlpha_, spBeta_, dpAlpha_, dpBeta_;

    // whether a table was removed
    bool removedTable_;

    double stren_, disc_;

public:

    PyDist(double stren, double disc) : total_(0), tables_(0), counts_(), 
        spAlpha_(PRIOR_SA), spBeta_(PRIOR_SB), dpAlpha_(PRIOR_DA), dpBeta_(PRIOR_DB),
        stren_(stren), disc_(disc) { }

    double getProb(int id, double base) const {
        const PyTableSet * tab = counts_.getTableSet(id);
        double myCount = tab ? tab->total-disc_*tab->size() : 0;
        // cerr << "getProb("<<id<<","<<base<<"): ( "<<myCount<<"+"<<base<<"*("<<stren_<<"+"<<tables_<<"*"<<disc_<<") )/("<<total_<<"+"<<stren_<<")"<<endl;
        return ( myCount+base*(stren_+tables_*disc_) )/(total_+stren_);
    }
    int getTotal(int id) const {
        return counts_.getTotal(id);
    }

    double getFallbackProb() const {
        return (stren_+tables_*disc_)/(total_+stren_);
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
            while((left -= (*it)-disc_) > 0)
                it++;
        }
        (*it)++;
        set.total++;
        total_++;
    }

    void addNew(int id) {
        PyTableSet & set = counts_.addTableSet(id);
        set.push_back(1);
        set.total++;
        tables_++;
        total_++;
    }

    // add one and return the probability
    double add(int id, double base) {
        double genProb = getProb(id,0), baseProb = base*getFallbackProb(), totProb = genProb+baseProb;
        if(bernoulliSample(genProb/totProb))
            addExisting(id);
        else
            addNew(id);
        return totProb;
    }
     
    double remove(int id, double base) {
#ifdef DEBUG_ON
        if(id < 0)
            THROW_ERROR("Illegal value " << id << " in PyDist::remove");
        if(!counts_.getTableSet(id))
            THROW_ERROR("Overflow in PyDist::remove for " << id); 
#endif
        removedTable_ = false;
        PyTableSet & set = counts_.addTableSet(id);
#ifdef DEBUG_ON
        if(set.total == 0)
            THROW_ERROR("Overflow in PyDist::remove for " << id); 
#endif
        int mySize = set.size();
        PyTableSetIter it = set.begin();
        
        if(mySize > 1) {
            int left = (int)(((double)rand())/RAND_MAX*set.total);
            while((left -= (*it)) >= 0)
                it++;
        }
        (*it)--;
        set.total--;
        total_--;
        if((*it) == 0) {
            removedTable_ = true;
            tables_--;
            set.erase(it);
        }
        return getProb(id,base);
    }

    // sample the parameters
    void sampleParameters() {
        double x = total_ > 1 ? betaSample(stren_+1,total_-1) : 1;
        double y = 0, z = 0;
        for(int i = 1; i < tables_; i++)
            y += bernoulliSample(stren_/(stren_+disc_*i));
        for(typename Index::iterator it2 = counts_.begin(); it2 != counts_.end(); it2++) {
            const PyTableSet & set = counts_.iterTableSet(it2);
            for(PyTableSet::const_iterator it = set.begin();
                    it != set.end(); it++) { 
                for(int k = 1; k < (*it); k++) {
                    z += (1-bernoulliSample((k-1)/(k-disc_)));
                }
            }
        }
        // std::cerr << "disc_ = betaSample("<<dpAlpha_<<"+"<<(tables_-1)<<"-"<<y<<","<<dpBeta_<<"+"<<z<<")"<<std::endl;
        disc_ = betaSample(dpAlpha_+(tables_-1)-y,dpBeta_+z);
        // std::cerr << "stren_ = gammaSample("<<spAlpha_<<"+"<<y<<","<<spBeta_<<"-log("<<x<<"))"<<std::endl;
        stren_ = gammaSample(spAlpha_+y,spBeta_-log(x));
    }

    double getStrength() const { return stren_; }
    void setStrength(double stren) { stren_ = stren; }
    double getDiscount() const { return disc_; }
    void setDiscount(double disc) { disc_ = disc; }
    int getTableCount() const { return tables_; }

    void setDiscountAlpha(double v) { dpAlpha_ = v; }
    void setDiscountBeta(double v) { dpBeta_ = v; }
    void setStrengthAlpha(double v) { spAlpha_ = v; }
    void setStrengthBeta(double v) { spBeta_ = v; }

};

}

#endif
