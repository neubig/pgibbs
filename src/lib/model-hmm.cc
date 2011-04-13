
#include "model-hmm.h"
#include "definitions.h"

using namespace pgibbs;

// add the sentence and return the log probability
double HMMModel::addSentence(int sid, const WordSent & sent, const ClassSent & tags) {
#ifdef DEBUG_ON
    if(sent.size() != tags.size())
        THROW_ERROR("Sent length != tag length ("<<sent.size()<<" != "<<tags.size()<<")");
    if(sentInc_[sid])
        THROW_ERROR("Double-adding sentence "<<sid<<" to model");
#endif
    sentInc_[sid] = true;
    double logProb = 0;
    int sl = sent.size(), i;
    for(i = 1; i < sl-1; i++) {
        // get transition probs and emission probs for all but the last transition
        logProb += log( 
                tDists_[tags[i-1]]->add(tags[i],baseT_[tags[i]]) *
                eDists_[tags[i]]->add(sent[i],baseE_[sent[i]])
            );
        // cerr << " logProb = "<<logProb<<endl;
    }
    logProb += log(tDists_[tags[i-1]]->add(tags[i],baseT_[tags[i]])); // get the last
    return logProb;
}

// remove the sentence and return the log probability
double HMMModel::removeSentence(int sid, const WordSent & sent, const ClassSent & tags) {
#ifdef DEBUG_ON
    if(sent.size() != tags.size())
        THROW_ERROR("Sent length != tag length ("<<sent.size()<<" != "<<tags.size()<<")");
    if(!sentInc_[sid])
        THROW_ERROR("Double-removing sentence "<<sid<<" from model");
#endif
    sentInc_[sid] = false;
    double logProb = 0;
    int sl = sent.size(), i;
    for(i = 1; i < sl-1; i++) {
        // get transition probs and emission probs for all but the last transition
        logProb += log( 
                tDists_[tags[i-1]]->remove(tags[i],baseT_[tags[i]]) *
                eDists_[tags[i]]->remove(sent[i],baseE_[sent[i]])
            );
    }
    logProb += log(tDists_[tags[i-1]]->remove(tags[i],baseT_[tags[i]])); // get the last
    return logProb;
}

// cache probabilities that can be used with multiple samples
void HMMModel::cacheProbabilities() {
    // calculate the translation matrix
    int cl = classes_+1, tl = cl*cl, idx=0;
    tMat_.resize(tl);
    for(int i = 0; i < cl; i++)
        for(int j = 0; j < cl; j++)
            tMat_[idx++] = log(tDists_[i]->getProb(j,baseT_[j]));
}

// calculate a sample (if necessary) and return the posterior probability
double HMMModel::backwardStep(const vector<double> & forProbs, ClassSent & tags, bool sample) const {
    // for each word, backwards
    int cl = classes_+1, sl = tags.size()-1;
    vector<double> myTrans(classes_);
    double totalProb = 0;
    for(int i = sl-1; i > 0; i--) {
        // for each potential tag
        int prev = tags[i+1];
        for(int j = 0; j < classes_; j++) {
            myTrans[j] = tMat_[j*cl+prev] + forProbs[i*cl+j];
            // cout << " myTrans["<<j<<"]("<<myTrans[j]<<") = tMat_["<<j*cl+prev<<"]("<<tMat_[j*cl+prev]<<") + forProbs["<<i*cl+j<<"]("<<forProbs[i*cl+j]<<")"<<endl;
        }
        normalizeLogProbs(myTrans);
        // sample if necessary
        if(sample) 
            tags[i] = sampleProbs(myTrans);
        // cerr << " chose "<<tags[i]<<endl;
        totalProb += log(myTrans[tags[i]]);
    }
    return totalProb;
}

// get a sample for the sentence
pair<double,double> HMMModel::sampleSentence(int sid, const WordSent & sent, ClassSent & oldTags, ClassSent & newTags) const {

    // initialize
    int cl = classes_+1, sl = newTags.size()-1;

    // sanity check
#ifdef DEBUG_ON
    if((int)tMat_.size() != cl*cl)
        THROW_ERROR("Transition matrix size "<<tMat_.size()<<" is not " << cl*cl);
    if(sent.size() != newTags.size())
        THROW_ERROR("Sentence size "<<sent.size()<<" != tag size "<<newTags.size());
    if(sentInc_[sid])
        THROW_ERROR("Sampling sentence "<<sid<<" that is already included");
#endif

    vector<double> forProbs(cl*sl,NEG_INFINITY); forProbs[classes_] = 0;
    vector<double> myTrans(cl);


    // calculate the emission matrix and forward step
    // pos in sentence
    for(int i = 1; i < sl; i++) {
        // current class
        for(int j = 0; j < classes_; j++) {
            // previous class
            for(int k = 0; k < cl; k++) {
                // cerr << "for: myTrans["<<k<<"] = forProbs["<<(i-1)*cl+k<<"]("<<forProbs[(i-1)*cl+k]<<") + tMat_["<<k*cl+j<<"]("<<tMat_[k*cl+j]<<")"<<endl;
                myTrans[k] = forProbs[(i-1)*cl+k] + tMat_[k*cl+j];
            }
            // sum the values
            // cerr << "setting forProbs["<<i<<"*cl+"<<j<<"] = "<<addLogProbs(myTrans) <<" + "<<log(eDists_[j]->getProb(sent[i],baseE_[sent[i]]))<<endl;
            forProbs[i*cl+j] = addLogProbs(myTrans) + 
                                log(eDists_[j]->getProb(sent[i],baseE_[sent[i]]));
        }
    }
    
    // sample backwards
    myTrans.resize(classes_);
    double probOld = backwardStep(forProbs,oldTags,false);
    double probNew = backwardStep(forProbs,newTags,true);
    
    return pair<double,double>(probOld,probNew);

}


void HMMModel::initialize(CorpusBase<WordSent> & corp, LabelsBase<WordSent,ClassSent> & labs) {
    ModelBase<WordSent,ClassSent>::initialize(corp,labs);
    if(base_ == "unigram") {
        int cs = corp.size(), ss, sum = 0;
        baseE_ = vector<double>(words_);
        for(int i = 0; i < cs; i++) {
            ss = corp[i].size()-1;
            for(int j = 1; j < ss; j++) 
                baseE_[corp[i][j]]++;
            sum += ss-1;
        }
        for(int i = 0; i < words_; i++) {
            baseE_[i] /= sum;
            // cerr << "baseE_["<<i<<"](" << corp.getSymbol(i) << ") == " << baseE_[i] << endl;
        }
    }
}

// sample the parameters
void HMMModel::sampleParameters() {
    pair<double,double> tpar = PyDist<PyDenseIndex>::sampleParameters(tDists_,tStrA_,tStrB_,tDiscA_,tDiscB_);
    pair<double,double> epar = PyDist<PySparseIndex>::sampleParameters(eDists_,eStrA_, eStrB_, eDiscA_, eDiscB_);
    cerr << "tstr="<<tpar.first<<", tdisc="<<tpar.second<< ", estr="<<epar.first<<", edisc="<<epar.second << endl;
}
