#ifndef MODEL_WS_H__
#define MODEL_WS_H__

#include "model-base.h"
#include "config-ws.h"
#include "labels-ws.h"
#include "dist-py.h"
#include "dist-pylm.h"
#include <map>

using namespace std;

namespace pgibbs {

class SegHyp {

public:

    int node_;                      // pointer to the language model state
    double prob_;                   // the probability of this node
    vector< pair<int,int> > links_; // back-links to previous node
    vector< double > linkProbs_;    // probabilities of each link

    SegHyp (int node) : node_(node), prob_(NEG_INFINITY) { }

};

class WSModel : public ModelBase<WordSent,Bounds> {

protected:
    // stack of hypotheses
    typedef map<int,SegHyp> Stack;
    
    // a Pitman-Yor language model for words
    PYLM lm_;

    // number of unique characters to handle
    int chars_, n_, maxLen_;

    // base values
    vector<double> bases_;

    // initial values
    GenericString<int> initString_;
    int initNode_, initId_;
    
    typedef SymbolSet< GenericString<int>, int, GenericStringHash<int> > Symbols;
    Symbols symbols_; 

    double str_,disc_,strA_,strB_,discA_,discB_;

public:
    WSModel(const WSConfig & conf) : ModelBase<WordSent,Bounds>(conf), 
        chars_(conf.getInt("chars")), n_(conf.getInt("n")), 
        maxLen_(conf.getInt("maxlen")),
        str_(conf.getDouble("str")), disc_(conf.getDouble("disc")),
        strA_(conf.getDouble("stra")), strB_(conf.getDouble("strb")),
        discA_(conf.getDouble("disca")), discB_(conf.getDouble("discb"))
    {
#ifdef DEBUG_ON
        if(chars_ == 0)
            THROW_ERROR("No chars in WS Model");
#endif
        lm_.setN(n_);
        for(int i = 0; i < n_; i++) {
            lm_.setStrength(i,str_);
            lm_.setDisc(i,disc_);
        }
        double baseChar = conf.getDouble("avglen");
        bases_ = vector<double>(maxLen_+1);
        bases_[0] = 1/(1+baseChar);
        baseChar = baseChar/(1+baseChar)/chars_;
        for(int i = 1; i <= maxLen_; i++)
            bases_[i] = bases_[i-1]*baseChar;

    }

    ~WSModel() { }

    // print the words
    string printWords(const WordCorpus & corp, const WordSent & tags);

    // virtual functions for processing sentences
    double addSentence(int sid, const WordSent & sent, const Bounds & labs);
    double removeSentence(int sid, const WordSent & sent, const Bounds & labs);

    double backwardStep(const WordSent & chars, const vector<Stack> & stacks, Bounds & bounds, bool sample) const;

    // virtual functions for processing sentences
    void cacheProbabilities() { };
    pair<double,double> sampleSentence(int sid, const WordSent & sent, Bounds & oldLabs, Bounds & newLabs) const;

    void initialize(CorpusBase<WordSent> & corp, LabelsBase<WordSent,Bounds> & labs, bool add);

    double getBase(const GenericString<int> & str) const;
    double getBase(int wid) const;
    WordSent makeWords(const WordSent & sent, const Bounds & bounds, bool add);

    void checkEmpty() const {
        if(!lm_.isEmpty())
            THROW_ERROR("LM is not empty");
    }

    void print(ostream & out) const {
        THROW_ERROR("WSModel::print not implemented yet");
    }

    ModelBase<WordSent,Bounds> * clone() const { 
        return new WSModel(*this);
    }

    void sampleParameters() {
        Symbols::Map & map = symbols_.getMap();
        vector<int> toRemove;
        for(Symbols::Map::iterator it = map.begin(); it != map.end(); it++)
            if(lm_.getCustCount(0,it->second) == 0)
                toRemove.push_back(it->second);
        for(int i = 0; i < (int)toRemove.size(); i++)
            symbols_.removeId(toRemove[i]);
        lm_.sampleParameters(strA_,strB_,discA_,discB_);
        for(int i = 0; i < n_; i++)
            cerr << " v("<<i+1<<")="<<lm_.getLevelSize(i)<<", s("<<i+1<<")="<<lm_.getStrength(i)<<", d("<<i+1<<")="<<lm_.getDisc(i)<<endl;
        cerr << " lmarr=" << lm_.getArraySize() << ", vocabarr="<<symbols_.capacity()<<","<<symbols_.hashCapacity() <<endl;
    }

};

}

#endif
