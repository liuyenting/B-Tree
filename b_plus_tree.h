#ifndef __B_PLUS_TREE_H
#define __B_PLUS_TREE_H

#include <algorithm>
#include <functional>
#include <istream>
#include <ostream>
#include <memory>
#include <cstddef>
#include <assert.h>

// Acquire the maximum of a and b
#define BPTREE_MAX(a,b) ((a) < (b) ? (b) : (a))

/** Generates default traits for a B+ tree used as a map. 
 * It estimates leaf and inner node sizes by assuming a cache line of 256 bytes. */
template<typename TKey, typename TData>
struct bptree_default_map_traits
{
    // Number of slots in each leaf of the tree.
    // Estimates so that each node has a size of 256 bytes
    static const int leafSlots = BPTREE_MAX(8, 256 / (sizeof(TKey) + sizeof(TData)));

    // Number of slots in each inner node of the tree.
    // Estimated so that each node has a size of about 256 bytes.
    static const int innerSlots = BPTREE_MAX(8, 256 / (sizeof(TKey) + sizeof(TData)));

    // Threshold for linear search if the node size is samller, otherwise, binary search
    static const size_t binary_search_threshold = 256;
};

template<typename TKey, typename TData,
         typename TValue = std::pair<TKey, TData>,
         typename TCompare = std::less<TKey>,
         typename TTraits = bptree_default_map_traits<TKey, TData>,
         bool allowDuplicates = false,
         typename Allocator = std::allocator<TValue>,
         bool usedAsSet = false>
class BpTree
{
public:
    /*
     * Template parameter types
     */

    // Key type of the B+ tree, stored in inner nodes and leaves.
    typedef TKey key_type;
    
    // Data type associated with each key. Stored in the B+ tree's leaves
    typedef TData data_type;

    // Composition of key and data types. Required by STL standard.
    typedef TValue value_type;

    // Key comparison function object
    typedef TCompare key_compare;

    // Traits object used to define more paramters of the B+ tree.
    typedef TTraits traits;

    // Allow duplicate keys in the B+ tree.
    static const bool allow_duplicates = allowDuplicates;

    // STL allocator for tree nodes;
    typedef Allocator allocator_type;

    // Boolean indicator whether the B+ tree is as a set.
    static const bool used_as_set = usedAsSet;

public:
    /* 
     * Constructed types
     */

    // typedef own type
    typedef BpTree<key_type, data_type, value_type,
                    key_compare, traits, allow_duplicates, allocator_type, used_as_set> bptree_self;
    
    // Size type used to count keys
    typedef size_t size_type;

    // The pair of key_type and data_type, may differ from value_type
    typedef std::pair<key_type, data_type> pair_type;

public:
    /*
     * Static constant options and values of the B+ tree
     */
    
    // Base B+ tree parameter
    // Number of key/data slots in each leaf
    static const unsigned short leafSlotMax = traits::leafSlots;
    // Number of key slots in each inner noe
    static const unsigned short innerSlotMax = traits::innerSlots;

    // Computed B+ tree parameter
    // Minimum number of key/data slots used in a leaf. Leaf will be merged or shifted if lower.
    static const unsigned short minLeafSlots = (leafSlotMax / 2);
    // Minimum number of key slots used in an inner node. Inner node merged or shifted if lower.
    static const unsigned short minInnerSlots = (innerSlotMax / 2);

private:
    /*
     * Node classes for in-memory nodes
     */
    
    // Header structure of each node in-memory.
    // Structure is extended by inner_node or leaf_node
    struct node
    {
        // Level in the B+ tree, it's a leaf node if level equals to 0
        unsigned short level;

        // Number of key slot used, equlas to number of valid children
        unsigned short slotUsed;

        // Delayed initialization of constructed node
        inline void initialize(const unsigned short lvl)
        {
            level = lvl;
            slotUsed = 0;
        }

        // Return true if this is a leaf node
        inline bool isLeafNode() const
        {
            return (level == 0);
        }
    };

    // Extended structure of an inner node in-memory.
    // Contains only keys and no data items.
    struct inner_node : public node
    {
        // Define an related allocator for the inner_node structs
        typedef typename Allocator::template rebind<inner_node>::other related_allocator_type;

        // Keys of children
        key_type slotKeys[innerSlotMax];

        // Pointers to children
        node* childID[innerSlotMax + 1];

        // Set variables to initial values
        inline void initialize(const unsigned short lvl)
        {
            node::initialize(lvl);
        }

        // Return true if the node's slots are full
        inline bool isFull() const
        {
            return (node::slotUsed == innerSlotMax);
        }

        // Return true if less than hlaf full
        inline bool isFew() const
        {
            return (node::slotUsed <= minInnerSlots);
        }

        // Return true if the node has too few entries
        inline bool isUnderflow() const
        {
            return (node::slotUsed < minInnerSlots);
        }
    };

    // Extended structure of a leaf node in memory.
    // Contains pairs of keys and data items.
    // Key and data slots are kept in separate arrays, because keys are traversed more often than data.
    struct leaf_node : public node
    {
        // Define an related allocator for the leaf_node
        typedef typename Allocator::template rebind<leaf_node>::other related_allocator_type;

        // Double linked list pointers to traverse the leaves
        leaf_node *previousLeaf;
        leaf_node *nextLeaf;

        // Keys of children or data pointers
        key_type slotKeys[leafSlotMax];

        // Array of data
        data_type slotData[leafSlotMax];

        // Set variables to initial values
        inline void initialize()
        {
            node::initialize(0);
            previousLeaf = nextLeaf = NULL;
        }
        
        // Return true if the node's slots are full
        inline bool isFull() const
        {
            return (node::slotUsed == leafSlotMax);
        }

        // Return true if the node is less than half full
        inline bool isFew() const
        {
            return (node::slotUsed <= minLeafSlots);
        }

        // Return true if the node as too few entries
        inline bool isUnderflow() const
        {
            return (node::slotUsed < minLeafSlots);
        }

        // Set the (key,data) pair in slot.
        // Overloaded function used by bulk_load()
        inline void set_slot(unsigned short slot, const pair_type& value)
        {
            slotKeys[slot] = value.first;
            slotData[slot] = value.second;
        }

        // Set the key pair in slot.
        // Overloaded function used by build_load()
        inline void set_slot(unsigned short slot, const key_type& key)
        {
            slotKeys[slot] = key;
        }
    };

private:
    /* 
     * Template magic to convert a pair or key/data types to a value_type
     */
    
    // set: second pair_type is an empty struct, so the value_type should only be the first.
    template<typename value_type, typename pair_type>
    struct bptree_pair_to_value
    {
        // Convert a fake pair type to just the first component
        inline value_type operator()(pair_type& p) const
        {
            return p.first;
        }

        inline value_type operator()(const pair_type& p) const
        {
            return p.first;
        }
    };

    // map: value_type is the same as the pair_type
    template<typename value_type>
    struct bptree_pair_to_value<value_type, value_type>
    {
        inline value_type operator()(pair_type& p) const
        {
            return p;
        }

        inline value_type operator()(const pair_type& p) const
        {
            return p;
        }
    };

    // Using template specialization select the correct converter used by the iterators.
    typedef bptree_pair_to_value<value_type, pair_type> pair_to_value_type;

public:
    /*
     * Iterators and reverse iterators
     */

    class iterator;
    class const_iterator;
    class reverse_iterator;
    class const_reverse_iterator;

    // STL-like iterator object for B+ tree items.
    // The iterator points to a specific slot number in a leaf.
    class iterator
    {
    public:
        /*
         * Types
         */

        // The key type of the B+ tree. Returned by key()
        typedef typename BpTree::key_type key_type;

        // Data type of the B+ tree. Returned by data()
        typedef typename BpTree::data_type data_type;

        // Value type of the B+ tree. Returned by operator*()
        typedef typename BpTree::value_type value_type;

        // Pair type of the B+ tree.
        typedef typename BpTree::pair_type pair_type;

        // Reference to the value_type. Required by STL
        typedef value_type& reference;

        // Pointer to the value_type. Required by STL
        typedef value_type* pointer;

        // STL iterator category
        typedef std::bidirectional_iterator_tag iterator_category;

        // STL-magic
        typedef ptrdiff_t difference_type;

        // Custom type
        typedef iterator self;

    private:
        /* 
         * Membmers
         */

        // The currently reference leaf node of the tree
        typename BpTree::leaf_node* currentNode;

        // Current key/data slot referenced
        unsigned short currentSlot;

        // Friendly to the iterator classes, so they mas directly access the items
        friend class const_iterator;
        friend class reverse_iterator;
        friend class const_reverse_iterator;
    
        // Friendly to base class
        // Since erase_iter() needs to read the currentNode and currentSlot values directly.
        friend class BpTree<key_type, data_type, value_type,
                            key_compare, traits, allow_duplicates, allocator_type, used_as_set>;

        // A temporary value_type to STL-correctly deliver operator* and operator->
        mutable value_type temp_value;

    public:
        /*
         * Methods
         */

        // Default constructor of a mutable iterator
        inline iterator() : currentNode(NULL), currentSlot(0)
        {
        }

        // Initializing constructor of a mutable iterator
        inline iterator(typename BpTree::leaf_node *leaf, unsigned short slot) : currentNode(leaf), currentSlot(slot)
        {
        }

        // Copy constructor from a reverse iterator
        inline iterator(const reverse_iterator &rev_itr) : currentNode(rev_itr.currentNode), currentSlot(rev_itr.currentSlot)
        {
        }

        // Dereference the iterator, not a value_type& because key and avlues are not stored together
        inline reference operator*() const
        {
            temp_value = pair_to_value_type()(pair_type(key(), data()));
            return temp_value;
        }

        // Dereference the iterator. 
        // Don't use this if possible, use key() and data() instead.
        inline pointer operator->() const
        {
            temp_value = pair_to_value_type()(pair_type(key(), data()));
            return &temp_value;
        }

        // Key of the current slot
        inline const key_type& key() const
        {
            return currentNode->slotKeys[currentSlot];
        }

        // Writable reference to the current data object
        inline data_type& data() const
        {
            return currentNode->slotData[currentSlot];
        }

        // Prefix ++ advance the iterator to the next slot
        inline self& operator++()
        {
            if((currentSlot + 1) < currentNode->slotUsed)
            {
                ++currentSlot;
            }
            else if(currentNode->nextLeaf != NULL)
            {
                currentNode = currentNode->nextLeaf;
                currentSlot = 0;
            }
            else
            {
                currentSlot = currentNode->slotUsed;
            }

            return *this;
        }

        // Postfix++ advance the iterator to the next slot
        inline self operator++(int)
        {
            // Copy the original content
            self tmp = *this;

            if((currentSlot + 1) < currentNode->slotUsed)
            {
                ++currentSlot;
            }
            else if(currentNode->nextLeaf != NULL)
            {
                currentNode = currentNode->nextLeaf;
                currentSlot = 0;
            }
            else
            {
                currentSlot = currentNode->slotUsed;
            }

            return tmp;
        }

        // Prefix -- backstep the iterator to the last slot
        inline self& operator--()
        {
            if(currentNode > 0)
            {
                --currentSlot;
            }
            else if(currentNode->previousLeaf != NULL)
            {
                currentNode = currentNode->previousLeaf;
                currentSlot = currentNode->slotUsed - 1;
            }
            else
            {
                currentSlot = 0;
            }

            return *this;
        }

        // Postfix -- backstep the iterator to the last slot
        inline self operator--(int)
        {
            // Backup current content
            self tmp = *this;

            if(currentSlot > 0)
            {
                --currentSlot;
            }
            else if(currentNode->previousLeaf != NULL)
            {
                currentNode = currentNode->previousLeaf;
                currentSlot = currentNode->slotUsed - 1;
            }
            else
            {
                currentSlot = 0;
            }

            return tmp;
        }

        // Equality of iterators
        inline bool operator==(const self& x) const
        {
            return (x.currentNode == currentNode) && (x.currentSlot == currentSlot);
        }

        // Inequality of iterators
        inline bool operator!=(const self& x) const
        {
            return (x.currentNode != currentNode) || (x.currentSlot != currentSlot);
        }
    };
};

#endif
