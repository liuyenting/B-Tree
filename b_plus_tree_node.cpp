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

/* Codes below are used to support insertion operation */
template<class TKey>
bool BpTreeNode<TKey>::isOverflow()
{
    return getKeyCount() == keys.size();
}

template<class TKey>
BpTreeNode<TKey>& BpTreeNode<TKey>::dealOverflow()
{
    int midIndex = getKeyCount() / 2;
    TKey upKey = getKey(midIndex);

    BpTreeNode<TKey> newNode = split();

    if(getParent() == NULL)
        setParent(new BpTreeInnerNode<TKey>());
    newNode.setParent(getParent());

    // maintain links of sibling nodes
    newNode.setLeftSibling(*this);
    newNode.setRightSibling(rightSibling);
    if(getRightSibling() != NULL)
        getRightSibling().setLeftSibling(newNode);
    setRightSibling(newNode);

    // push up a key to parent internal node
    return getParent().pushUpKey(upKey, *this, newNode);
}

template<class TKey>
bool BpTreeNode<TKey>::isUnderflow()
{
    return getKeyCount() < (keys.size() / 2);
}

template<class TKey>
bool BpTreeNode<TKey>::canLendAKey()
{
    return getKeyCount() > (keys.size() / 2);
}

template<class TKey>
BpTreeNode<TKey>& BpTreeNode<TKey>::getLeftSibling()
{
    if((leftSibling != NULL) && (leftSibling.getParent() == getParent()))
        return leftSibling;
    return NULL;
}

template<class TKey>
void BpTreeNode<TKey>::setLeftSibling(const BpTreeNode<TKey>& sibling)
{
    leftSibling = sibling;
}

template<class TKey>
BpTreeNode<TKey>& BpTreeNode<TKey>::getRightSibling()
{
    if((rightSibling != NULL) && (rightSibling.getParent() == getParent()))
        return rightSibling;
    return NULL;
}

template<class TKey>
void BpTreeNode<TKey>::setRightSibling(const BpTreeNode<TKey>& sibling)
{
    rightSibling = sibling;
}

template<class TKey>
BpTreeNode<TKey>& BpTreeNode<TKey>::dealUnderflow()
{
    if(getParent() == NULL)
        return NULL;
    
    // try to borrow a key from sibling
    BpTreeNode<TKey> leftSiblingTemp = getLeftSibling();
    if((leftSiblingTemp != NULL) && (leftSiblingTemp.canLendAKey()))
    {
        getParent().processChildrenTransfer(this, leftSiblingTemp, leftSiblingTemp.getKeyCount() - 1);
        return NULL;
    }

    BpTreeNode<TKey> rightSiblingTemp = getRightSibling();
    if((rightSiblingTemp != NULL) && (rightSiblingTemp.canLendAKey()))
    {
        getParent().processChildrenTransfer(this, rightSiblingTemp, 0);
        return NULL;
    }

    // can not borrow a key from any sibling, then do fusion with wibling
    if(leftSiblingTemp != NULL)
        return getParent().processChildrenFusion(leftSiblingTemp, this);
    else
        return getParent().processChildrenFusion(this, rightSiblingTemp);
}

