#ifndef GNG_MISC_FUNC_H__
#define GNG_MISC_FUNC_H__

#include <sys/time.h>

template <class T>
void shuffle(vector<T> & vec) {
    int vecSize = vec.size(), pos;
    T temp;
    for(int i = vecSize - 1; i > 0; i--) {
        pos = discreteUniformSample(i+1);
        temp = vec[pos];
        vec[pos] = vec[i];
        vec[i] = temp;
    }
}

double timeDifference(const timeval & s, const timeval & e) {
    return (e.tv_sec-s.tv_sec)+(e.tv_usec-s.tv_usec)/1000000.0;
}

#endif
