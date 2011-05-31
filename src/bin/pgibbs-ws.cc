
#include "pgibbs/labels-ws.h"
#include "pgibbs/config-ws.h"
#include "pgibbs/model-ws.h"
#include "pgibbs/corpus-word.h"
#include <string>
#include <vector>

using namespace std;
using namespace pgibbs;

int main(int argc, char** argv) {

    // load the arguments
    WSConfig conf;
    vector<string> args = conf.loadConfig(argc,argv);

    // load the corpus and save the number of unique words
    WordCorpus corp;
    corp.load(args[0], false);
	conf.addConfigEntry("chars", "0", "");
    conf.setInt("chars",corp.getVocabSize());

    // initialize the tags
    WSLabels labs;
    labs.initRandom(corp,conf);

    // create and train the model
    WSModel mod(conf);
    mod.train(corp,labs);

}
