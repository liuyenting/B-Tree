#include "b_plus_tree_node.h"

#include <cstddef>

template<class TKey>
BpTreeNode<TKey>::BpTreeNode()
{
    keyCount = 0;
    parentNode = leftSibling = rightSibling = NULL;
}

template<class TKey>
int BpTreeNode<TKey>::getKeyCount()
{
    return keyCount;
}

template<class TKey>
TKey BpTreeNode<TKey>::getKey(const int index)
{
    return keys[index];
}

template<class TKey>
void BpTreeNode<TKey>::setKey(const int index, const TKey& key)
{
    keys[index] = key;
}

template<class TKey>
BpTreeNode<TKey>& BpTreeNode<TKey>::getParent()
{
    return parentNode;
}

template<class TKey>
void BpTreeNode<TKey>::setParent(const BpTreeNode<TKey>& parent)
{
    parentNode = parent;
}

template<class TKey>
bool BpTreeNode<TKey>::isOverflow()
{
    return getKeyCount() == keys.size();
}

