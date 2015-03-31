#ifndef __B_PLUS_TREE_INNER_NODE_H
#define __B_PLUS_TREE_INNER_NODE_H

#include "b_plus_tree_node.h"

template<class TKey>
class BpTreeInnerNode
{
public:
    BpTreeInnerNode();

    BpTreeNode<TKey>& getChild(const int);
    void setChild(int, const BpTreeNode<TKey>);

    enum class TreeNodeType getNodeType();

    int search(const TKey&);
    void insertAt(const int&, const TKey&, const BpTreeNode<TKey>&, const BpTreeNode<TKey>&);
    void removeAt(const int&);
};

#endif
