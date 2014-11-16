#ifndef LABELS_HMM_H__
#define LABELS_HMM_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

#include "pgibbs/labels-base.h"
#include "pgibbs/corpus-word.h"
#include "gng/samp-gen.h"

using namespace std;

namespace pgibbs {

// hold the classes as an integer vector
typedef GenericString<int> ClassSent;

class HMMLabels : public LabelsBase<WordSent,ClassSent> {

public:

    HMMLabels() { }

    ~HMMLabels() { }

    // 0->classes-1 are normal tags, classes is the final tag
    void initRandom(const CorpusBase<WordSent> & corp, const ConfigBase & conf) {
        int classes = conf.getInt("classes"), i, j, cs = corp.size();
        for(i = 0; i < cs; i++) {
            const WordSent & ws = corp[i];
            ClassSent cl(ws.length());
            cl[0] = classes; cl[cl.length()-1] = classes;
            for(j = 1; j < (int)ws.length()-1; j++)
                cl[j] = discreteUniformSample(classes);
            push_back(cl);
        }
    }

    void print(const CorpusBase<WordSent> & corp, ostream & out) {
        int cs = size(), ss, i, j;
        for(i = 0; i < cs; i++) {
            out << (*this)[i][1];
            ss = (*this)[i].length()-1;
            for(j = 2; j < ss; j++) 
                out << " " << (*this)[i][j];
            out << endl;
        }
    }

    HMMLabels * clone(int* sents, int len) const {
        HMMLabels * ret = new HMMLabels();
        ret->resize(this->size());
        for(int i = 0; i < len; i++)
            (*ret)[sents[i]] = (*this)[sents[i]];
        return ret;
    }

};

}

#endif
