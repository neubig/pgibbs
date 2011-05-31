
#include "pgibbs/labels-hmm.h"
#include "pgibbs/config-hmm.h"
#include "pgibbs/model-hmm.h"
#include "pgibbs/corpus-word.h"
#include <string>
#include <vector>

using namespace std;
using namespace pgibbs;

int main(int argc, char** argv) {

    // load the arguments
    HMMConfig conf;
    vector<string> args = conf.loadConfig(argc,argv);

    // load the corpus and save the number of unique words
    WordCorpus corp;
    corp.load(args[0], true);
	conf.addConfigEntry("words", "0", "");
    conf.setInt("words",corp.getVocabSize());

    // initialize the tags
    HMMLabels labs;
    labs.initRandom(corp,conf);

    // create and train the model
    HMMModel mod(conf);
    mod.train(corp,labs);

}
