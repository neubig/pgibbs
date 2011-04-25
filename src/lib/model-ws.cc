
#include "model-ws.h"
#include "definitions.h"

using namespace pgibbs;

// make words from bounds
WordSent WSModel::makeWords(const WordSent & chars, const Bounds & bounds, bool add) {
#ifdef DEBUG_ON
    if(chars.length() != bounds.size())
        THROW_ERROR("WSModel::makeWords, chars.size="<<chars.length()<<", bounds.size="<<bounds.size());
#endif
    vector<int> beg(1,0);
    for(int i = 0; i < (int)bounds.size(); i++)
        if(bounds[i])
            beg.push_back(i+1);
    WordSent ws(beg.size()-1);
    for(int i = 1; i < (int)beg.size(); i++) {
        if(beg[i]-beg[i-1]>maxLen_)
            THROW_ERROR("String length "<<beg[i]-beg[i-1]<<" (@"<<i<<") exceeded maximum length"<<maxLen_);
        ws[i-1] = symbols_.getId(chars.substr(beg[i-1],beg[i]-beg[i-1]),add);
        // cerr << "adding word "<<ws[i-1]<<" substr("<<beg[i-1]<<","<<beg[i]-beg[i-1]<<")"<<endl;
    }
    return ws;
}

double WSModel::getBase(const GenericString<int> & str) const {
    // cerr << "str.length() == "<<str.length()<< ", bases_.size() == "<<bases_.size()<<endl;
    return bases_[str.length()];
}
double WSModel::getBase(int wid) const {
    return getBase(symbols_.getSymbol(wid));
}

// add the sentence and return the log probability
double WSModel::addSentence(int sid, const WordSent & sent, const Bounds & bounds) {
#ifdef DEBUG_ON
    if((int)sentInc_.size()<=sid) sentInc_.resize((sid+1)*2,false);
    if(sentInc_[sid])
        THROW_ERROR("Double-adding sentence "<<sid<<" to model");
    sentInc_[sid] = true;
#endif
    // add the sent
    WordSent tags = makeWords(sent,bounds,true);
    double logProb = 0;
    for(int i = 0; i < (int)tags.length(); i++) {
        int node = 0;
        for(int j = i-1; i-j<n_ && j >= -1; j--) {
            // cerr << endl<<"addnode("<<tags[j]<<") "<<node; // HERE
            node = lm_.getChild(node,(j==-1?initId_:tags[j]),true,lm_.getStrength(i-j), lm_.getDisc(i-j));
            // cerr << " --> "<<node<<endl;
        }
        double myProb = log(lm_.addCust(node,tags[i],getBase(tags[i])));
        logProb += myProb;
        // cerr << " a(n="<<node<<",t="<<tags[i]<<",l="<<symbols_.getSymbol(tags[i]).length()<<",p="<<myProb<<")";
    }
    // cerr << endl;
    return logProb;
}

// remove the sentence and return the log probability
// TODO: make words include a first non-terminal word
double WSModel::removeSentence(int sid, const WordSent & sent, const Bounds & bounds) {
#ifdef DEBUG_ON
    if(sid >= (int)sentInc_.size() || !sentInc_[sid])
        THROW_ERROR("Double-removing sentence "<<sid<<" from model");
    sentInc_[sid] = false;
#endif
    // add the sent
    WordSent tags = makeWords(sent,bounds,false);
    double logProb = 0;
    for(int i = 0; i < (int)tags.length(); i++) {
        int node = 0;
        for(int j = i-1; i-j<n_ && j >= -1; j--) {
            // cerr << endl<<"remnode("<<tags[j]<<") "<<node;
            node = lm_.getChild(node,(j==-1?initId_:tags[j]),false,0,0);
            // cerr << " --> "<<node<<endl;
        }
        double myProb = log(lm_.removeCust(node,tags[i],getBase(tags[i]))); 
        logProb += myProb;
        // cerr << " r(n="<<node<<",t="<<tags[i]<<",l="<<symbols_.getSymbol(tags[i]).length()<<",p="<<myProb<<")";
    }
    // cerr << endl;
    return logProb;
}

// calculate a sample (if necessary) and return the posterior probability
double WSModel::backwardStep(const WordSent & chars, const vector<Stack> & stacks, Bounds & bounds, bool sample) const {
    
    // get the stack to start
    int ss = stacks.size();

    // if we are not sampling, convert the bounds into a path through the lattice
    vector< pair<int,int> > path;
    if(sample == false) {
        WordSent tags = const_cast<WSModel*>(this)->makeWords(chars,bounds,false);
        path.push_back(pair<int,int>(0,initNode_));
        // cerr << "Lattice 0 (0,"<<initNode_<<")"<<endl;
        int currNode = initNode_;
        int idx = 0;
        for(int i = 0; i < (int)tags.length(); i++) {
            currNode = lm_.getNext(currNode,tags[i]);
            while(!bounds[idx++]);
            // cerr << "Lattice "<<i+1<<" ("<<idx<<","<<currNode<<") t="<<tags[i]<<endl;
            path.push_back(pair<int,int>(idx,currNode));
        }
    }

    // start at the end of the stack 
    double logProb = 0;
    const SegHyp* currHyp = &(stacks[ss-1].begin()->second);
    vector< pair<int,int> > links = currHyp->links_;
    Bounds oldBounds = bounds;
    for(int i = 0; i < (int)bounds.size(); i++) bounds[i] = 0;
    while(links.size() != 0) {
        int nextLink;
        vector<double> myTrans = currHyp->linkProbs_;
        normalizeLogProbs(myTrans);
        // choose which link to use, sample if necessary
        if(sample) {
            nextLink = sampleProbs(myTrans);
            // cerr << "  selected next=("<<links[nextLink].first<<","<<links[nextLink].second<<")"<<endl;
        }
        // otherwise follow the path
        else {
            pair<int,int> currPath = path.back(); path.pop_back();
            for(nextLink = 0; nextLink < (int)links.size() && links[nextLink] != currPath; nextLink++) {
                // cerr << "next=("<<links[nextLink].first<<","<<links[nextLink].second<<"), curr=("<<currPath.first<<","<<currPath.second<<")"<<endl;
            }
            if(nextLink == (int)links.size())
                THROW_ERROR("Could not find path in lattice");
            // cerr << "next=("<<links[nextLink].first<<","<<links[nextLink].second<<"), curr=("<<currPath.first<<","<<currPath.second<<") found!"<<endl;
        }
        // set the boundary, save the probability, and get the next stack
        if(links[nextLink].first != 0)
            bounds[links[nextLink].first-1] = 1;
        logProb += log(myTrans[nextLink]);
        Stack::const_iterator it = stacks[links[nextLink].first].find(links[nextLink].second);
        if(it == stacks[links[nextLink].first].end())
            THROW_ERROR("Could not find selected value in stack");
        currHyp = &(it->second);
        // cerr << " b(b="<<links[nextLink].first<<",n="<<links[nextLink].second<<",p="<<log(myTrans[nextLink])<<")";
        links = currHyp->links_;
    }
    // cerr << endl;

    // return the score
    return logProb;
}

// get a sample for the sentence
pair<double,double> WSModel::sampleSentence(int sid, const WordSent & sent, Bounds & oldTags, Bounds & newTags) const {

    // cerr << "-------------- STARTING -----------------" <<endl;
    int ss = sent.length();

    vector< Stack > stacks (ss+2, Stack());
    // 0.0 probability, terminal symbol 0, initial node
    SegHyp initHyp(initNode_);
    initHyp.prob_ = 0.0;
    stacks[0].insert(Stack::value_type(initNode_,initHyp));

    // do the forward step, build the segmentation lattice
    for(int end = 1; end <= ss; end++) { // end of the string
        Stack & next = stacks[end];
        for(int beg = max(end-maxLen_,0); beg < end; beg++) { // beginning of the string
            // get the substring and base probability
            GenericString<int> nStr = sent.substr(beg,end-beg);
            int wid = symbols_.getId(nStr);
            double base = getBase(nStr);
            // for all stacks in the history
            for(Stack::iterator cit = stacks[beg].begin(); cit != stacks[beg].end(); cit++) {
                double myProb = log(lm_.getProb(cit->second.node_,wid,base)); // get the prob for wid
                int node = lm_.getNext(cit->second.node_,wid); // get the LM state after wid
                Stack::iterator nit = next.find(node); // find the state in current stack
                if(nit == next.end()) {
                    next.insert(Stack::value_type(node,SegHyp(node))); // add a new node
                    nit = next.find(node);
                }
                // cerr << "middle links.push_back(n="<<node<<", w="<<wid<<", b="<<beg<<", e="<<end<<", pn="<<cit->second.node_<<", p="<<myProb<<"+"<<cit->second.prob_<<")"<<endl;
                nit->second.links_.push_back(pair<int,int>(beg,cit->second.node_)); // add a back link
                nit->second.linkProbs_.push_back(myProb + cit->second.prob_); // add the link's prob
            }
        }
        // sum the probabilities for the current stack
        for(Stack::iterator nit = next.begin(); nit != next.end(); nit++)
            nit->second.prob_ = addLogProbs(nit->second.linkProbs_);
    }

    // add transitions to the final stack
    int end = ss+1, beg = ss;
    double base = getBase(initString_);
    Stack & next = stacks[end];
    for(Stack::iterator cit = stacks[beg].begin(); cit != stacks[beg].end(); cit++) {
        double myProb = log(lm_.getProb(cit->second.node_,initId_,base)); // get the prob for wid
        Stack::iterator nit = next.find(0); // find the state in current stack
        if(nit == next.end()) {
            next.insert(Stack::value_type(0,SegHyp(0))); // add a new node
            nit = next.find(0);
        }
        // cerr << "final links.push_back(n="<<0<<", w="<<initId_<<", b="<<beg<<", e="<<end<<", pn="<<cit->second.node_<<", p="<<myProb<<"+"<<cit->second.prob_<<")"<<endl;
        nit->second.links_.push_back(pair<int,int>(beg,cit->second.node_)); // add a back link
        nit->second.linkProbs_.push_back(myProb + cit->second.prob_); // add the link's prob
    }
 
    // sample backwards
    // cerr << "-------------- STACKING -----------------" <<endl;
    double probOld = backwardStep(sent,stacks,oldTags,false);
    // cerr << "-------------- SAMPLING -----------------" <<endl;
    double probNew = backwardStep(sent,stacks,newTags,true);
    // cerr << "newTags =";
    // for(int i = 0; i < (int)newTags.size(); i++)
    //     cerr << " " << newTags[i];
    // cerr << endl;
    
    return pair<double,double>(probOld,probNew);

}


void WSModel::initialize(CorpusBase<WordSent> & corp, LabelsBase<WordSent,Bounds> & labs, bool add) {
    initString_ = GenericString<int>(0);
    initId_ = symbols_.getId(initString_,true);
    initNode_ = (lm_.getN()==1?0:lm_.getChild(0,initId_,true,lm_.getStrength(0),lm_.getDisc(0)));
    ModelBase<WordSent,Bounds>::initialize(corp,labs,add);
}
