#ifndef LABELS_ws_H__
#define LABELS_ws_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tr1/unordered_map>

#include "labels-base.h"
#include "corpus-word.h"
#include "gng/samp-gen.h"

using namespace std;

namespace pgibbs {

// hold the classes as an integer vector
typedef vector<bool> Bounds;

class WSLabels : public LabelsBase<WordSent,Bounds> {

public:

    WSLabels() { }

    ~WSLabels() { }

    // 0->classes-1 are normal tags, classes is the final tag
    void initRandom(const CorpusBase<WordSent> & corp, const ConfigBase & conf) {
        int maxLen = conf.getInt("maxlen");
        cerr << "maxLen = "<<maxLen<<endl;
        int i, j, cs = corp.size();
        for(i = 0; i < cs; i++) {
            const WordSent & chars = corp[i];
            Bounds bs(chars.length());
            int last = 0;
            for(j = 0; j < (int)chars.length(); j++) {
                bs[j] = (j==(int)chars.length()-1 || j-last==maxLen-1)?1:bernoulliSample(0.5);
                if(bs[j]) last = j;
            }
            push_back(bs);
        }
    }

    void print(const CorpusBase<WordSent> & corp, ostream & out) {
        int cs = size(), i, j;
        for(i = 0; i < cs; i++) {
            out << corp.getSymbol(corp[i][0]);
            for(j = 1; j < (int)(*this)[i].size(); j++) {
                if((*this)[i][j-1]) out << " ";
                out << corp.getSymbol(corp[i][j]);
            }
            out << endl;
        }
    }

    virtual WSLabels * clone(int* sents, int len) const {
        WSLabels * ret = new WSLabels();
        ret->resize(this->size());
        for(int i = 0; i < len; i++)
            (*ret)[sents[i]] = (*this)[sents[i]];
        return ret;
    }

};

}

#endif
