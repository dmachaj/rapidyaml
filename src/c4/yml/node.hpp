#ifndef _C4_YML_NODE_HPP_
#define _C4_YML_NODE_HPP_

/** @file node.hpp
 * @see NodeRef */

#include <cstddef>

#include "c4/yml/tree.hpp"
#include "c4/base64.hpp"

#ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4251/*needs to have dll-interface to be used by clients of struct*/)
#   pragma warning(disable: 4296/*expression is always 'boolean_value'*/)
#endif

namespace c4 {
namespace yml {

template<class K> struct Key { K & k; };
template<> struct Key<fmt::const_base64_wrapper> { fmt::const_base64_wrapper wrapper; };
template<> struct Key<fmt::base64_wrapper> { fmt::base64_wrapper wrapper; };

template<class K> C4_ALWAYS_INLINE Key<K> key(K & k) { return Key<K>{k}; }
C4_ALWAYS_INLINE Key<fmt::const_base64_wrapper> key(fmt::const_base64_wrapper w) { return {w}; }
C4_ALWAYS_INLINE Key<fmt::base64_wrapper> key(fmt::base64_wrapper w) { return {w}; }

template<class T> void write(NodeRef *n, T const& v);

template<class T>
typename std::enable_if< ! std::is_floating_point<T>::value, bool>::type
read(NodeRef const& n, T *v);

template<class T>
typename std::enable_if<   std::is_floating_point<T>::value, bool>::type
read(NodeRef const& n, T *v);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// forward decls
class NodeRef;
class ConstNodeRef;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace detail {

template<class NodeRefType>
struct child_iterator
{
    using value_type = NodeRefType;
    using tree_type = typename NodeRefType::tree_type;

    tree_type * C4_RESTRICT m_tree;
    size_t m_child_id;

    child_iterator(tree_type * t, size_t id) : m_tree(t), m_child_id(id) {}

    child_iterator& operator++ () { RYML_ASSERT(m_child_id != NONE); m_child_id = m_tree->next_sibling(m_child_id); return *this; }
    child_iterator& operator-- () { RYML_ASSERT(m_child_id != NONE); m_child_id = m_tree->prev_sibling(m_child_id); return *this; }

    NodeRefType operator*  () const { return NodeRefType(m_tree, m_child_id); }
    NodeRefType operator-> () const { return NodeRefType(m_tree, m_child_id); }

    bool operator!= (child_iterator that) const { RYML_ASSERT(m_tree == that.m_tree); return m_child_id != that.m_child_id; }
    bool operator== (child_iterator that) const { RYML_ASSERT(m_tree == that.m_tree); return m_child_id == that.m_child_id; }
};

template<class NodeRefType>
struct children_view_
{
    using n_iterator = child_iterator<NodeRefType>;

    n_iterator b, e;

    inline children_view_(n_iterator const& C4_RESTRICT b_,
                          n_iterator const& C4_RESTRICT e_) : b(b_), e(e_) {}

    inline n_iterator begin() const { return b; }
    inline n_iterator end  () const { return e; }
};

template<class NodeRefType, class Visitor>
bool _visit(NodeRefType &node, Visitor fn, size_t indentation_level, bool skip_root=false)
{
    size_t increment = 0;
    if( ! (node.is_root() && skip_root))
    {
        if(fn(node, indentation_level))
            return true;
        ++increment;
    }
    if(node.has_children())
    {
        for(auto ch : node.children())
        {
            if(_visit(ch, fn, indentation_level + increment, false)) // no need to forward skip_root as it won't be root
            {
                return true;
            }
        }
    }
    return false;
}

template<class NodeRefType, class Visitor>
bool _visit_stacked(NodeRefType &node, Visitor fn, size_t indentation_level, bool skip_root=false)
{
    size_t increment = 0;
    if( ! (node.is_root() && skip_root))
    {
        if(fn(node, indentation_level))
        {
            return true;
        }
        ++increment;
    }
    if(node.has_children())
    {
        fn.push(node, indentation_level);
        for(auto ch : node.children())
        {
            if(_visit_stacked(ch, fn, indentation_level + increment, false)) // no need to forward skip_root as it won't be root
            {
                fn.pop(node, indentation_level);
                return true;
            }
        }
        fn.pop(node, indentation_level);
    }
    return false;
}


//-----------------------------------------------------------------------------

/** a CRTP base for read-only node methods */
template<class Impl, class ConstImpl>
struct RoNodeMethods
{
    C4_SUPPRESS_WARNING_CLANG_WITH_PUSH("-Wcast-align")
    // helper CRTP macros, undefined at the end
    #define tree_ ((ConstImpl const* C4_RESTRICT)this)->m_tree
    #define id_ ((ConstImpl const* C4_RESTRICT)this)->m_id
    // require valid
    #define _C4RV()                                       \
        RYML_ASSERT(tree_ != nullptr);                    \
        _RYML_CB_ASSERT(tree_->m_callbacks, id_ != NONE)

public:

    /** @name node property getters */
    /** @{ */

    /** returns the data or null when the id is NONE */
    C4_ALWAYS_INLINE C4_CONST NodeData const* get() const noexcept { RYML_ASSERT(tree_ != nullptr); return tree_->get(id_); }

    C4_ALWAYS_INLINE C4_CONST NodeType    type() const noexcept { _C4RV(); return tree_->type(id_); }
    C4_ALWAYS_INLINE C4_CONST const char* type_str() const noexcept { return tree_->type_str(id_); }

    C4_ALWAYS_INLINE C4_CONST csubstr key()        const noexcept { _C4RV(); return tree_->key(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr key_tag()    const noexcept { _C4RV(); return tree_->key_tag(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr key_ref()    const noexcept { _C4RV(); return tree_->key_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr key_anchor() const noexcept { _C4RV(); return tree_->key_anchor(id_); }

    C4_ALWAYS_INLINE C4_CONST csubstr val()        const noexcept { _C4RV(); return tree_->val(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr val_tag()    const noexcept { _C4RV(); return tree_->val_tag(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr val_ref()    const noexcept { _C4RV(); return tree_->val_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST csubstr val_anchor() const noexcept { _C4RV(); return tree_->val_anchor(id_); }

    C4_ALWAYS_INLINE C4_CONST NodeScalar const& keysc() const noexcept { _C4RV(); return tree_->keysc(id_); }
    C4_ALWAYS_INLINE C4_CONST NodeScalar const& valsc() const noexcept { _C4RV(); return tree_->valsc(id_); }

    C4_ALWAYS_INLINE C4_CONST bool key_is_null() const noexcept { _C4RV(); return tree_->key_is_null(id_); }
    C4_ALWAYS_INLINE C4_CONST bool val_is_null() const noexcept { _C4RV(); return tree_->val_is_null(id_); }

    /** @} */

public:

    /** @name node property predicates */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST bool empty()            const noexcept { _C4RV(); return tree_->empty(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_stream()        const noexcept { _C4RV(); return tree_->is_stream(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_doc()           const noexcept { _C4RV(); return tree_->is_doc(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_container()     const noexcept { _C4RV(); return tree_->is_container(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_map()           const noexcept { _C4RV(); return tree_->is_map(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_seq()           const noexcept { _C4RV(); return tree_->is_seq(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_val()          const noexcept { _C4RV(); return tree_->has_val(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_key()          const noexcept { _C4RV(); return tree_->has_key(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_val()           const noexcept { _C4RV(); return tree_->is_val(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_keyval()        const noexcept { _C4RV(); return tree_->is_keyval(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_key_tag()      const noexcept { _C4RV(); return tree_->has_key_tag(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_val_tag()      const noexcept { _C4RV(); return tree_->has_val_tag(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_key_anchor()   const noexcept { _C4RV(); return tree_->has_key_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_key_anchor()    const noexcept { _C4RV(); return tree_->is_key_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_val_anchor()   const noexcept { _C4RV(); return tree_->has_val_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_val_anchor()    const noexcept { _C4RV(); return tree_->is_val_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_anchor()       const noexcept { _C4RV(); return tree_->has_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_anchor()        const noexcept { _C4RV(); return tree_->is_anchor(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_key_ref()       const noexcept { _C4RV(); return tree_->is_key_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_val_ref()       const noexcept { _C4RV(); return tree_->is_val_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_ref()           const noexcept { _C4RV(); return tree_->is_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_anchor_or_ref() const noexcept { _C4RV(); return tree_->is_anchor_or_ref(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_key_quoted()    const noexcept { _C4RV(); return tree_->is_key_quoted(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_val_quoted()    const noexcept { _C4RV(); return tree_->is_val_quoted(id_); }
    C4_ALWAYS_INLINE C4_CONST bool is_quoted()        const noexcept { _C4RV(); return tree_->is_quoted(id_); }
    C4_ALWAYS_INLINE C4_CONST bool parent_is_seq()    const noexcept { _C4RV(); return tree_->parent_is_seq(id_); }
    C4_ALWAYS_INLINE C4_CONST bool parent_is_map()    const noexcept { _C4RV(); return tree_->parent_is_map(id_); }

    /** @} */

public:

    /** @name hierarchy predicates */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST bool is_root()    const noexcept { _C4RV(); return tree_->is_root(id_); }
    C4_ALWAYS_INLINE C4_CONST bool has_parent() const noexcept { _C4RV(); return tree_->has_parent(id_); }

    C4_ALWAYS_INLINE C4_CONST bool has_child(ConstImpl const& ch) const noexcept { _C4RV(); return tree_->has_child(id_, ch.m_id); }
    C4_ALWAYS_INLINE C4_CONST bool has_child(csubstr name) const noexcept { _C4RV(); return tree_->has_child(id_, name); }
    C4_ALWAYS_INLINE C4_CONST bool has_children() const noexcept { _C4RV(); return tree_->has_children(id_); }

    C4_ALWAYS_INLINE C4_CONST bool has_sibling(ConstImpl const& n) const noexcept { _C4RV(); return tree_->has_sibling(id_, n.m_id); }
    C4_ALWAYS_INLINE C4_CONST bool has_sibling(csubstr name) const noexcept { _C4RV(); return tree_->has_sibling(id_, name); }
    /** counts with this */
    C4_ALWAYS_INLINE C4_CONST bool has_siblings() const noexcept { _C4RV(); return tree_->has_siblings(id_); }
    /** does not count with this */
    C4_ALWAYS_INLINE C4_CONST bool has_other_siblings() const noexcept { _C4RV(); return tree_->has_other_siblings(id_); }

    /** @} */

public:

    /** @name hierarchy getters */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST ConstImpl parent() const noexcept { _C4RV(); return {tree_, tree_->parent(id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl prev_sibling() const noexcept { _C4RV(); return {tree_, tree_->prev_sibling(id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl next_sibling() const noexcept { _C4RV(); return {tree_, tree_->next_sibling(id_)}; }

    /** O(#num_children) */
    C4_ALWAYS_INLINE C4_CONST size_t    num_children() const noexcept { _C4RV(); return tree_->num_children(id_); }
    C4_ALWAYS_INLINE C4_CONST size_t    child_pos(ConstImpl const& n) const noexcept { _C4RV(); return tree_->child_pos(id_, n.m_id); }
    C4_ALWAYS_INLINE C4_CONST ConstImpl first_child() const noexcept { _C4RV(); return {tree_, tree_->first_child(id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl last_child () const noexcept { _C4RV(); return {tree_, tree_->last_child (id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl child(size_t pos) const noexcept { _C4RV(); return {tree_, tree_->child(id_, pos)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl find_child(csubstr name) const noexcept { _C4RV(); return {tree_, tree_->find_child(id_, name)}; }

    /** O(#num_siblings) */
    C4_ALWAYS_INLINE C4_CONST size_t    num_siblings() const noexcept { _C4RV(); return tree_->num_siblings(id_); }
    C4_ALWAYS_INLINE C4_CONST size_t    num_other_siblings() const noexcept { _C4RV(); return tree_->num_other_siblings(id_); }
    C4_ALWAYS_INLINE C4_CONST size_t    sibling_pos(ConstImpl const& n) const noexcept { _C4RV(); return tree_->child_pos(tree_->parent(id_), n.m_id); }
    C4_ALWAYS_INLINE C4_CONST ConstImpl first_sibling() const noexcept { _C4RV(); return {tree_, tree_->first_sibling(id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl last_sibling () const noexcept { _C4RV(); return {tree_, tree_->last_sibling(id_)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl sibling(size_t pos) const noexcept { _C4RV(); return {tree_, tree_->sibling(id_, pos)}; }
    C4_ALWAYS_INLINE C4_CONST ConstImpl find_sibling(csubstr name) const noexcept { _C4RV(); return {tree_, tree_->find_sibling(id_, name)}; }

    C4_ALWAYS_INLINE C4_CONST ConstImpl doc(size_t num) const noexcept { _C4RV(); return {tree_, tree_->doc(num)}; }

    /** O(num_children) */
    C4_ALWAYS_INLINE C4_CONST ConstImpl operator[] (csubstr k) const noexcept
    {
        _C4RV();
        size_t ch = tree_->find_child(id_, k);
        _RYML_CB_ASSERT(tree_->m_callbacks, ch != NONE);
        return {tree_, ch};
    }

    /** O(num_children) */
    C4_ALWAYS_INLINE C4_CONST ConstImpl operator[] (size_t pos) const noexcept
    {
        _C4RV();
        size_t ch = tree_->child(id_, pos);
        _RYML_CB_ASSERT(tree_->m_callbacks, ch != NONE);
        return {tree_, ch};
    }

    /** @} */

public:

    /** deserialization */
    /** @{ */

    template<class T>
    ConstImpl const& operator>> (T &v) const
    {
        _C4RV();
        if( ! read((ConstImpl const&)*this, &v))
            _RYML_CB_ERR(tree_->m_callbacks, "could not deserialize value");
        return *((ConstImpl const*)this);
    }

    /** deserialize the node's key to the given variable */
    template<class T>
    ConstImpl const& operator>> (Key<T> v) const
    {
        _C4RV();
        if( ! from_chars(key(), &v.k))
            _RYML_CB_ERR(tree_->m_callbacks, "could not deserialize key");
        return *((ConstImpl const*)this);
    }

    /** deserialize the node's key as base64 */
    ConstImpl const& operator>> (Key<fmt::base64_wrapper> w) const
    {
        deserialize_key(w.wrapper);
        return *((ConstImpl const*)this);
    }

    /** deserialize the node's val as base64 */
    ConstImpl const& operator>> (fmt::base64_wrapper w) const
    {
        deserialize_val(w);
        return *((ConstImpl const*)this);
    }

    /** decode the base64-encoded key and assign the
     * decoded blob to the given buffer/
     * @return the size of base64-decoded blob */
    size_t deserialize_key(fmt::base64_wrapper v) const
    {
        _C4RV();
        return from_chars(key(), &v);
    }
    /** decode the base64-encoded key and assign the
     * decoded blob to the given buffer/
     * @return the size of base64-decoded blob */
    size_t deserialize_val(fmt::base64_wrapper v) const
    {
        _C4RV();
        return from_chars(val(), &v);
    };

    template<class T>
    bool get_if(csubstr name, T *var) const
    {
        auto ch = find_child(name);
        if(!ch.valid())
            return false;
        ch >> *var;
        return true;
    }

    template<class T>
    bool get_if(csubstr name, T *var, T const& fallback) const
    {
        auto ch = find_child(name);
        if(ch.valid())
        {
            ch >> *var;
            return true;
        }
        else
        {
            *var = fallback;
            return false;
        }
    }

    /** @} */

public:

    /** @name iteration */
    /** @{ */

    using const_iterator = detail::child_iterator<ConstImpl>;
    using const_children_view = detail::children_view_<ConstImpl>;

    C4_ALWAYS_INLINE C4_CONST const_iterator begin() const noexcept { _C4RV(); return const_iterator(tree_, tree_->first_child(id_)); }
    C4_ALWAYS_INLINE C4_CONST const_iterator end  () const noexcept { _C4RV(); return const_iterator(tree_, NONE); }

    C4_ALWAYS_INLINE C4_CONST const_children_view children() const noexcept { _C4RV(); return const_children_view(begin(), end()); }

    #if defined(__clang__)
    #   pragma clang diagnostic push
    #   pragma clang diagnostic ignored "-Wnull-dereference"
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic push
    #   if __GNUC__ >= 6
    #       pragma GCC diagnostic ignored "-Wnull-dereference"
    #   endif
    #endif

    C4_ALWAYS_INLINE C4_CONST const_children_view siblings() const noexcept
    {
        _C4RV();
        if(is_root())
            return const_children_view(end(), end());
        size_t p = get()->m_parent;
        return const_children_view(
            const_iterator(tree_, tree_->get(p)->m_first_child),
            const_iterator(tree_, NONE));
    }

    #if defined(__clang__)
    #   pragma clang diagnostic pop
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic pop
    #endif

    /** visit every child node calling fn(node) */
    template<class Visitor>
    C4_ALWAYS_INLINE C4_CONST bool visit(Visitor fn, size_t indentation_level=0, bool skip_root=true) const noexcept
    {
        return detail::_visit(*(ConstImpl*)this, fn, indentation_level, skip_root);
    }

    /** visit every child node calling fn(node, level) */
    template<class Visitor>
    C4_ALWAYS_INLINE C4_CONST bool visit_stacked(Visitor fn, size_t indentation_level=0, bool skip_root=true) const noexcept
    {
        return detail::_visit_stacked(*(ConstImpl*)this, fn, indentation_level, skip_root);
    }

    /** @} */

    #undef _C4RV
    #undef tree_
    #undef id_
    C4_SUPPRESS_WARNING_CLANG_POP
};

} // namespace detail


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class RYML_EXPORT ConstNodeRef : public detail::RoNodeMethods<ConstNodeRef, ConstNodeRef>
{
public:

    using tree_type = Tree const;

public:

    Tree const* C4_RESTRICT m_tree;
    size_t m_id;

    friend NodeRef;
    template<class Impl, class ConstImpl>
    friend struct RoNodeMethods;

public:

    /** @name construction */
    /** @{ */

    ConstNodeRef() : m_tree(nullptr), m_id(NONE) {}
    ConstNodeRef(Tree const &t) : m_tree(&t), m_id(t .root_id()) {}
    ConstNodeRef(Tree const *t) : m_tree(t ), m_id(t->root_id()) {}
    ConstNodeRef(Tree const *t, size_t id) : m_tree(t), m_id(id) {}
    ConstNodeRef(std::nullptr_t) : m_tree(nullptr), m_id(NONE) {}

    ConstNodeRef(ConstNodeRef const&) = default;
    ConstNodeRef(ConstNodeRef     &&) = default;

    ConstNodeRef(NodeRef const&);
    ConstNodeRef(NodeRef     &&);

    /** @} */

public:

    /** @name assignment */
    /** @{ */

    ConstNodeRef& operator= (std::nullptr_t) { m_tree = nullptr; m_id = NONE; return *this; }

    ConstNodeRef& operator= (ConstNodeRef const&) = default;
    ConstNodeRef& operator= (ConstNodeRef     &&) = default;

    ConstNodeRef& operator= (NodeRef const&);
    ConstNodeRef& operator= (NodeRef     &&);


    /** @} */

public:

    /** @name state queries */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST bool valid() const noexcept { return m_tree != nullptr && m_id != NONE; }

    /** @} */

public:

    /** @name member getters */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST Tree const* tree() const noexcept { return m_tree; }
    C4_ALWAYS_INLINE C4_CONST size_t id() const noexcept { return m_id; }

    /** @} */

public:

    /** @name comparisons */
    /** @{ */

    C4_ALWAYS_INLINE C4_CONST bool operator== (ConstNodeRef const& that) const noexcept { RYML_ASSERT(that.m_tree == m_tree); return m_id == that.m_id; }
    C4_ALWAYS_INLINE C4_CONST bool operator!= (ConstNodeRef const& that) const noexcept { RYML_ASSERT(that.m_tree == m_tree); return ! this->operator==(that); }

    C4_ALWAYS_INLINE C4_CONST bool operator== (std::nullptr_t) const noexcept { return m_tree == nullptr || m_id == NONE; }
    C4_ALWAYS_INLINE C4_CONST bool operator!= (std::nullptr_t) const noexcept { return ! this->operator== (nullptr); }

    C4_ALWAYS_INLINE C4_CONST bool operator== (csubstr val) const noexcept { RYML_ASSERT(has_val()); return m_tree->val(m_id) == val; }
    C4_ALWAYS_INLINE C4_CONST bool operator!= (csubstr val) const noexcept { RYML_ASSERT(has_val()); return m_tree->val(m_id) != val; }

    /** @} */

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** a reference to a node in an existing yaml tree, offering a more
 * convenient API than the index-based API used in the tree. */
class RYML_EXPORT NodeRef : public detail::RoNodeMethods<NodeRef, ConstNodeRef>
{
public:

    using tree_type = Tree;

private:

    Tree *C4_RESTRICT m_tree;
    size_t m_id;

    /** This member is used to enable lazy operator[] writing. When a child
     * with a key or index is not found, m_id is set to the id of the parent
     * and the asked-for key or index are stored in this member until a write
     * does happen. Then it is given as key or index for creating the child.
     * When a key is used, the csubstr stores it (so the csubstr's string is
     * non-null and the csubstr's size is different from NONE). When an index is
     * used instead, the csubstr's string is set to null, and only the csubstr's
     * size is set to a value different from NONE. Otherwise, when operator[]
     * does find the child then this member is empty: the string is null and
     * the size is NONE. */
    csubstr m_seed;

    friend ConstNodeRef;
    template<class Impl, class ConstImpl>
    friend struct RoNodeMethods;

    // require valid: a helper macro, undefined at the end
    #define _C4RV()                                                         \
        RYML_ASSERT(m_tree != nullptr);                                     \
        _RYML_CB_ASSERT(m_tree->m_callbacks, m_id != NONE && !is_seed())

public:

    /** @name construction */
    /** @{ */

    NodeRef() : m_tree(nullptr), m_id(NONE), m_seed() { _clear_seed(); }
    NodeRef(Tree &t) : m_tree(&t), m_id(t .root_id()), m_seed() { _clear_seed(); }
    NodeRef(Tree *t) : m_tree(t ), m_id(t->root_id()), m_seed() { _clear_seed(); }
    NodeRef(Tree *t, size_t id) : m_tree(t), m_id(id), m_seed() { _clear_seed(); }
    NodeRef(Tree *t, size_t id, size_t seed_pos) : m_tree(t), m_id(id), m_seed() { m_seed.str = nullptr; m_seed.len = seed_pos; }
    NodeRef(Tree *t, size_t id, csubstr  seed_key) : m_tree(t), m_id(id), m_seed(seed_key) {}
    NodeRef(std::nullptr_t) : m_tree(nullptr), m_id(NONE), m_seed() {}

    /** @} */

public:

    /** @name assignment */
    /** @{ */

    NodeRef(NodeRef const&) = default;
    NodeRef(NodeRef     &&) = default;

    NodeRef& operator= (NodeRef const&) = default;
    NodeRef& operator= (NodeRef     &&) = default;

    NodeRef& operator= (std::nullptr_t) { m_tree = nullptr; m_id = NONE; m_seed = {}; return *this; }

    /** @} */

public:

    /** @name state queries */
    /** @{ */

    inline bool valid() const { return m_tree != nullptr && m_id != NONE; }
    inline bool is_seed() const { return m_seed.str != nullptr || m_seed.len != NONE; }

    inline void _clear_seed() { /*do this manually or an assert is triggered*/ m_seed.str = nullptr; m_seed.len = NONE; }

    /** @} */

public:

    /** @name comparisons */
    /** @{ */

    inline bool operator== (NodeRef const& that) const { _C4RV(); RYML_ASSERT(that.valid() && !that.is_seed()); RYML_ASSERT(that.m_tree == m_tree); return m_id == that.m_id; }
    inline bool operator!= (NodeRef const& that) const { return ! this->operator==(that); }

    inline bool operator== (ConstNodeRef const& that) const { _C4RV(); RYML_ASSERT(that.valid()); RYML_ASSERT(that.m_tree == m_tree); return m_id == that.m_id; }
    inline bool operator!= (ConstNodeRef const& that) const { return ! this->operator==(that); }

    inline bool operator== (std::nullptr_t) const { return m_tree == nullptr || m_id == NONE || is_seed(); }
    inline bool operator!= (std::nullptr_t) const { return m_tree != nullptr && m_id != NONE && !is_seed(); }

    inline bool operator== (csubstr val) const { _C4RV(); RYML_ASSERT(has_val()); return m_tree->val(m_id) == val; }
    inline bool operator!= (csubstr val) const { _C4RV(); RYML_ASSERT(has_val()); return m_tree->val(m_id) != val; }

    //inline operator bool () const { return m_tree == nullptr || m_id == NONE || is_seed(); }

    /** @} */

public:

    /** @name non-const node property getters */
    /** @{ */

    inline Tree * tree() { return m_tree; }
    inline size_t id() const { return m_id; }

    /** returns the data or null when the id is NONE */
    inline NodeData * get() { RYML_ASSERT(m_tree != nullptr); return m_tree->get(m_id); }

    /** @} */

public:

    /** @name hierarchy getters */
    /** @{ */

    NodeRef parent() { _C4RV(); return {m_tree, m_tree->parent(m_id)}; }
    NodeRef prev_sibling() { _C4RV(); return {m_tree, m_tree->prev_sibling(m_id)}; }
    NodeRef next_sibling() { _C4RV(); return {m_tree, m_tree->next_sibling(m_id)}; }

    /** O(#num_children) */
    NodeRef first_child(){ _C4RV(); return {m_tree, m_tree->first_child(m_id)}; }
    NodeRef last_child() { _C4RV(); return {m_tree, m_tree->last_child(m_id)}; }
    NodeRef child(size_t pos) { _C4RV(); return {m_tree, m_tree->child(m_id, pos)}; }
    NodeRef find_child(csubstr name) { _C4RV(); return {m_tree, m_tree->find_child(m_id, name)}; }

    /** O(#num_siblings) */
    NodeRef first_sibling() { _C4RV(); return {m_tree, m_tree->first_sibling(m_id)}; }
    NodeRef last_sibling () { _C4RV(); return {m_tree, m_tree->last_sibling(m_id)}; }
    NodeRef sibling(size_t pos) { _C4RV(); return {m_tree, m_tree->sibling(m_id, pos)}; }
    NodeRef find_sibling(csubstr name) { _C4RV(); return {m_tree, m_tree->find_sibling(m_id, name)}; }

    NodeRef doc(size_t num) { _C4RV(); return {m_tree, m_tree->doc(num)}; }

    /** Find child by key. O(num_children). returns a seed node if no such child is found.  */
    NodeRef operator[] (csubstr k)
    {
        _C4RV();
        size_t ch = m_tree->find_child(m_id, k);
        return ch != NONE ? NodeRef(m_tree, ch) : NodeRef(m_tree, m_id, k);
    }

    /** Find child by position. O(pos). returns a seed node if no such child is found.  */
    NodeRef operator[] (size_t pos)
    {
        _C4RV();
        size_t ch = m_tree->child(m_id, pos);
        return ch != NONE ? NodeRef(m_tree, ch) : NodeRef(m_tree, m_id, pos);
    }

    /** @} */

public:

    /** @name node modifiers */
    /** @{ */

    void change_type(NodeType t) { _C4RV(); m_tree->change_type(m_id, t); }

    void set_type(NodeType t) { _C4RV(); m_tree->_set_flags(m_id, t); }
    void set_key(csubstr key) { _C4RV(); m_tree->_set_key(m_id, key); }
    void set_val(csubstr val) { _C4RV(); m_tree->_set_val(m_id, val); }
    void set_key_tag(csubstr key_tag) { _C4RV(); m_tree->set_key_tag(m_id, key_tag); }
    void set_val_tag(csubstr val_tag) { _C4RV(); m_tree->set_val_tag(m_id, val_tag); }
    void set_key_anchor(csubstr key_anchor) { _C4RV(); m_tree->set_key_anchor(m_id, key_anchor); }
    void set_val_anchor(csubstr val_anchor) { _C4RV(); m_tree->set_val_anchor(m_id, val_anchor); }
    void set_key_ref(csubstr key_ref) { _C4RV(); m_tree->set_key_ref(m_id, key_ref); }
    void set_val_ref(csubstr val_ref) { _C4RV(); m_tree->set_val_ref(m_id, val_ref); }

    template<class T>
    size_t set_key_serialized(T const& C4_RESTRICT k)
    {
        _C4RV();
        csubstr s = m_tree->to_arena(k);
        m_tree->_set_key(m_id, s);
        return s.len;
    }
    template<class T>
    size_t set_val_serialized(T const& C4_RESTRICT v)
    {
        _C4RV();
        csubstr s = m_tree->to_arena(v);
        m_tree->_set_val(m_id, s);
        return s.len;
    }

    /** encode a blob as base64, then assign the result to the node's key
     * @return the size of base64-encoded blob */
    size_t set_key_serialized(fmt::const_base64_wrapper w);
    /** encode a blob as base64, then assign the result to the node's val
     * @return the size of base64-encoded blob */
    size_t set_val_serialized(fmt::const_base64_wrapper w);

public:

    inline void clear()
    {
        if(is_seed())
            return;
        m_tree->remove_children(m_id);
        m_tree->_clear(m_id);
    }

    inline void clear_key()
    {
        if(is_seed())
            return;
        m_tree->_clear_key(m_id);
    }

    inline void clear_val()
    {
        if(is_seed())
            return;
        m_tree->_clear_val(m_id);
    }

    inline void clear_children()
    {
        if(is_seed())
            return;
        m_tree->remove_children(m_id);
    }

    void create() { _apply_seed(); }

    inline void operator= (NodeType_e t)
    {
        _apply_seed();
        m_tree->_add_flags(m_id, t);
    }

    inline void operator|= (NodeType_e t)
    {
        _apply_seed();
        m_tree->_add_flags(m_id, t);
    }

    inline void operator= (NodeInit const& v)
    {
        _apply_seed();
        _apply(v);
    }

    inline void operator= (NodeScalar const& v)
    {
        _apply_seed();
        _apply(v);
    }

    inline void operator= (csubstr v)
    {
        _apply_seed();
        _apply(v);
    }

    template<size_t N>
    inline void operator= (const char (&v)[N])
    {
        _apply_seed();
        csubstr sv;
        sv.assign<N>(v);
        _apply(sv);
    }

    /** @} */

public:

    /** @name serialization */
    /** @{ */

    /** serialize a variable to the arena */
    template<class T>
    inline csubstr to_arena(T const& C4_RESTRICT s) const
    {
        _C4RV();
        return m_tree->to_arena(s);
    }

    /** serialize a variable, then assign the result to the node's val */
    inline NodeRef& operator<< (csubstr s)
    {
        // this overload is needed to prevent ambiguity (there's also
        // operator<< for writing a substr to a stream)
        _apply_seed();
        write(this, s);
        RYML_ASSERT(val() == s);
        return *this;
    }

    template<class T>
    inline NodeRef& operator<< (T const& C4_RESTRICT v)
    {
        _apply_seed();
        write(this, v);
        return *this;
    }

    /** serialize a variable, then assign the result to the node's key */
    template<class T>
    inline NodeRef& operator<< (Key<const T> const& C4_RESTRICT v)
    {
        _apply_seed();
        set_key_serialized(v.k);
        return *this;
    }

    /** serialize a variable, then assign the result to the node's key */
    template<class T>
    inline NodeRef& operator<< (Key<T> const& C4_RESTRICT v)
    {
        _apply_seed();
        set_key_serialized(v.k);
        return *this;
    }

    NodeRef& operator<< (Key<fmt::const_base64_wrapper> w)
    {
        set_key_serialized(w.wrapper);
        return *this;
    }

    NodeRef& operator<< (fmt::const_base64_wrapper w)
    {
        set_val_serialized(w);
        return *this;
    }

    /** @} */

private:

    void _apply_seed()
    {
        if(m_seed.str) // we have a seed key: use it to create the new child
        {
            //RYML_ASSERT(i.key.scalar.empty() || m_key == i.key.scalar || m_key.empty());
            m_id = m_tree->append_child(m_id);
            m_tree->_set_key(m_id, m_seed);
            m_seed.str = nullptr;
            m_seed.len = NONE;
        }
        else if(m_seed.len != NONE) // we have a seed index: create a child at that position
        {
            RYML_ASSERT(m_tree->num_children(m_id) == m_seed.len);
            m_id = m_tree->append_child(m_id);
            m_seed.str = nullptr;
            m_seed.len = NONE;
        }
        else
        {
            RYML_ASSERT(valid());
        }
    }

    inline void _apply(csubstr v)
    {
        m_tree->_set_val(m_id, v);
    }

    inline void _apply(NodeScalar const& v)
    {
        m_tree->_set_val(m_id, v);
    }

    inline void _apply(NodeInit const& i)
    {
        m_tree->_set(m_id, i);
    }

public:

    /** @name modification of hierarchy */
    /** @{ */

    inline NodeRef insert_child(NodeRef after)
    {
        _C4RV();
        RYML_ASSERT(after.m_tree == m_tree);
        NodeRef r(m_tree, m_tree->insert_child(m_id, after.m_id));
        return r;
    }

    inline NodeRef insert_child(NodeInit const& i, NodeRef after)
    {
        _C4RV();
        RYML_ASSERT(after.m_tree == m_tree);
        NodeRef r(m_tree, m_tree->insert_child(m_id, after.m_id));
        r._apply(i);
        return r;
    }

    inline NodeRef prepend_child()
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->insert_child(m_id, NONE));
        return r;
    }

    inline NodeRef prepend_child(NodeInit const& i)
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->insert_child(m_id, NONE));
        r._apply(i);
        return r;
    }

    inline NodeRef append_child()
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->append_child(m_id));
        return r;
    }

    inline NodeRef append_child(NodeInit const& i)
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->append_child(m_id));
        r._apply(i);
        return r;
    }

public:

    inline NodeRef insert_sibling(ConstNodeRef const& after)
    {
        _C4RV();
        RYML_ASSERT(after.m_tree == m_tree);
        NodeRef r(m_tree, m_tree->insert_sibling(m_id, after.m_id));
        return r;
    }

    inline NodeRef insert_sibling(NodeInit const& i, ConstNodeRef const& after)
    {
        _C4RV();
        RYML_ASSERT(after.m_tree == m_tree);
        NodeRef r(m_tree, m_tree->insert_sibling(m_id, after.m_id));
        r._apply(i);
        return r;
    }

    inline NodeRef prepend_sibling()
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->prepend_sibling(m_id));
        return r;
    }

    inline NodeRef prepend_sibling(NodeInit const& i)
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->prepend_sibling(m_id));
        r._apply(i);
        return r;
    }

    inline NodeRef append_sibling()
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->append_sibling(m_id));
        return r;
    }

    inline NodeRef append_sibling(NodeInit const& i)
    {
        _C4RV();
        NodeRef r(m_tree, m_tree->append_sibling(m_id));
        r._apply(i);
        return r;
    }

public:

    inline void remove_child(NodeRef & child)
    {
        _C4RV();
        RYML_ASSERT(has_child(child));
        RYML_ASSERT(child.parent().id() == id());
        m_tree->remove(child.id());
        child.clear();
    }

    //! remove the nth child of this node
    inline void remove_child(size_t pos)
    {
        _C4RV();
        RYML_ASSERT(pos >= 0 && pos < num_children());
        size_t child = m_tree->child(m_id, pos);
        RYML_ASSERT(child != NONE);
        m_tree->remove(child);
    }

    //! remove a child by name
    inline void remove_child(csubstr key)
    {
        _C4RV();
        size_t child = m_tree->find_child(m_id, key);
        RYML_ASSERT(child != NONE);
        m_tree->remove(child);
    }

public:

    /** change the node's position within its parent */
    inline void move(ConstNodeRef const& after)
    {
        _C4RV();
        m_tree->move(m_id, after.m_id);
    }

    /** move the node to a different parent, which may belong to a different
     * tree. When this is the case, then this node's tree pointer is reset to
     * the tree of the parent node. */
    inline void move(NodeRef const& parent, ConstNodeRef const& after)
    {
        _C4RV();
        RYML_ASSERT(parent.m_tree == after.m_tree);
        if(parent.m_tree == m_tree)
        {
            m_tree->move(m_id, parent.m_id, after.m_id);
        }
        else
        {
            parent.m_tree->move(m_tree, m_id, parent.m_id, after.m_id);
            m_tree = parent.m_tree;
        }
    }

    inline NodeRef duplicate(NodeRef const& parent, ConstNodeRef const& after) const
    {
        _C4RV();
        RYML_ASSERT(parent.m_tree == after.m_tree);
        if(parent.m_tree == m_tree)
        {
            size_t dup = m_tree->duplicate(m_id, parent.m_id, after.m_id);
            NodeRef r(m_tree, dup);
            return r;
        }
        else
        {
            size_t dup = parent.m_tree->duplicate(m_tree, m_id, parent.m_id, after.m_id);
            NodeRef r(parent.m_tree, dup);
            return r;
        }
    }

    inline void duplicate_children(NodeRef const& parent, ConstNodeRef const& after) const
    {
        _C4RV();
        RYML_ASSERT(parent.m_tree == after.m_tree);
        if(parent.m_tree == m_tree)
        {
            m_tree->duplicate_children(m_id, parent.m_id, after.m_id);
        }
        else
        {
            parent.m_tree->duplicate_children(m_tree, m_id, parent.m_id, after.m_id);
        }
    }

    /** @} */

public:

    /** @name iteration */
    /** @{ */

    using       iterator = detail::child_iterator<     NodeRef>;

    using       children_view = detail::children_view_<     NodeRef>;

    inline iterator begin() { return iterator(m_tree, m_tree->first_child(m_id)); }
    inline iterator end  () { return iterator(m_tree, NONE); }

          children_view children()       { return       children_view(begin(), end()); }

    #if defined(__clang__)
    #   pragma clang diagnostic push
    #   pragma clang diagnostic ignored "-Wnull-dereference"
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic push
    #   if __GNUC__ >= 6
    #       pragma GCC diagnostic ignored "-Wnull-dereference"
    #   endif
    #endif

    children_view siblings() { if(is_root()) { return children_view(end(), end()); } else { size_t p = get()->m_parent; return       children_view(iterator(m_tree, m_tree->get(p)->m_first_child), iterator(m_tree, NONE)); } }

    #if defined(__clang__)
    #   pragma clang diagnostic pop
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic pop
    #endif

    /** visit every child node calling fn(node) */
    template<class Visitor>
    bool visit(Visitor fn, size_t indentation_level=0, bool skip_root=true)
    {
        return detail::_visit(*this, fn, indentation_level, skip_root);
    }
    /** visit every child node calling fn(node, level) */
    template<class Visitor>
    bool visit_stacked(Visitor fn, size_t indentation_level=0, bool skip_root=true)
    {
        return detail::_visit_stacked(*this, fn, indentation_level, skip_root);
    }

    /** @} */

#undef _C4RV
};


//-----------------------------------------------------------------------------

inline ConstNodeRef::ConstNodeRef(NodeRef const& that)
    : m_tree(that.m_tree)
    , m_id(!that.is_seed() ? that.id() : NONE)
{
}

inline ConstNodeRef::ConstNodeRef(NodeRef && that)
    : m_tree(that.m_tree)
    , m_id(!that.is_seed() ? that.id() : NONE)
{
}


inline ConstNodeRef& ConstNodeRef::operator= (NodeRef const& that)
{
    m_tree = (that.m_tree);
    m_id = (!that.is_seed() ? that.id() : NONE);
    return *this;
}

inline ConstNodeRef& ConstNodeRef::operator= (NodeRef && that)
{
    m_tree = (that.m_tree);
    m_id = (!that.is_seed() ? that.id() : NONE);
    return *this;
}


//-----------------------------------------------------------------------------

template<class T>
inline void write(NodeRef *n, T const& v)
{
    n->set_val_serialized(v);
}

template<class T>
typename std::enable_if< ! std::is_floating_point<T>::value, bool>::type
inline read(NodeRef const& n, T *v)
{
    return from_chars(n.val(), v);
}
template<class T>
typename std::enable_if< ! std::is_floating_point<T>::value, bool>::type
inline read(ConstNodeRef const& n, T *v)
{
    return from_chars(n.val(), v);
}

template<class T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
inline read(NodeRef const& n, T *v)
{
    return from_chars_float(n.val(), v);
}
template<class T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
inline read(ConstNodeRef const& n, T *v)
{
    return from_chars_float(n.val(), v);
}


} // namespace yml
} // namespace c4


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#ifdef __GNUC__
#   pragma GCC diagnostic pop
#endif

#endif /* _C4_YML_NODE_HPP_ */
