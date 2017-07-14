#include "../include/rb_tree.h"
#include "../include/memory_pool.h"
#include <string.h>

#pragma warning( push )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4706 )
#define	RB_RED		    0
#define	RB_BLACK	    1
#define rb_color(r)   ((r)->rb_color)
#define rb_is_red(r)    (!rb_color(r))
#define rb_is_black(r)  rb_color(r)

struct rb_node
{
    struct rb_node* rb_right;
    struct rb_node* rb_left;
    struct rb_node* rb_parent;
    int             rb_color;

    struct rb_node* list_prev;
    struct rb_node* list_next;

    union
    {
        size_t              key_int;
        const char*         key_str;
        void*               key_user;
        unsigned long long  key_int64;
    };

    union
    {
        void* value;
        size_t value_int;
        unsigned long long value_int64;
    };
};

struct rb_tree
{
    struct rb_node*     root;
    struct rb_node*     head;
    struct rb_node*     tail;
    size_t              size;

    user_key_cmp        key_cmp;
};

static HMEMORYUNIT _get_rb_tree_unit(void)
{
    static HMEMORYUNIT tree_unit = 0;

    if (tree_unit)
    {
        return tree_unit;
    }

    tree_unit = create_memory_unit(sizeof(struct rb_tree));

    return tree_unit;
}

static HMEMORYUNIT _get_rb_node_unit(void)
{
    static HMEMORYUNIT node_unit = 0;

    if (node_unit)
    {
        return node_unit;
    }

    node_unit = create_memory_unit(sizeof(struct rb_node));

    return node_unit;
}

static __inline void _rb_link_node(struct rb_tree* root, struct rb_node * node, struct rb_node * parent,
struct rb_node ** rb_link)
{
    node->rb_color = RB_RED;
    node->rb_parent = parent;
    node->rb_left = node->rb_right = 0;

    *rb_link = node;

    if (parent)
    {
        if (&(parent->rb_left) == rb_link)
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

static __inline void _rb_erase_list(struct rb_tree* root, struct rb_node* node)
{
    if (node->list_prev)
    {
        node->list_prev->list_next = node->list_next;
    }
    else
    {
        root->head = node->list_next;
    }

    if (node->list_next)
    {
        node->list_next->list_prev = node->list_prev;
    }
    else
    {
        root->tail = node->list_prev;
    }

    --root->size;
}

static void _rb_rotate_left(struct rb_node *node, struct rb_tree *root)
{
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = node->rb_parent;

    if ((node->rb_right = right->rb_left))
        right->rb_left->rb_parent = node;

    right->rb_left = node;

    right->rb_parent = parent;

    if (parent)
    {
        if (node == parent->rb_left)
            parent->rb_left = right;
        else
            parent->rb_right = right;
    }
    else
        root->root = right;

    node->rb_parent = right;
}

static void _rb_rotate_right(struct rb_node *node, struct rb_tree *root)
{
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = node->rb_parent;

    if ((node->rb_left = left->rb_right))
        left->rb_right->rb_parent = node;

    left->rb_right = node;

    left->rb_parent = parent;

    if (parent)
    {
        if (node == parent->rb_right)
            parent->rb_right = left;
        else
            parent->rb_left = left;
    }
    else
        root->root = left;
    node->rb_parent = left;
}

void _rb_insert_balance(struct rb_node *node, struct rb_tree *root)
{
    struct rb_node *parent, *gparent;

    while ((parent = node->rb_parent) && rb_is_red(parent))
    {
        gparent = parent->rb_parent;

        if (parent == gparent->rb_left)
        {
            {
                register struct rb_node *uncle = gparent->rb_right;
                if (uncle && rb_is_red(uncle))
                {
                    uncle->rb_color = RB_BLACK;
                    parent->rb_color = RB_BLACK;
                    gparent->rb_color = RB_RED;

                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_right == node)
            {
                register struct rb_node *tmp;
                _rb_rotate_left(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->rb_color = RB_BLACK;
            gparent->rb_color = RB_RED;

            _rb_rotate_right(gparent, root);
        } else {
            {
                register struct rb_node *uncle = gparent->rb_left;
                if (uncle && rb_is_red(uncle))
                {
                    uncle->rb_color = RB_BLACK;
                    parent->rb_color = RB_BLACK;
                    gparent->rb_color = RB_RED;

                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_left == node)
            {
                register struct rb_node *tmp;
                _rb_rotate_right(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->rb_color = RB_BLACK;
            gparent->rb_color = RB_RED;

            _rb_rotate_left(gparent, root);
        }
    }

    root->root->rb_color = RB_BLACK;
}

static void _rb_erase_balance(struct rb_node *node, struct rb_node *parent,
struct rb_tree *root)
{
    struct rb_node *other;

    while ((!node || rb_is_black(node)) && node != root->root)
    {
        if (parent->rb_left == node)
        {
            other = parent->rb_right;
            if (rb_is_red(other))
            {
                other->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;
                _rb_rotate_left(parent, root);
                other = parent->rb_right;
            }
            if ((!other->rb_left || rb_is_black(other->rb_left)) &&
                (!other->rb_right || rb_is_black(other->rb_right)))
            {
                other->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            }
            else
            {
                if (!other->rb_right || rb_is_black(other->rb_right))
                {
                    other->rb_left->rb_color = RB_BLACK;
                    other->rb_color = RB_RED;
                    _rb_rotate_right(other, root);
                    other = parent->rb_right;
                }
                other->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                other->rb_right->rb_color = RB_BLACK;

                _rb_rotate_left(parent, root);
                node = root->root;
                break;
            }
        }
        else
        {
            other = parent->rb_left;
            if (rb_is_red(other))
            {
                other->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;

                _rb_rotate_right(parent, root);
                other = parent->rb_left;
            }
            if ((!other->rb_left || rb_is_black(other->rb_left)) &&
                (!other->rb_right || rb_is_black(other->rb_right)))
            {
                other->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            }
            else
            {
                if (!other->rb_left || rb_is_black(other->rb_left))
                {
                    other->rb_right->rb_color = RB_BLACK;
                    other->rb_color = RB_RED;
                    _rb_rotate_left(other, root);
                    other = parent->rb_left;
                }
                other->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                other->rb_left->rb_color = RB_BLACK;
                _rb_rotate_right(parent, root);
                node = root->root;
                break;
            }
        }
    }
    if (node)
        node->rb_color = RB_BLACK;
}

void rb_tree_erase(struct rb_tree *root, struct rb_node *node)
{
    struct rb_node *child, *parent, *del;
    int color;

    if (node)
    {
        del = node;

        if (!node->rb_left)
            child = node->rb_right;
        else if (!node->rb_right)
            child = node->rb_left;
        else
        {
            struct rb_node *old = node, *left;

            node = node->rb_right;
            while ((left = node->rb_left) != 0)
                node = left;

            if (old->rb_parent) {
                if (old->rb_parent->rb_left == old)
                    old->rb_parent->rb_left = node;
                else
                    old->rb_parent->rb_right = node;
            } else
                root->root = node;

            child = node->rb_right;
            parent = node->rb_parent;
            color = rb_color(node);

            if (parent == old) {
                parent = node;
            } else {
                if (child)
                    child->rb_parent = parent;

                parent->rb_left = child;

                node->rb_right = old->rb_right;
                old->rb_right->rb_parent = node;
            }

            node->rb_parent = old->rb_parent;
            node->rb_color = old->rb_color;
            node->rb_left = old->rb_left;
            old->rb_left->rb_parent = node;

            goto color;
        }

        parent = node->rb_parent;
        color = rb_color(node);

        if (child)
            child->rb_parent = parent;

        if (parent)
        {
            if (parent->rb_left == node)
                parent->rb_left = child;
            else
                parent->rb_right = child;
        }
        else
            root->root = child;

color:
        if (color == RB_BLACK)
            _rb_erase_balance(child, parent, root);
        _rb_erase_list(root, del);
        memory_unit_free(_get_rb_node_unit(), del);
    }
}

struct rb_node *rb_first(const struct rb_tree *tree)
{
    return tree->head;
}

struct rb_node *rb_last(const struct rb_tree *tree)
{
    return tree->tail;
}

struct rb_node *rb_next(const struct rb_node *node)
{
    return node->list_next;
}

struct rb_node *rb_prev(const struct rb_node *node)
{
    return node->list_prev;
}

size_t rb_tree_size(struct rb_tree* root)
{
    return root->size;
}

struct rb_tree* create_rb_tree(user_key_cmp cmp_func)
{
    struct rb_tree* tree = (struct rb_tree*)memory_unit_alloc(_get_rb_tree_unit(), 256);
    tree->root = 0;
    tree->size = 0;
    tree->head = 0;
    tree->tail = 0;
    tree->key_cmp = cmp_func;

    return tree;
}

void destroy_rb_tree(struct rb_tree* tree)
{
    struct rb_node* node = tree->head;

    HMEMORYUNIT node_unit = _get_rb_node_unit();
    while (node)
    {
        memory_unit_free(node_unit, node);
        node = node->list_next;
    }

    memory_unit_free(_get_rb_tree_unit(), tree);
}

void rb_tree_clear(struct rb_tree* tree)
{
    struct rb_node* node = tree->head;

    HMEMORYUNIT node_unit = _get_rb_node_unit();
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

struct rb_node* rb_tree_insert_int(struct rb_tree* tree, size_t key, void* value)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (key > (*new_node)->key_int)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_int = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    return node;
}

bool rb_tree_try_insert_int(struct rb_tree* tree, size_t key, void* value, struct rb_node** insert_or_exist_node)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (key > (*new_node)->key_int)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_int = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    *insert_or_exist_node = node;

    return true;
}

struct rb_node* rb_tree_insert_int64(struct rb_tree* tree, unsigned long long key, void* value)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int64)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (key > (*new_node)->key_int64)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_int64 = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    return node;
}

bool rb_tree_try_insert_int64(struct rb_tree* tree, unsigned long long key, void* value, struct rb_node** insert_or_exist_node)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        parent = *new_node;

        if (key < (*new_node)->key_int64)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (key > (*new_node)->key_int64)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_int64 = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    *insert_or_exist_node = node;

    return true;
}

struct rb_node* rb_tree_insert_str(struct rb_tree* tree, const char* key, void* value)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        int cmp_ret = strcmp(key, (*new_node)->key_str);

        parent = *new_node;

        if (cmp_ret < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (cmp_ret > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            (*new_node)->key_str = key;
            (*new_node)->value = value;
            return (*new_node);
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_str = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    return node;
}

bool rb_tree_try_insert_str(struct rb_tree* tree, const char* key, void* value, struct rb_node** insert_or_exist_node)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    while (*new_node)
    {
        int cmp_ret = strcmp(key, (*new_node)->key_str);

        parent = *new_node;

        if (cmp_ret < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (cmp_ret > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            *insert_or_exist_node = (*new_node);
            return false;
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_str = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    *insert_or_exist_node = node;

    return true;
}

struct rb_node* rb_tree_insert_user(struct rb_tree* tree, void* key, void* value)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    if (tree->key_cmp)
    {
        while (*new_node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, (*new_node)->key_user);

            parent = *new_node;

            if (cmp_ret < 0)
            {
                new_node = &((*new_node)->rb_left);
            }
            else if (cmp_ret > 0)
            {
                new_node = &((*new_node)->rb_right);
            }
            else
            {
                (*new_node)->key_str = key;
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
                new_node = &((*new_node)->rb_left);
            }
            else if (key > (*new_node)->key_user)
            {
                new_node = &((*new_node)->rb_right);
            }
            else
            {
                (*new_node)->key_str = key;
                (*new_node)->value = value;
                return (*new_node);
            }
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_user = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    return node;
}

bool rb_tree_try_insert_user(struct rb_tree* tree, void* key, void* value, struct rb_node** insert_or_exist_node)
{
    struct rb_node* node;
    struct rb_node** new_node = &(tree->root);
    struct rb_node* parent = 0;

    if (tree->key_cmp)
    {
        while (*new_node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, (*new_node)->key_user);

            parent = *new_node;

            if (cmp_ret < 0)
            {
                new_node = &((*new_node)->rb_left);
            }
            else if (cmp_ret > 0)
            {
                new_node = &((*new_node)->rb_right);
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
                new_node = &((*new_node)->rb_left);
            }
            else if (key > (*new_node)->key_user)
            {
                new_node = &((*new_node)->rb_right);
            }
            else
            {
                *insert_or_exist_node = (*new_node);
                return false;
            }
        }
    }

    node = (struct rb_node*)memory_unit_alloc(_get_rb_node_unit(), 4*1024);

    node->key_user = key;
    node->value = value;

    _rb_link_node(tree, node, parent, new_node);
    _rb_insert_balance(node, tree);

    *insert_or_exist_node = node;

    return true;
}

struct rb_node* rb_tree_find_int(struct rb_tree* tree, size_t key)
{
    struct rb_node* node = tree->root;

    while (node)
    {
        if (key < node->key_int)
        {
            node = node->rb_left;
        }
        else if (key > node->key_int)
        {
            node = node->rb_right;
        }
        else
            return node;
    }

    return 0;
}

struct rb_node* rb_tree_find_int_near_large(struct rb_tree* tree, size_t key)
{
    struct rb_node* node = tree->root;
    struct rb_node* nearby_node = node;

    while (node)
    {
        nearby_node = node;

        if (key < node->key_int)
        {
            node = node->rb_left;
        }
        else if (key > node->key_int)
        {
            node = node->rb_right;
        }
        else
            return node;
    }

    if (nearby_node->key_int < key)
    {
        if (nearby_node->list_next)
        {
            nearby_node = nearby_node->list_next;
        }
    }

    return nearby_node;
}

struct rb_node* rb_tree_find_int64(struct rb_tree* tree, unsigned long long key)
{
    struct rb_node* node = tree->root;

    while (node)
    {
        if (key < node->key_int64)
        {
            node = node->rb_left;
        }
        else if (key > node->key_int64)
        {
            node = node->rb_right;
        }
        else
            return node;
    }

    return 0;
}

struct rb_node* rb_tree_find_str(struct rb_tree* tree, const char* key)
{
    struct rb_node* node = tree->root;

    while (node)
    {
        int cmp_ret = strcmp(key, node->key_str);

        if (cmp_ret < 0)
        {
            node = node->rb_left;
        }
        else if (cmp_ret > 0)
        {
            node = node->rb_right;
        }
        else
            return node;
    }

    return 0;
}

struct rb_node* rb_tree_find_user(struct rb_tree* tree, void* key)
{
    struct rb_node* node = tree->root;

    if (tree->key_cmp)
    {
        while (node)
        {
            ptrdiff_t cmp_ret = tree->key_cmp(key, node->key_user);

            if (cmp_ret < 0)
            {
                node = node->rb_left;
            }
            else if (cmp_ret > 0)
            {
                node = node->rb_right;
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
                node = node->rb_left;
            }
            else if (key > node->key_user)
            {
                node = node->rb_right;
            }
            else
                return node;
        }
    }

    return 0;
}

size_t rb_node_key_int( struct rb_node* node )
{
    return node->key_int;
}

unsigned long long rb_node_key_int64( struct rb_node* node )
{
    return node->key_int64;
}

const char* rb_node_key_str( struct rb_node* node )
{
    return node->key_str;
}

void* rb_node_key_user( struct rb_node* node )
{
    return node->key_user;
}

void* rb_node_value( struct rb_node* node )
{
    return node->value;
}

void rb_node_set_value(struct rb_node* node, void* new_value)
{
    node->value = new_value;
}

size_t rb_node_value_int( struct rb_node* node )
{
    return node->value_int;
}

void rb_node_set_value_int(struct rb_node* node, size_t value_int)
{
    node->value_int = value_int;
}

unsigned long long rb_node_value_int64( struct rb_node* node )
{
    return node->value_int64;
}

void rb_node_set_value_int64(struct rb_node* node, unsigned long long value_int64)
{
    node->value_int64 = value_int64;
}

user_key_cmp rb_tree_cmp_func_ptr(struct rb_tree* tree)
{
    return tree->key_cmp;
}

#pragma warning( pop )
