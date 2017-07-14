#include "../include/avl_tree.h"
#include "../include/memory_pool.h"
#include <string.h>
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable : 4201 )
struct avl_node 
{
    struct avl_node*    avl_child[2];
    struct avl_node*    avl_parent;
    ptrdiff_t           avl_height;

    struct avl_node*    list_prev;
    struct avl_node*    list_next;

    union
    {
        size_t              key_int;
        const char*         key_str;
        void*               key_user;
        unsigned long long  key_int64;
    };
    void* value;
};
#pragma warning( pop )

struct avl_tree
{
    struct avl_node*    root;
    struct avl_node*    head;
    struct avl_node*    tail;
    size_t              size;

    user_key_cmp        key_cmp;
};

static __inline ptrdiff_t _avl_node_height(struct avl_node* node)
{
    return node ? node->avl_height : 0;
}

static __inline void _avl_link_node(struct avl_tree* root, struct avl_node * node, struct avl_node * parent,
struct avl_node ** avl_link)
{
    node->avl_height = 1;
    node->avl_parent = parent;
    node->avl_child[0] = node->avl_child[1] = 0;

    *avl_link = node;

    if (parent)
    {
        if (&(parent->avl_child[0]) == avl_link)
        {
            node->list_next = parent;
            node->list_prev = parent->list_prev;

            if (parent->list_prev)
            {
                parent->list_prev->list_next = node;
            }

            parent->list_prev = node;

            if (root->head == parent)
            {
                root->head = node;
            }
        }
        else
        {
            node->list_next = parent->list_next;
            node->list_prev = parent;

            if (parent->list_next)
            {
                parent->list_next->list_prev = node;
            }

            parent->list_next = node;

            if (root->tail == parent)
            {
                root->tail = node;
            }
        }
    }
    else
    {
        root->head = node;
        root->tail = node;
        node->list_next = 0;
        node->list_prev = 0;
    }

    ++root->size;
}

static HMEMORYUNIT _get_avl_tree_unit(void)
{
    static HMEMORYUNIT tree_unit = 0;

    if (tree_unit)
    {
        return tree_unit;
    }

    tree_unit = create_memory_unit(sizeof(struct avl_tree));

    return tree_unit;
}

static HMEMORYUNIT _get_avl_node_unit(void)
{
    static HMEMORYUNIT node_unit = 0;

    if (node_unit)
    {
        return node_unit;
    }

    node_unit = create_memory_unit(sizeof(struct avl_node));

    return node_unit;
}

static void _avl_roate(struct avl_tree* tree, struct avl_node* node)
{
    const ptrdiff_t side = (node != node->avl_parent->avl_child[0]);
    const ptrdiff_t other_side = !side;

    struct avl_node* child = node->avl_child[other_side];
    struct avl_node* parent = node->avl_parent;

    node->avl_parent = parent->avl_parent;
    node->avl_child[other_side] = parent;

    parent->avl_child[side] = child;
    if (child)
        child->avl_parent = parent;

    if (parent->avl_parent)
    {
        const ptrdiff_t parent_side = (parent != parent->avl_parent->avl_child[0]);
        parent->avl_parent->avl_child[parent_side] = node;
    }
    else
        tree->root = node;

    parent->avl_parent = node;

    parent->avl_height = 1 + max(_avl_node_height(parent->avl_child[0]),
                                 _avl_node_height(parent->avl_child[1]));
    node->avl_height = 1 + max(_avl_node_height(node->avl_child[0]),
                                _avl_node_height(node->avl_child[1]));
}

static void _avl_balance(struct avl_tree* tree, struct avl_node* node)
{
    while (node)
    {
        ptrdiff_t balance;

        node->avl_height = 1 + max(_avl_node_height(node->avl_child[0]),
                                    _avl_node_height(node->avl_child[1]));

        balance = _avl_node_height(node->avl_child[0]) - _avl_node_height(node->avl_child[1]);

        if (balance == 2 || balance == -2)
        {
            ptrdiff_t child_balance;

            struct avl_node* tall_child = node->avl_child[balance == -2];

            child_balance = _avl_node_height(tall_child->avl_child[0]) - _avl_node_height(tall_child->avl_child[1]);

            if (child_balance == 0 || (child_balance < 0) == (balance < 0))
            {
                _avl_roate(tree, tall_child);

                node = tall_child->avl_parent;
            }
            else
            {
                struct avl_node* tall_grand_child = tall_child->avl_child[child_balance == -1];

                _avl_roate(tree, tall_grand_child);
                _avl_roate(tree, tall_grand_child);

                node = tall_grand_child->avl_parent;
            }
        }
        else
        {
            node = node->avl_parent;
        }
    }
}

void _avl_splice_out(struct avl_tree* tree, struct avl_node* node)
{
    //assert(!node->avl_child[0] || !node->avl_child[1]);

    struct avl_node* parent = node->avl_parent;

    const size_t child_index = (node->avl_child[1] != 0);

    struct avl_node* child = node->avl_child[child_index];

    //assert(node->avl_child[!child_index] == 0);

    if (child)
        child->avl_parent = parent;

    if (parent)
    {
        parent->avl_child[node == parent->avl_child[1]] = child;
    }
    else
        tree->root = child;
}

void avl_tree_erase(struct avl_tree* tree, struct avl_node* node)
{
    if (node)
    {
        struct avl_node* parent = node->avl_parent;

        if (!node->avl_child[0] || !node->avl_child[1])
        {
            _avl_splice_out(tree, node);
            _avl_balance(tree, parent);
        }
        else
        {
            struct avl_node* successor = node->list_next;

            struct avl_node* successor_parent = successor->avl_parent;

            _avl_splice_out(tree, successor);

            successor->avl_parent = parent;
            successor->avl_child[0] = node->avl_child[0];
            successor->avl_child[1] = node->avl_child[1];

            if (successor->avl_child[0])
            {
                successor->avl_child[0]->avl_parent = successor;
            }

            if (successor->avl_child[1])
            {
                successor->avl_child[1]->avl_parent = successor;
            }

            if (parent)
            {
                parent->avl_child[node == parent->avl_child[1]] = successor;
            }
            else
                tree->root = successor;

            _avl_balance(tree, node == successor_parent ? successor : successor_parent);
        }

        if (node->list_next)
        {
            node->list_next->list_prev = node->list_prev;
        }
        else
        {
            tree->tail = node->list_prev;
        }

        if (node->list_prev)
        {
            node->list_prev->list_next = node->list_next;
        }
        else
        {
            tree->head = node->list_next;
        }

        memory_unit_free(_get_avl_node_unit(), node);
        --tree->size;
    }
}

struct avl_node* avl_tree_insert_int(struct avl_tree* tree, size_t key, void* value)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (key > (*new_node)->key_int)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_int = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    return node;
}

struct avl_node* avl_tree_insert_int64(struct avl_tree* tree, unsigned long long key, void* value)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int64)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (key > (*new_node)->key_int64)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_int64 = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    return node;
}

struct avl_node* avl_tree_insert_str(struct avl_tree* tree, const char* key, void* value)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        int cmp_ret = strcmp(key, (*new_node)->key_str);

        parent = *new_node;

        if (cmp_ret < 0)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (cmp_ret > 0)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            (*new_node)->key_str = key;
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_str = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    return node;
}

struct avl_node* avl_tree_insert_user(struct avl_tree* tree, void* key, void* value)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    if (tree->key_cmp)
    {
        while (*new_node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, (*new_node)->key_user);

            parent = *new_node;

            if (cmp_ret < 0)
            {
                new_node = &((*new_node)->avl_child[0]);
            }
            else if (cmp_ret > 0)
            {
                new_node = &((*new_node)->avl_child[1]);
            }
            else
            {
                (*new_node)->key_user = key;
                (*new_node)->value = value;
                return (*new_node);
            }
        }
    }
    else
    {
        while (*new_node)
        {
            parent = *new_node;

            if (key < (*new_node)->key_user)
            {
                new_node = &((*new_node)->avl_child[0]);
            }
            else if (key > (*new_node)->key_user)
            {
                new_node = &((*new_node)->avl_child[1]);
            }
            else
            {
                (*new_node)->key_user = key;
                (*new_node)->value = value;
                return (*new_node);
            }
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_user = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    return node;
}

bool avl_tree_try_insert_int(struct avl_tree* tree, size_t key, void* value, struct avl_node** insert_or_exist_node)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (key > (*new_node)->key_int)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_int = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    *insert_or_exist_node = node;

    return true;
}

bool avl_tree_try_insert_int64(struct avl_tree* tree, unsigned long long key, void* value, struct avl_node** insert_or_exist_node)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int64)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (key > (*new_node)->key_int64)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_int64 = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    *insert_or_exist_node = node;

    return true;
}

bool avl_tree_try_insert_str(struct avl_tree* tree, const char* key, void* value, struct avl_node** insert_or_exist_node)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    while (*new_node)
    {
        int cmp_ret = strcmp(key, (*new_node)->key_str);

        parent = *new_node;

        if (cmp_ret < 0)
        {
            new_node = &((*new_node)->avl_child[0]);
        }
        else if (cmp_ret > 0)
        {
            new_node = &((*new_node)->avl_child[1]);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_str = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    *insert_or_exist_node = node;

    return true;
}

bool avl_tree_try_insert_user(struct avl_tree* tree, void* key, void* value, struct avl_node** insert_or_exist_node)
{
    struct avl_node* node;
    struct avl_node** new_node = &(tree->root);
    struct avl_node* parent = 0;

    if (tree->key_cmp)
    {
        while (*new_node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, (*new_node)->key_user);

            parent = *new_node;

            if (cmp_ret < 0)
            {
                new_node = &((*new_node)->avl_child[0]);
            }
            else if (cmp_ret > 0)
            {
                new_node = &((*new_node)->avl_child[1]);
            }
            else
            {
                *insert_or_exist_node = (*new_node);
                return false;
            }
        }
    }
    else
    {
        while (*new_node)
        {
            parent = *new_node;

            if (key < (*new_node)->key_user)
            {
                new_node = &((*new_node)->avl_child[0]);
            }
            else if (key > (*new_node)->key_user)
            {
                new_node = &((*new_node)->avl_child[1]);
            }
            else
            {
                *insert_or_exist_node = (*new_node);
                return false;
            }
        }
    }

    node = (struct avl_node*)memory_unit_alloc(_get_avl_node_unit(), 1024);

    node->key_user = key;
    node->value = value;

    _avl_link_node(tree, node, parent, new_node);
    _avl_balance(tree, node);

    *insert_or_exist_node = node;

    return true;
}

struct avl_node* avl_tree_find_int(struct avl_tree* tree, size_t key)
{
    struct avl_node* node = tree->root;

    while (node)
    {
        if (key < node->key_int)
        {
            node = node->avl_child[0];
        }
        else if (key > node->key_int)
        {
            node = node->avl_child[1];
        }
        else
            return node;
    }

    return 0;
}

struct avl_node* avl_tree_find_int64(struct avl_tree* tree, unsigned long long key)
{
    struct avl_node* node = tree->root;

    while (node)
    {
        if (key < node->key_int64)
        {
            node = node->avl_child[0];
        }
        else if (key > node->key_int64)
        {
            node = node->avl_child[1];
        }
        else
            return node;
    }

    return 0;
}

struct avl_node* avl_tree_find_str(struct avl_tree* tree, const char* key)
{
    struct avl_node* node = tree->root;

    while (node)
    {
        int cmp_ret = strcmp(key, node->key_str);

        if (cmp_ret < 0)
        {
            node = node->avl_child[0];
        }
        else if (cmp_ret > 0)
        {
            node = node->avl_child[1];
        }
        else
            return node;
    }

    return 0;
}

struct avl_node* avl_tree_find_user(struct avl_tree* tree, void* key)
{
    struct avl_node* node = tree->root;

    if (tree->key_cmp)
    {
        while (node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, node->key_user);

            if (cmp_ret < 0)
            {
                node = node->avl_child[0];
            }
            else if (cmp_ret > 0)
            {
                node = node->avl_child[1];
            }
            else
                return node;
        }
    }
    else
    {
        while (node)
        {
            if (key < node->key_user)
            {
                node = node->avl_child[0];
            }
            else if (key > node->key_user)
            {
                node = node->avl_child[1];
            }
            else
                return node;
        }
    }

    return 0;
}

size_t avl_node_key_int( struct avl_node* node )
{
    return node->key_int;
}

unsigned long long avl_node_key_int64( struct avl_node* node )
{
    return node->key_int64;
}

const char* avl_node_key_str( struct avl_node* node )
{
    return node->key_str;
}

void* avl_node_key_user( struct avl_node* node )
{
    return node->key_user;
}

void* avl_node_value( struct avl_node* node )
{
    return node->value;
}

struct avl_node *avl_first(const struct avl_tree *tree)
{
    return tree->head;
}

struct avl_node *avl_last(const struct avl_tree *tree)
{
    return tree->tail;
}

struct avl_node *avl_next(const struct avl_node *node)
{
    return node->list_next;
}

struct avl_node *avl_prev(const struct avl_node *node)
{
    return node->list_prev;
}

struct avl_tree* create_avl_tree(user_key_cmp cmp_func)
{
    struct avl_tree* tree = (struct avl_tree*)memory_unit_alloc(_get_avl_tree_unit(), 256);
    tree->root = 0;
    tree->size = 0;
    tree->head = 0;
    tree->tail = 0;
    tree->key_cmp = cmp_func;

    return tree;
}

void destroy_avl_tree(struct avl_tree* tree)
{
    struct avl_node* node = tree->head;

    HMEMORYUNIT node_unit = _get_avl_node_unit();
    while (node)
    {
        memory_unit_free(node_unit, node);
        node = node->list_next;
    }

    memory_unit_free(_get_avl_tree_unit(), tree);
}

void avl_tree_clear(struct avl_tree* tree)
{
    struct avl_node* node = tree->head;

    HMEMORYUNIT node_unit = _get_avl_node_unit();
    while (node)
    {
        memory_unit_free(node_unit, node);
        node = node->list_next;
    }

    tree->root = 0;
    tree->size = 0;
    tree->head = 0;
    tree->tail = 0;
}

size_t avl_tree_size(struct avl_tree* root)
{
    return root->size;
}

void avl_node_set_value(struct avl_node* node, void* new_value)
{
    node->value = new_value;
}