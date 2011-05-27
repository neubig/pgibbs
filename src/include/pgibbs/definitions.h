#ifndef DEFINITIONS_H__
#define DEFINITIONS_H__

#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cmath>

// define this to perform extra debugging checks
// #define DEBUG_ON

#define NEG_INFINITY -1e99

#define THROW_ERROR(msg) do {                   \
    std::ostringstream oss;                     \
    oss << msg;                                 \
    throw std::runtime_error(oss.str()); }       \
  while (0);


inline double addLogProbs(const std::vector<double> & probs) {
    const unsigned size = probs.size();
    double myMax = std::max(probs[0],probs[size-1]), norm=0;
    for(unsigned i = 0; i < probs.size(); i++)
        norm += exp(probs[i]-myMax);
    return log(norm)+myMax;
}

inline void normalizeLogProbs(double* vec, unsigned size, double anneal = 1) {
    double myMax = NEG_INFINITY, norm = 0;
    for(unsigned i = 0; i < size; i++) {
        vec[i] *= anneal;
        myMax = std::max(myMax,vec[i]);
    }
    for(unsigned i = 0; i < size; i++) {
        vec[i] = exp(vec[i]-myMax);
        norm += vec[i];
    }
    for(unsigned i = 0; i < size; i++)
        vec[i] /= norm;
}

inline void normalizeLogProbs(std::vector<double> & vec, double anneal = 1) {
    normalizeLogProbs(&vec[0],vec.size(),anneal);
}

// sample a single probability
inline int sampleProbs(double* vec, int size) {
    double left = (double)rand()/RAND_MAX;
    int ret = 0;
    while(ret+1 < size && (left -= vec[ret]) > 0)
        ret++;
    return ret;
}

inline int sampleProbs(std::vector<double> vec) {
    return sampleProbs(&vec[0],vec.size());
}

// sample a single set of log probabilities
//  return the sampled ID and the log probability of the sample
inline int sampleLogProbs(double* vec, unsigned size, double anneal = 1) {
    normalizeLogProbs(vec,size,anneal);
    return sampleProbs(vec,size);
}

inline int sampleLogProbs(std::vector<double> vec, double anneal = 1) {
    return sampleLogProbs(&vec[0],vec.size(),anneal);
}

#endif
