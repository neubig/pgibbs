
#include "model-base.h"
#include "model-hmm.h"
#include "model-ws.h"
#include "gng/misc-func.h"

using namespace pgibbs;

// the information needed for a single sampling pass
template <class Sent, class Labs>
class BlockJob {

protected:
    int numSents_;

public:

    // the thread
    pthread_t thread_;
    
    // which iteration we're on
    int iter_;

    // corpus values
    ModelBase<Sent,Labs> * mod_;
    CorpusBase<Sent> * corp_;

    // input
    int* sentOrder_;

    // input/output
    LabelsBase<Sent,Labs> * labs_;
    
    // output
    pair<double,double> propProbs_;

    // statistics
    double likelihood_;
    int accepted_, sents_;
    vector<int> sentAccepted_;

    BlockJob(ModelBase<Sent,Labs> * mod, CorpusBase<Sent> * corp, LabelsBase<Sent,Labs> * labs) : mod_(mod), corp_(corp), labs_(labs) { };
    
    void setNumSents(int numSents) {
        numSents_ = numSents;
        sentAccepted_.resize(numSents_,0);
    }
    int getNumSents() { return numSents_; }

};

template <class Sent, class Labs>
void ModelBase<Sent,Labs>::initialize(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs, bool add) {

    // add all sentences in the corpus
    double likelihood = 0;
    int i,cs = corp.size();
    sentOrder_.resize(cs);
    sentInc_.resize(cs,false);
    if(add) {
        for(i = 0; i < cs; i++) {
            likelihood += addSentence(i,corp[i],labs[i]);
            sentOrder_[i] = i;
        }
        cout << "Likelihood after initialization: " << likelihood << endl;
    }

    // variables shared by all training processes
    iters_ = conf_.getInt("iters");
    doShuffle_ = conf_.getBool("shuffle");
    skipIters_ = conf_.getInt("skipiters");
    printModel_ = conf_.getBool("printmod");
    numThreads_ = conf_.getInt("threads"); blockSize_ = conf_.getInt("blocksize"); 
    sentAccepted_ = vector<int>(cs,0);
    sampParam_ = conf_.getBool("sampparam");

    // seed the random number generator, with the time if necessary
    srand( conf_.getInt("randseed") > 0 ? conf_.getInt("randseed") : time(NULL) );

    // get the main arguments
    vector<string> mainArgs = conf_.getMainArgs();
    if(mainArgs.size() > 0)
        prefix_ = mainArgs[mainArgs.size()-1];

}

template <class Sent, class Labs>
void ModelBase<Sent,Labs>::clear(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs) {

    // add all sentences in the corpus
    int i,cs = corp.size();
    for(i = 0; i < cs; i++)
        removeSentence(i,corp[i],labs[i]);
    checkEmpty();

}


// print the results of one iteration
template <class Sent, class Labs>
void ModelBase<Sent,Labs>::printIterationResult(int iter, const CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs) const {
    // skip printing iterations for larger values
    if(iter <= 50 ||  (iter <= 1000 && iter % 10 == 0) || iter % 50 == 0) {
        if(printModel_) {
            // print the model
            ostringstream modName; modName << prefix_ << "."<<iter<<".mod";
            ofstream modOut(modName.str().c_str());
            this->print(modOut);
            modOut.close();
        }
        // print the labels
        ostringstream labName; labName << prefix_ << "."<<iter<<".lab";
        ofstream labOut(labName.str().c_str());
        labs.print(corp,labOut); 
        labOut.close();
    }
    cout << endl << "Iteration "<<iter<< " (time: "<<iterTime_<<"s)" << endl
         << " Likelihood: "<<likelihood_<<endl
         << " Acceptance rate: "<<100.0*accepted_/labs.size() <<"%" << endl;
}

// perform a single sampling pass over the sentences in myOrder
//  model is updated as the pass continues
template <class Sent, class Labs>
void* samplingPass(void* ptr) {
    BlockJob<Sent,Labs> & job = *(BlockJob<Sent,Labs>*)ptr;
    job.likelihood_ = 0; job.accepted_ = 0; job.sents_ = 0;

    // perform sampling in sequence
    double accept;
    pair<double,double> trueProbs, propProbs;
    Labs oldTags;
    for(int i = 0; i < job.getNumSents(); i++) {
        int s=job.sentOrder_[i];
        // sampling from the proposal distribution and calculating probs
        oldTags = (*job.labs_)[s];
        trueProbs.first = job.mod_->removeSentence(s,(*job.corp_)[s],oldTags);
        job.mod_->cacheProbabilities();
        propProbs = job.mod_->sampleSentence(s,(*job.corp_)[s],oldTags,(*job.labs_)[s]);
        trueProbs.second = job.mod_->addSentence(s,(*job.corp_)[s],(*job.labs_)[s]);

        // performing the metropolis-hastings step
        accept = trueProbs.second-trueProbs.first+propProbs.first-propProbs.second;
        // cerr << s << " (tn=" <<trueProbs.second<<")-(tc="<<trueProbs.first<<")+(pc="<<propProbs.first<<")-(pn="<<propProbs.second<<") == " <<accept<< endl;

        if(job.mod_->getSkipIters() >= job.iter_ || accept >= 0 || bernoulliSample(exp(accept))) {
            // cout << ": accepted"<<endl;
            job.accepted_++;
            job.sentAccepted_[i]++;
            job.likelihood_ += trueProbs.second;
        } else {
            // cout << ": REJECTED!"<<endl;
            job.mod_->removeSentence(s,(*job.corp_)[s],(*job.labs_)[s]);
            (*job.labs_)[s] = oldTags;
            job.mod_->addSentence(s,(*job.corp_)[s],(*job.labs_)[s]);
            job.likelihood_ += trueProbs.first;
        }

        if(++job.sents_ % 1000 == 0 && job.mod_->getPrintStatus()) {
            cout << "\r" << job.sents_;
            cout.flush();
        }

    }
    return NULL;

}

// train via normal gibbs sampling
template <class Sent, class Labs>
void ModelBase<Sent,Labs>::trainInSequence(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs) {

    // initialize the model
    initialize(corp,labs,true);

    // training variables
    int cs = corp.size();
    printStatus_ = true;
    sentAccepted_ = vector<int>(cs,0);
    timeval tStart, tEnd;

    // make the job
    BlockJob<Sent,Labs> job(this,&corp,&labs);
    job.sentOrder_ = &sentOrder_[0];
    job.setNumSents(sentOrder_.size());

    // perform training
    for(int iter = 1; iter <= iters_; iter++) {

        // shuffle the sentences
        if(doShuffle_) 
            shuffle(sentOrder_);

        // do a sampling pass over the whole corpus 
        gettimeofday(&tStart, NULL);
        job.iter_ = iter;
        samplingPass<Sent,Labs>(&job);
        likelihood_ = job.likelihood_; accepted_ = job.accepted_;
        gettimeofday(&tEnd, NULL);
        iterTime_ = timeDifference(tStart,tEnd);

        // print information about the iteration
        printIterationResult(iter,corp,labs);

        // sample the parameters
        if(sampParam_)
            sampleParameters();
        
    }

}

// train via parallel samples
template <class Sent, class Labs>
void ModelBase<Sent,Labs>::trainInParallel(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs) {

    // initialize the model
    initialize(corp,labs,true);

    // training variables
    int cs = corp.size();
    printStatus_ = true;
    timeval tStart, tEnd;
    
    // divide the parts of the array to be handled by each thread
    vector< BlockJob<Sent,Labs> > jobs(numThreads_, BlockJob<Sent,Labs>(this,&corp,&labs) );
    for(int i = 0; i < numThreads_; i++) {
        jobs[i].sentOrder_ = (&sentOrder_[0])+i*cs/numThreads_;
        jobs[i].setNumSents((i+1)*cs/numThreads_ - i*cs/numThreads_);
    }

    // perform training
    for(int iter = 1; iter <= iters_; iter++) {

        // shuffle the sentences and divide them appropriately
        if(doShuffle_) 
            shuffle(sentOrder_);

        likelihood_ = 0; accepted_ = 0;
        gettimeofday(&tStart, NULL);
        
        // initialize the models for each thread and do a sampling pass
        for(int i = 0; i < numThreads_; i++) {
            jobs[i].mod_ = this->clone();
            jobs[i].labs_ = labs.clone(jobs[i].sentOrder_,jobs[i].getNumSents());
            jobs[i].iter_ = iter;
            pthread_create( &jobs[i].thread_, NULL, samplingPass<Sent,Labs>, (void*) &jobs[i]);
            // samplingPass<Sent,Labs>(&jobs[i]);
        }

        // combine the samples into a single model
        for(int i = 0; i < numThreads_; i++) {
            pthread_join(jobs[i].thread_, NULL);
            for(int j = 0; j < jobs[i].getNumSents(); j++) {
                int s = jobs[i].sentOrder_[j];
                removeSentence(s,corp[s],labs[s]);
                labs[s] = (*jobs[i].labs_)[s];
                likelihood_ += addSentence(s,corp[s],labs[s]); 
            }
            accepted_ += jobs[i].accepted_;
            delete jobs[i].labs_;
            delete jobs[i].mod_;
        }
        
        gettimeofday(&tEnd, NULL);
        iterTime_ = timeDifference(tStart,tEnd);
        
        // print information about the iteration
        printIterationResult(iter,corp,labs);

        // sample the parameters
        if(sampParam_)
            sampleParameters();
        
    }

}

// perform a single sample over a block
template <class Sent, class Labs>
void* blockSample(void* ptr) {
    BlockJob<Sent,Labs> & myJob = *(BlockJob<Sent,Labs>*)ptr;
    int i,s;
    pair<double,double> dub;
    for(i = 0; i < myJob.getNumSents(); i++) {
        s = myJob.sentOrder_[i];
        dub = myJob.mod_->sampleSentence(s,(*myJob.corp_)[s],(*myJob.labs_)[s],(*myJob.labs_)[s]);
        myJob.propProbs_.first += dub.first; myJob.propProbs_.second += dub.second;
    }
    return NULL;
}

// train in blocks
template <class Sent, class Labs>
void ModelBase<Sent,Labs>::trainInBlocks(CorpusBase<Sent> & corp, LabelsBase<Sent,Labs> & labs) {
    
    // initialize the model
    initialize(corp,labs,true);

    // training variables
    int cs = corp.size();
    printStatus_ = true;
    vector<Labs> oldLabs(blockSize_);    // old labels to save
    vector< BlockJob<Sent,Labs> > jobs(numThreads_,BlockJob<Sent,Labs>(this,&corp,&labs));  // jobs
    timeval tStart, tEnd;

    // perform training
    for(int iter = 1; iter <= iters_; iter++) {

        // shuffle the sentences and divide them appropriately
        if(doShuffle_) 
            shuffle(sentOrder_);

        // here
        likelihood_ = 0; accepted_ = 0;
        int lastSent = 0;
        double accept = 0.0;
        
        gettimeofday(&tStart, NULL);
        
        // for each block
        for(int i = 0; i < cs; i += blockSize_) {

            // remove all of the sentences from the distribution
            pair<double,double> trueProbs = pair<double,double>(0.0,0.0),
                                propProbs = pair<double,double>(0.0,0.0);
            int myBlock = min(cs-i,blockSize_);
            for(int j = 0; j < myBlock; j++) {
                int s = sentOrder_[i+j];
                oldLabs[j] = labs[s];
                trueProbs.first += removeSentence(s,corp[s],labs[s]);
            }

            // divide the parts of the array to be handled by each thread and sample in parallel
            cacheProbabilities();
            for(int j = 0; j < numThreads_; j++) {
                jobs[j].sentOrder_ = (&sentOrder_[i])+j*myBlock/numThreads_;
                jobs[j].setNumSents( (j+1)*myBlock/numThreads_ - j*myBlock/numThreads_ );
                jobs[j].propProbs_ = pair<double,double>(0.0,0.0);
                jobs[j].iter_ = iter;
                pthread_create( &jobs[j].thread_, NULL, blockSample<Sent,Labs>, (void*) &jobs[j]);
                // blockSample<Sent,Labs>(&jobs[j]);
            }

            // wait for the threads to complete, and add statistics from the jobs
            for(int j = 0; j < numThreads_; j++) {
                pthread_join(jobs[j].thread_, NULL);
                propProbs.first += jobs[j].propProbs_.first;
                propProbs.second += jobs[j].propProbs_.second;
            }

            // add the new samples to the corpus, and calculate the probability
            for(int j = 0; j < myBlock; j++) {
                int s = sentOrder_[i+j];
                trueProbs.second += addSentence(s,corp[s],labs[s]);
            }

            // perform the acceptance/rejection step
            accept = trueProbs.second-trueProbs.first+propProbs.first-propProbs.second;
            // cout << i << " (tn=" <<trueProbs.second<<")-(tc="<<trueProbs.first<<")+(pc="<<propProbs.first<<")-(pn="<<propProbs.second<<")" << accept<<endl;

            if(iter <= skipIters_ || accept >= 0 || bernoulliSample(exp(accept))) {
                // cout << ": accepted"<<endl;
                accepted_ += myBlock;
                for(int j = 0; j < myBlock; j++)
                    sentAccepted_[sentOrder_[i+j]]++;
                likelihood_ += trueProbs.second;
            } else {
                // cout << ": REJECTED!"<<endl;
                for(int j = 0; j < myBlock; j++) {
                    int s = sentOrder_[i+j];
                    removeSentence(s,corp[s],labs[s]);
                    labs[s] = oldLabs[j];
                    addSentence(s,corp[s],labs[s]);
                }
                likelihood_ += trueProbs.first;
            }

#ifdef DEBUG_ON
            if(likelihood_ != likelihood_ || accepted_ != accepted_)
                THROW_ERROR("NaN found in training l="<<likelihood_<<", a="<<accepted_);      
#endif       

            if((i+1) / 1000 != lastSent) {
                cout << "\r" << i;
                cout.flush();
                lastSent = (i+1)/1000;
            }

        }
        gettimeofday(&tEnd, NULL);
        iterTime_ = timeDifference(tStart,tEnd);
 
        // print information about the iteration
        printIterationResult(iter,corp,labs);

        // sample the parameters
        if(sampParam_)
            sampleParameters();
        
    }
}

// make the functions for the HMM model
template class ModelBase<WordSent,ClassSent>;
template class ModelBase<WordSent,Bounds>;
