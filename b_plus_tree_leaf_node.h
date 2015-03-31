#ifndef __B_PLUS_TREE_LEAF_NODE_H
#define __B_PLUS_TREE_LEAF_NODE_H

#include "b_plus_tree_node.h"

template<class TKey, class TValue>
class BpTreeLeafNode
{
public:
    BpTreeLeafNode();

    TValue& getValue(const int);
    void setValue(const int, const TValue&);
    
    enum class TreeNodeType getNodeType();

    int search(const TKey&);
    
    void insertKey(const TKey&, const TValue&);
    void insertAt(const int, const TKey&, const TValue&);
    
    bool remove(const TKey&);
    void removeAt(const int);
};

#endif
