#ifndef MODEL_HMM_H__
#define MODEL_HMM_H__

#include "model-base.h"
#include "config-hmm.h"
#include "labels-hmm.h"
#include "dist-py.h"

namespace pgibbs {

class HMMModel : public ModelBase<WordSent,ClassSent> {

protected:
    
    // number of unique words and classes to handle
    int classes_, words_;

    // type of the base distribution
    string base_;

    // distributions
	vector< PyDist<PyDenseIndex>* > tDists_;
	vector< PyDist<PySparseIndex>* > eDists_;

    // cache of the transition probabilities
    vector<double> tMat_;

    // cached probabilities
    vector<double> baseE_, baseT_;
    double tStrA_, tStrB_, tDiscA_, tDiscB_;
    double eStrA_, eStrB_, eDiscA_, eDiscB_;    

public:
    HMMModel(const HMMModel & mod) : ModelBase<WordSent,ClassSent>(mod),
    classes_(mod.classes_), words_(mod.words_), base_(mod.base_), tMat_(mod.tMat_), 
    baseE_(mod.baseE_), baseT_(mod.baseT_), 
    tStrA_(mod.tStrA_), tStrB_(mod.tStrB_), tDiscA_(mod.tDiscA_), tDiscB_(mod.tDiscB_), 
    eStrA_(mod.eStrA_), eStrB_(mod.eStrB_), eDiscA_(mod.eDiscA_), eDiscB_(mod.eDiscB_)
    {
	    tDists_=vector< PyDist<PyDenseIndex>* >(mod.tDists_.size());
	    eDists_=vector< PyDist<PySparseIndex>* >(mod.eDists_.size());
        for(int i = 0; i < (int)tDists_.size(); i++) {
            tDists_[i] = new PyDist<PyDenseIndex>(*mod.tDists_[i]);
            eDists_[i] = new PyDist<PySparseIndex>(*mod.eDists_[i]);
        }
    }

    HMMModel(const HMMConfig & conf) : ModelBase<WordSent,ClassSent>(conf), 
        classes_(conf.getInt("classes")), words_(conf.getInt("words")), 
        base_(conf.getString("base")),
        baseE_(words_+1,1.0/words_), baseT_(classes_+1,1.0/(classes_+1)), 
        tStrA_(conf.getDouble("tstra")), tStrB_(conf.getDouble("tstrb")),
        tDiscA_(conf.getDouble("tdisca")), tDiscB_(conf.getDouble("tdiscb")),
        eStrA_(conf.getDouble("estra")), eStrB_(conf.getDouble("estrb")),
        eDiscA_(conf.getDouble("edisca")), eDiscB_(conf.getDouble("ediscb"))
    {
#ifdef DEBUG_ON
        if(words_ == 0)
            THROW_ERROR("No words in HMM Model");
        if(classes_ == 0)
            THROW_ERROR("No classes in HMM Model");
        cout << "Words == "<<words_<<" Classes == "<<classes_<<endl;
#endif
        tDists_ = vector< PyDist<PyDenseIndex>* >(classes_+1);
        eDists_ = vector< PyDist<PySparseIndex>* >(classes_+1);
        for(int i = 0; i <= classes_; i++) {
            tDists_[i] = new PyDist<PyDenseIndex>(conf.getDouble("tstr"), conf.getDouble("tdisc"));
            eDists_[i] = new PyDist<PySparseIndex>(conf.getDouble("estr"), conf.getDouble("edisc"));
        }
    }

    ~HMMModel() {
        for(int i = 0; i < (int)tDists_.size(); i++)
            delete tDists_[i];
        for(int i = 0; i < (int)eDists_.size(); i++)
            delete eDists_[i];

    }

    void initialize(CorpusBase<WordSent> & corp, LabelsBase<WordSent,ClassSent> & labs);

    // virtual functions for processing sentences
    double addSentence(int sid, const WordSent & sent, const ClassSent & labs);
    double removeSentence(int sid, const WordSent & sent, const ClassSent & labs);

    // virtual function for processing sentences
    double backwardStep(const vector<double> & forProbs, ClassSent & tags, bool sample) const;

    // virtual functions for processing sentences
    void cacheProbabilities();
    pair<double,double> sampleSentence(int sid, const WordSent & sent, ClassSent & oldLabs, ClassSent & newLabs) const;

    void checkEmpty() const {
        for(int i = 0; i < (int)tDists_.size(); i++)
            if(tDists_[i]->getTableCount() != 0)
                THROW_ERROR("Transition model "<<i<<" not equal to zero "<<tDists_[i]->getTableCount());
        for(int i = 0; i < (int)eDists_.size(); i++)
            if(eDists_[i]->getTableCount() != 0)
                THROW_ERROR("Emission model "<<i<<" not equal to zero "<<eDists_[i]->getTableCount());
    }

    void print(ostream & out) const {
        out << "T str="<<tDists_[0]->getStrength()<<" disc="<<tDists_[0]->getDiscount()<<endl;
        out << "E str="<<eDists_[0]->getStrength()<<" disc="<<eDists_[0]->getDiscount()<<endl<<endl;
        
        out << "T Matrix:"<<endl;
        out << "TODO" <<endl<<endl;

        out << "E Matrix:"<<endl;
        out << "TODO" <<endl<<endl;
    }

    ModelBase<WordSent,ClassSent> * clone() const { 
        return new HMMModel(*this);
    }

    void sampleParameters();

};

}

#endif
