#ifndef __B_PLUS_TREE_H
#define __B_PLUS_TREE_H

class Element
{
};

class BpTree
{
public:
    BpTree();
    ~BpTree();
    
    Element search();
    void update();
    void add(const Element&);
    void remove();
};

#endif
