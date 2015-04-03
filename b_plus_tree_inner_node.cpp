#include "b_plus_tree_node.h"
#include "b_plus_tree_inner_node.h"

template<class TKey>
BpTreeInnerNode<TKey>::BpTreeInnerNode()
{
}

template<class TKey>
BpTreeNode<TKey>& BpTreeInnerNode<TKey>::getChild(const int index)
{
    return children[index];
}

template<class TKey>
void BpTreeInnerNode<TKey>::setChild(const int index, BpTreeNode<TKey>& child)
{
    children[index] = child;
    if(child != NULL)
        child.setParent(this);
}

template<class TKey>
enum class TreeNodeType BpTreeInnerNode<TKey>::getNodeType()
{
    return InnerNode;
}

