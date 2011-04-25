#ifndef GNG_SYMBOL_SET_H__
#define GNG_SYMBOL_SET_H__

#include "string.h"
#include <tr1/unordered_map>
#include <vector>
#include <stdexcept>

namespace gng {

template < class Key, class T, class Hash = std::tr1::hash<Key> >
class SymbolSet {

public:

    typedef std::tr1::unordered_map< Key, T, Hash > Map;
    typedef std::vector< Key* > Vocab;
    typedef std::vector< T > Ids;

protected:
    
    Map map_;
    Vocab vocab_;
    Ids reuse_;

public:
    SymbolSet() : map_(), vocab_(), reuse_() { }

    const Key & getSymbol(T id) const {
        // if(id >= (int)vocab_.size() || id < 0 || vocab_[id] == 0) {
        //     std::cerr << "id == "<<id << ", vocabsize="<<vocab_.size()<<", reusesize="<<reuse_.size()<<", vocab_["<<id<<"] == "<<(int)vocab_[id]<<std::endl;
        //     throw std::runtime_error("Size overflow in getSymbol");
        // }            
        return *vocab_[id];
    }
    T getId(const Key & sym, bool add = false) {
        typename Map::const_iterator it = map_.find(sym);
        if(it != map_.end())
            return it->second;
        else if(add) {
            T id;
            if(reuse_.size() != 0) {
                id = reuse_.back(); reuse_.pop_back();
                vocab_[id] = new Key(sym);
            } else {
                id = vocab_.size();
                vocab_.push_back(new Key(sym));
            }
            map_.insert(std::pair<Key,T>(sym,id));
            return id;
        }
        return -1;
    }
    T getId(const Key & sym) const {
        return const_cast< SymbolSet<Key,T,Hash>* >(this)->getId(sym,false);
    }
    size_t size() const { return vocab_.size() - reuse_.size(); }
    void removeId(const T id) {
        map_.erase(*vocab_[id]);
        delete vocab_[id];
        vocab_[id] = 0;
        reuse_.push_back(id);
    }

    Map & getMap() { return map_; }

};

}

#endif
