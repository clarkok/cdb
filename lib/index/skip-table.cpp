#include <cstdlib>
#include <cassert>

#include "lib/utils/buffer.hpp"
#include "skip-table.hpp"

using namespace cdb;

/**
 * Base class for NonLeaf & Leaf
 */
struct SkipTable::Node
{
    NonLeaf *parent;    /** parent of this Node must be NonLeaf */
    bool is_leaf;       /** if this Node is a Leaf */
    Leaf *leaf;         /** use this Leaf's value as the Node's value */
    Node *next;         /** next slibing for this Node */
    Node *prev;         /** previous slibing for this Node */

    Node(
            NonLeaf *parent,
            bool is_leaf,
            Leaf *leaf,
            Node *next,
            Node *prev
        )
        : parent(parent),
          is_leaf(is_leaf),
          leaf(leaf),
          next(next),
          prev(prev)
    { }
};

struct SkipTable::NonLeaf : public SkipTable::Node
{
    Node *child;        /** first child of this NonLeaf */

    NonLeaf(
            NonLeaf *parent,
            Leaf *leaf,
            Node *next,
            Node *prev,
            Node *child
        )
        : Node(parent, false, leaf, next, prev),
          child(child)
    { }
};

struct SkipTable::Leaf : public SkipTable::Node
{
    Buffer value;       /** value of this Leaf */

    Leaf(
            NonLeaf *parent,
            Leaf *leaf,
            Node *next,
            Node *prev,
            Buffer value
        )
        : Node(parent, true, leaf, next, prev),
          value(value)
    { }
};

SkipTable::SkipTable(Comparator less)
    : _less(less)
{ }

SkipTable::~SkipTable()
{ clear(); }

SkipTable::Iterator::Iterator(SkipTable *owner, Leaf *ptr)
    : _owner(owner), _ptr(ptr), _slice(ptr ? ptr->value : Slice(nullptr, 0))
{ }

SkipTable::Iterator
SkipTable::nextIterator(const Iterator &iter)
{ return Iterator(this, static_cast<Leaf*>(iter._ptr->next)); }

SkipTable::Iterator
SkipTable::prevIterator(const Iterator &iter)
{
    if (iter._ptr) {
        return Iterator(this, static_cast<Leaf*>(iter._ptr->prev));
    }
    else {
        // if arrived at end() already
        return Iterator(this, last());
    }
}

SkipTable::Iterator
SkipTable::begin()
{ return Iterator(this, first()); }

SkipTable::Iterator
SkipTable::end()
{ return Iterator(this, nullptr); }

SkipTable::Leaf*
SkipTable::lowerBoundInLeaf(ConstSlice value)
{
    Node *ptr = _root;
    Node *child;

    while (ptr && !ptr->is_leaf) {
        child = static_cast<NonLeaf*>(ptr)->child;
        while (ptr && _less(ptr->leaf->value, value)) {
            child = static_cast<NonLeaf*>(ptr)->child;
            ptr = ptr->next;
        }
        ptr = child;
    }

    while (ptr && _less(static_cast<Leaf*>(ptr)->value, value)) {
        ptr = ptr->next;
    }

    return static_cast<Leaf*>(ptr);
}

SkipTable::Leaf*
SkipTable::upperBoundInLeaf(ConstSlice value)
{
    Node *ptr = _root;
    Node *child;

    while (ptr && !ptr->is_leaf) {
        child = static_cast<NonLeaf*>(ptr)->child;
        while (ptr && !_less(value, ptr->leaf->value)) {
            child = static_cast<NonLeaf*>(ptr)->child;
            ptr = ptr->next;
        }
        ptr = child;
    }

    while (ptr && !_less(value, static_cast<Leaf*>(ptr)->value)) {
        ptr = ptr->next;
    }

    return static_cast<Leaf*>(ptr);
}

SkipTable::Iterator
SkipTable::lowerBound(ConstSlice value)
{ return Iterator(this, lowerBoundInLeaf(value)); }

SkipTable::Iterator
SkipTable::upperBound(ConstSlice value)
{ return Iterator(this, upperBoundInLeaf(value)); }

SkipTable::Iterator
SkipTable::insert(ConstSlice value)
{
    // Empty table
    if (!_root) {
        _root = newLeaf(nullptr, nullptr, nullptr, value);
        return Iterator(this, static_cast<Leaf*>(_root));
    }

    Node *insert_point = upperBoundInLeaf(value);
    insert_point = insert_point ? insert_point->prev : last();

    // insert before the first
    if (!insert_point) {
        Leaf *original_first = first();
        Leaf *new_leaf = newLeaf(
                original_first->parent,
                original_first,
                nullptr,
                value
            );
        original_first->prev = new_leaf;

        int level_flag = std::rand();
        Node *ptr = new_leaf;
        Node *original_ptr = original_first;

        while (original_ptr != _root && (level_flag & 1)) {
            level_flag >>= 1;

            auto *parent = original_ptr->parent;
            auto *new_parent = newNonLeaf(
                    parent->parent,
                    new_leaf,
                    parent,
                    nullptr,
                    ptr
                );
            parent->prev = new_parent;
            ptr->parent = new_parent;

            ptr = new_parent;
            original_ptr = parent;
        }

        if (original_ptr == _root) {
            _root = newNonLeaf(
                    nullptr,
                    new_leaf,
                    nullptr,
                    nullptr,
                    ptr
                );
            ptr->parent = original_ptr->parent = static_cast<NonLeaf*>(_root);
        }
        else {
            do {
                auto *parent = ptr->parent;
                parent->child = ptr;
                parent->leaf = ptr->leaf;
                ptr = parent;
            } while (ptr != _root);
        }
        return Iterator(this, new_leaf);
    }
    else {
        // insert in leaf
        Leaf *new_leaf = newLeaf(
                insert_point->parent,
                static_cast<Leaf*>(insert_point->next),
                static_cast<Leaf*>(insert_point),
                value
            );
        insert_point->next = new_leaf;
        if (new_leaf->next) {
            new_leaf->next->prev = new_leaf;
        }

        int level_flag = std::rand();
        Node *sep_node = new_leaf;

        // insert in each level
        while (insert_point != _root && (level_flag & 1)) {
            level_flag >>= 1;

            auto *parent = insert_point->parent;
            auto *new_parent = newNonLeaf(
                    parent->parent,
                    new_leaf,
                    parent->next,
                    parent,
                    sep_node
                );
            parent->next = new_parent;
            if (new_parent->next) {
                new_parent->next->prev = new_parent;
            }

            while (sep_node && sep_node->parent == parent) {
                sep_node->parent = new_parent;
                sep_node = sep_node->next;
            }

            sep_node = new_parent;
            insert_point = parent;
        }

        if (insert_point == _root) {
            _root = newNonLeaf(
                        nullptr,
                        static_cast<Leaf*>(insert_point)->leaf,
                        nullptr,
                        nullptr,
                        _root
                    );
            insert_point->parent = sep_node->parent = static_cast<NonLeaf*>(_root);
        }

        return Iterator(this, new_leaf);
    }
}

SkipTable::Leaf *
SkipTable::newLeaf(
        NonLeaf *parent,
        Leaf *next,
        Leaf *prev,
        ConstSlice value
    ) const
{
    auto *ret = new Leaf(
            parent,
            nullptr,    // to be set
            next,
            prev,
            Buffer(value.cbegin(), value.cend())
        );
    ret->leaf = ret;
    return ret;
}

SkipTable::NonLeaf *
SkipTable::newNonLeaf(
        NonLeaf *parent,
        Leaf *leaf,
        Node *next,
        Node *prev,
        Node *child
    ) const
{
    return new NonLeaf(
            parent,
            leaf,
            next,
            prev,
            child
        );
}

SkipTable::Leaf *
SkipTable::last() const
{
    if (!_root) {
        return nullptr;
    }

    Node *ptr = _root;

    while (true) {
        while (ptr->next) {
            ptr = ptr->next;
        }
        if (ptr->is_leaf) {
            return static_cast<Leaf*>(ptr);
        }
        ptr = static_cast<NonLeaf*>(ptr)->child;
    }
}

SkipTable::Leaf *
SkipTable::first() const
{
    return _root->is_leaf ?
        static_cast<Leaf*>(_root) :
        static_cast<NonLeaf*>(_root)->leaf;
}

void
SkipTable::clear()
{
    Node *head = _root;
    while (head) {
        auto child = head->is_leaf ? nullptr : static_cast<NonLeaf*>(head)->child;
        Node *ptr = head;
        while (ptr) {
            Node *next_ptr = ptr->next;
            delete ptr;
            ptr = next_ptr;
        }
        head = child;
    }
    _root = nullptr;
}

SkipTable::Iterator
SkipTable::erase(Iterator pos)
{
    assert(pos._owner == this);
    assert(pos._ptr);

    Iterator ret(this, static_cast<Leaf*>(pos._ptr->next));

    if (pos._ptr->prev) {
        // not the first
        Node *ptr = pos._ptr;
        NonLeaf *parent = ptr->parent;
        bool is_child = parent && parent->child == ptr;
        while (is_child) {
            if (ptr->prev) {
                ptr->prev->next = ptr->next;
            }
            if (ptr->next) {
                ptr->next->prev = ptr->prev;
            }
            delete ptr;
            NonLeaf *new_parent = static_cast<NonLeaf*>(parent->prev);
            for (auto *child = ptr->next; child && child->parent == parent; child = child->next) {
                child->parent = new_parent;
            }
            ptr = parent;
            parent = ptr->parent;
            is_child = parent && parent->child == ptr;
        }
        if (ptr->prev) {
            ptr->prev->next = ptr->next;
        }
        if (ptr->next) {
            ptr->next->prev = ptr->prev;
        }
        delete ptr;
    }
    else {
        // first node in this list
        Node *ptr = pos._ptr;
        NonLeaf *parent = ptr->parent;
        bool is_only_child = parent && (!ptr->next || ptr->parent != ptr->next->parent);
        Node *break_point = ptr->next;

        if (ptr->next) {
            ptr->next->prev = nullptr;
        }
        delete ptr;

        while (is_only_child) {
            ptr = parent;
            parent = ptr->parent;
            break_point = ptr->next;
            is_only_child = parent && (!ptr->next || ptr->parent != ptr->next->parent);

            if (ptr->next) {
                ptr->next->prev = nullptr;
            }
            delete ptr;
        }
        if (!parent) {
            _root = break_point;
        }
        else {
            break_point->prev = nullptr;
            parent->child = break_point;
            while (parent) {
                parent->leaf = break_point->leaf;
                parent = parent->parent;
            }
        }
    }

    return ret;
}

std::ostream &
SkipTable::__debug_output(std::ostream &os, std::function<void(std::ostream &, ConstSlice)> print)
{
    Node *head = _root;
    do {
        auto *ptr = head;
        while (ptr) {
            print(os, ptr->leaf->value);
            ptr = ptr->next;
        }
        os << std::endl;
        if (head->is_leaf) {
            break;
        }
        head = static_cast<NonLeaf*>(head)->child;
    } while(true);
    return os;
}
