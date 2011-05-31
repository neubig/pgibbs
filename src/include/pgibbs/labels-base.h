#ifndef LABELS_BASE_H__
#define LABELS_BASE_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tr1/unordered_map>

#include "pgibbs/corpus-base.h"
#include "pgibbs/config-base.h"

using namespace std;

namespace pgibbs {

template <class Sent, class Labs>
class LabelsBase : public vector<Labs> {

public:

    LabelsBase() { }

    virtual ~LabelsBase() { }

    virtual LabelsBase * clone(int* sents, int len) const = 0;

    virtual void initRandom(const CorpusBase<Sent> & corp, const ConfigBase & conf) = 0;

    virtual void print(const CorpusBase<Sent> & corp, ostream & out) = 0;

};

}

#endif
