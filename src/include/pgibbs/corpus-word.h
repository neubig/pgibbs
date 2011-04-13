#ifndef CORPUS_WORD_H__
#define CORPUS_WORD_H__

#include "gng/symbol-map.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "definitions.h"
#include "corpus-base.h"

// TODO: fix
using namespace std;

typedef vector<int> WordSent;

class WordCorpus : public CorpusBase< WordSent > {

public:

    WordCorpus() { }

    int getVocabSize() { return ids_.size(); }

    // load the corpus, and pad on either side with words if necessary
    void load(istream & in, bool padSent = false, int padId = -1) {
        string line,str;
        while(getline(in,line)) {
            istringstream iss(line);
            vector<int> vals;
            if(padSent) vals.push_back(padId);
            while(iss >> str)
                vals.push_back(ids_.getId(str,true));
            if(padSent) vals.push_back(padId);
            push_back(vals);
        }
    }

    void load(const string & fileName, bool padSent = false, int padId = -1) {
        ifstream in(fileName.c_str());
        if(!in)
            THROW_ERROR("could not find input file " << fileName);
        load(in,padSent,padId);
    }

};

#endif
