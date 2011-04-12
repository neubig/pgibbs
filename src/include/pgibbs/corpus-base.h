#ifndef CORPUS_BASE_H__
#define CORPUS_BASE_H__

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "definitions.h"
#include "gng/symbol-map.h"

// TODO: fix
using namespace std;
using namespace gng;

// T is the representation of a single sentence
template < class Sent >
class CorpusBase : public vector< Sent > {

protected:
    SymbolMap<string,int> ids_; // a mapping from words to IDs

};

#endif
