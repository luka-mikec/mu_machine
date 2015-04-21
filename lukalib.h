#ifndef LUKALIB_H
#define LUKALIB_H

#include <functional>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <stack>
#include <list>
#include <map>
#include <set>

/*using std::cout;
using std::endl;
using std::list;
using std::find;
using std::stack;
using std::string;
using std::ostream;
using std::ios_base;
using std::stringstream;*/
using namespace std;

/*  GENERAL FUNCTIONS   */

//#define full(x) x.begin(), x.end()
#define full(x) begin(x), end(x)
#define contains(x, y) (std::find(full(x), y) != x.end())
#define pointer_iterate(iter, cont) if (cont.size()) for (auto &iter : cont) if (iter)
#define pointer_iterate_blown(iter, cont) for (auto iter = cont.begin(); iter != cont.end(); ++iter) if (*iter)
#ifndef llib_ignore_byte_typedef
    typedef uint8_t byte;
#endif

template <typename T>
int sgn(T val)  /// SO's user79758, jer se luki neda
{
    return (T(0) < val) - (val < T(0));
}

template <class C>
void container_del(C &v)
{
    if (v.size()) for (auto i : v) if (i) delete i;
}

template <class C, class O>
void value_dump(C val, O& strm) { strm << val << " "; }

template <class C, class O = ostream>
void container_dump(C v,
                    O& strm = cout,
                    function<void(typename C::value_type, O&)> print_func = value_dump<typename C::value_type, O>)
{
    if (v.size()) for (auto &i : v) print_func(i, strm);
}

/*template <class C>
void container_dump2d(C v, function<void(typename C::value_type::value_type)> print_func = value_dump<typename C::value_type::value_type>)
{
    container_dump(v,
        [print_func](typename C::value_type v2) {
            container_dump(v2, print_func);
            cout << endl;
        }
    );
}*/

template <class T2, class T1>
T2 scast(T1 val)
{    stringstream _in; _in.setf(ios_base::fixed); _in << val;
     _in.setf(ios_base::fixed); T2 _out; _in >> _out; return _out;
}

/*  TREE    */

/*
 * idejise:
 *    - curious recurring pattern: neka tree ima param <T>, T : tree, to sredi sve valjda
 *    - T : tree, node_data, 2struki inherit
 *    - generic container
 *
 */


template <class derived_tree>
class tree
{
    template <class any_derived_tree>
    friend ostream& operator<<(ostream& out, tree<any_derived_tree>& what);

protected:
    list<tree<derived_tree>*> children; // *** derived_tree?

    void _init()
    {

    }

    void _clear()
    {
        container_del(children);
        children.clear();
    }

    void _reset()
    {
        _clear();
        _init();
    }

    virtual void _copy_from(tree<derived_tree>& what);

    virtual tree<derived_tree>* _deep_copy();

    void _push_left(tree<derived_tree> &what)
    {
        children.push_front(what._deep_copy());
    }

    void _push_right(tree<derived_tree> &what)
    {
        children.push_back(what._deep_copy());
    }

public:
    class iterator;

    tree<derived_tree>()
    {
        _reset();
    }

    tree<derived_tree>(tree<derived_tree>& cp)
    {
        _copy_from(cp);
    }

    tree<derived_tree>(tree<derived_tree>&& slow_cp)
    {
        _copy_from(slow_cp);
    }

    virtual ~tree<derived_tree>()
    {
        _clear();
    }

    // dodati move konstr.

    virtual void push_left(derived_tree& what)
    {
        _push_left(what);
    }

    virtual void push_left(derived_tree slow_what)
    {
        _push_left(slow_what);
    }

    virtual void push_right(derived_tree& what)
    {
        _push_right(what);
    }

    iterator left();

    iterator right();

    iterator begin();

    iterator end();

    virtual void print_node(ostream& out)
    {
        out << this << endl;
    }

    void print_tree(ostream& out, string prefix = "")
    {
        out << prefix;
        print_node(out);
        pointer_iterate(child, children) child->print_tree(out, prefix + "\t");
    }
};

template <class derived_tree>
class tree<derived_tree>::iterator
{
    list<tree<derived_tree>*>       skip_children_in_next_iter;
    tree<derived_tree>*             t;
    bool _should_skip(tree<derived_tree>* which)
    {
        return contains(skip_children_in_next_iter, which);
    }

    struct iteration_tree_frame
    {
        typedef typename list<tree<derived_tree>*>::iterator list_itr;
        tree<derived_tree>*                             actual_parent;
        list_itr                                        current_position;
        iterator&                                       ref_to_itr;

        iteration_tree_frame(tree* parent, list_itr current, iterator& itr)
            : ref_to_itr(itr), actual_parent(parent), current_position(current)
        { }

        bool operator==(const iteration_tree_frame& b) const
        {
            return actual_parent == b.actual_parent
                    && current_position == b.current_position;
        }

        list_itr next_child(list_itr current_position)
        {
            ++current_position;
            while (current_position != actual_parent->children.end())
                if (ref_to_itr._should_skip(*current_position))
                    ++current_position;
                else
                    break;
            return current_position;
        }

        bool frame_at_last()
        {
            return next_child(current_position) == actual_parent->children.end();
        }

        // skips one, and than skips while not EOT and skippable nodes
        iteration_tree_frame& operator++()
        {
            current_position = next_child(current_position);
            return *this;
        }

    };

    stack<iteration_tree_frame>     ancestry;

public:
    iterator(tree<derived_tree>* from) : t(from)      { }

    iterator& operator++()
    {
        bool made_it = false;
        if (!t->children.empty())
            pointer_iterate_blown(child, t->children)
                if (!_should_skip(*child))
                {
                    made_it = true;
                    ancestry.push(iteration_tree_frame(t, child, *this));
                    t = *child;
                    break;
                }

        if (!made_it)
        {
            while (!ancestry.empty() && ancestry.top().frame_at_last())
                ancestry.pop();

            if (ancestry.empty())
            {
                t = 0;  // done iterating, set iterator to .end()
                ancestry = stack<iteration_tree_frame>();
                skip_children_in_next_iter = list<tree<derived_tree>*>();
            }
            else
            {   // iterate on quasi-sister node (quasi-mother's next child)
                iteration_tree_frame &last = ancestry.top();
                last.current_position++;
                t = *last.current_position;
            }
        }
        return *this;
    }

    bool operator==(const iterator& b) const
    {
        return (t == b.t)
                && (ancestry == b.ancestry)
                && (skip_children_in_next_iter == b.skip_children_in_next_iter);
    }

    bool operator!=(const iterator& b) const
    {
        return !(*this == b);
    }

    derived_tree* operator->()
    {
        return dynamic_cast<derived_tree*>(t);
    }

    derived_tree&  operator*()
    {
        return dynamic_cast<derived_tree&>(*t);
    }

    void push_and_skip_left(derived_tree &child)
    {
        t->push_left(child);
        if (!contains(skip_children_in_next_iter, t->children.front()))
            skip_children_in_next_iter.push_back(t->children.front());
    }

    void push_and_skip_right(derived_tree &child)
    {
        t->push_right(child);
        if (!contains(skip_children_in_next_iter, t->children.back()))
            skip_children_in_next_iter.push_back(t->children.back());
    }

};

template <typename derived_tree> void
    tree<derived_tree>::_copy_from(tree<derived_tree>& what)
    {
        _reset();
        if (what.children.size()) for (tree<derived_tree>* &child : what.children)
            children.push_back(new derived_tree(dynamic_cast<derived_tree&>(*child)));
    }

template <typename derived_tree> tree<derived_tree>*
    tree<derived_tree>::_deep_copy()
    {
        tree<derived_tree>* n = new derived_tree;
        n->_copy_from(*this);
        return n;
    }

template <typename derived_tree> typename tree<derived_tree>::iterator
    tree<derived_tree>::left()  { return children.front()->begin(); }

template <typename derived_tree> typename tree<derived_tree>::iterator
    tree<derived_tree>::right() { return children.back()->begin(); }

template <typename derived_tree> typename tree<derived_tree>::iterator
    tree<derived_tree>::begin() { return iterator(this); }

template <typename derived_tree> typename tree<derived_tree>::iterator
    tree<derived_tree>::end()   { return iterator(0); }

template<class derived_tree>
    ostream& operator<<(ostream& out, tree<derived_tree>& what)
    {
        what.print_tree(out);
        return out;
    }

#endif // LUKALIB_H
