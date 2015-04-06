#include "btree_multimap.h"

// Traits used for the speed tests, BTREE_DEBUG is not defined.
template <int _innerslots, int _leafslots>
struct btree_traits_speed : stx::btree_default_set_traits<unsigned int>
{
    static const bool   selfverify = false;
    static const bool   debug = false;

    static const int    leafslots = _innerslots;
    static const int    innerslots = _leafslots;

    static const size_t binsearch_threshold = 256*1024*1024; // never
};
