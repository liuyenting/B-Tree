#ifndef __B_PLUS_TREE_NODE_H
#define __B_PLUS_TREE_NODE_H

#include <vector>

template<class TKey>
class BpTreeNode
{
    enum class TreeNodeType
    {
        InnerNode,
        LeafNode
    };

private:
    int keyCount;
    std::vector<TKey> keys;
    
    BpTreeNode<TKey> parentNode;
    BpTreeNode<TKey> leftSibling;
    BpTreeNode<TKey> rightSibling;

public:
    BpTreeNode();
    ~BpTreeNode();

    int getKeyCount();
    bool isOverflow();
    bool isUnderflow();
    bool canLendAKey();
    
    TKey getKey(int);
    void setKey(int, const TKey&);
    
    BpTreeNode<TKey>& getParent();
    void setParent(const BpTreeNode<TKey>&);

    BpTreeNode<TKey>& getLeftSibling();
    void setLeftSibling(const BpTreeNode<TKey>&);

    BpTreeNode<TKey>& getRightSibling();
    void setRightSibling(const BpTreeNode<TKey>&);

    BpTreeNode<TKey>& dealOverflow();
    BpTreeNode<TKey>& dealUnderflow();

    virtual TreeNodeType getNodeType();
    virtual BpTreeNode<TKey>& split();
    virtual BpTreeNode<TKey> pushUpKey(const TKey&, const BpTreeNode<TKey>&, const BpTreeNode<TKey>&);
    /**
     * Search a key on current node, and return its position if found,
     * otherwise, return -1 for a leaf node.
     * Return the child node index which should contain the key for a 
     * internal node.
     */
    virtual int search(const TKey&);
};

#endif
