#ifndef __B_PLUS_TREE_H
#define __B_PLUS_TREE_H

template<class TKey, class TValue>
class BpTree
{
public:
    BpTree();

    void insert(const TKey&, const TValue&);
    TValue& search(const TKey&);
    void remove(const TKey&);
};

#endif
