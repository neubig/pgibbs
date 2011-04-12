#ifndef MODEL_BASE_H__
#define MODEL_BASE_H__

#include "config-base.h"
#include "corpus-base.h"
#include "labels-base.h"

namespace pgibbs {

// a model that processes sentences Sent, and gives them tags Labs
template <class Sent, class Labs>
class ModelBase {

protected:
    ConfigBase conf_;

    // saved variables used in every model
    int iters_, accepted_, sents_, numThreads_, blockSize_;
    bool doShuffle_, skipMH_, printModel_, printStatus_;
    string prefix_;
    vector<int> sentOrder_, sentAccepted_;
    vector<bool> sentInc_;
    double likelihood_, iterTime_; 

public:

    ModelBase(const ConfigBase & conf) : iters_(0), accepted_(0), sents_(0),
        numThreads_(1), blockSize_(1) {
        conf_ = conf;
    }

    virtual ~ModelBase() { };

    // function to initialize the model (add all sentences)
    void initialize(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs);

    // train in a single thread
    void trainInSequence(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs);
    // train in parallel samplers
    void trainInParallel(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs);
    // train using the blocked sampling method
    void trainInBlocks(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs);

    // function to clear the model (remove all sentences and check if empty)
    void clear(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs);

    void printIterationResult(int iter, LabelsBase<Sent,Labs> & labs) const;

    void train(CorpusBase<Sent> & sent, LabelsBase<Sent,Labs> & labs) {
        string sampMeth = conf_.getString("sampmeth");
        if(sampMeth == "sequence")
            this->trainInSequence(sent,labs);
        else if (sampMeth == "parallel")
            this->trainInParallel(sent,labs);
        else if (sampMeth == "block")
            this->trainInBlocks(sent,labs);
        else
            THROW_ERROR("Illegal -sampmeth argument '"<<sampMeth<<"'"<<endl);
    }

    // -------------------- getters/setters -----------------------------

    int getAccepted() const { return accepted_; }
    bool getSkipMH() const { return skipMH_; }
    bool getPrintStatus() const { return printStatus_; }

    // ------------ virtual functions to overload ------------------------

    // processing sentences
    virtual double addSentence(int sid, const Sent & sent, const Labs & labs) = 0;
    virtual double removeSentence(int sid, const Sent & sent, const Labs & labs) = 0;

    // sampling functions
    virtual void cacheProbabilities() = 0;
    virtual pair<double,double> sampleSentence(int sid, const Sent & sent, Labs & oldLabs, Labs & newLabs) const = 0;

    // check to make sure that everything has been removed properly
    virtual void checkEmpty() const = 0;

    // print the model to an output stream
    virtual void print(ostream & out) const = 0;

    // get the model pointer
    virtual ModelBase<Sent,Labs> * clone() const = 0;

};

}

#endif
