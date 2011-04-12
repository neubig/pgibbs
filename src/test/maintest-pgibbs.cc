
#include <iostream>
#include "model-hmm.h"

using namespace std;
using namespace pgibbs;

int testHMMSample() {
    
    // make a corpus with one one-word sentence
    WordSent ws(3); ws[0]=0; ws[1]=1; ws[2]=0;
    WordCorpus corp; corp.push_back(ws);
 
    // make a corpus with one one-word sentence
    ClassSent cs(3); cs[0]=2; cs[1]=0; cs[2]=2;
    HMMLabels labs; labs.push_back(cs);
    
    // make the value
    HMMConfig conf;
    conf.setDouble("tstr", 1.0); conf.setDouble("estr", 1.0);
    conf.setInt("classes",2); conf.addConfigEntry("words","2","");
    conf.setBool("skipmh",true);
    HMMModel hmmMod(conf);
    
    // do samples
    int numSamps = 10000;
    vector<int> counts(2,0);
    ClassSent samp = cs;
    hmmMod.cacheProbabilities();
    for(int i = 0; i < numSamps; i++) {
        hmmMod.sampleSentence(1,ws,samp,samp);
        counts[samp[1]]++;
    }
    cout << "Probability of 1 before: "<<100.0*counts[0]/numSamps<<endl;

    // add one sentence to the model
    hmmMod.addSentence(0,ws,cs);
    counts[0]=0; counts[1]=0;
    hmmMod.cacheProbabilities();
    for(int i = 0; i < numSamps; i++) {
        hmmMod.sampleSentence(1,ws,samp,samp);
        counts[samp[1]]++;
    }
    cout << "Probability of 1 after: "<<100.0*counts[0]/numSamps<<endl;

    return 1;

}

int main(int argc, char **argv) {
    cout << "Hello world" << endl;

    int correct = 0, total = 0;

    correct += testHMMSample(); total++;

    cout << correct << "/" << total << " correct"<<endl;

}
