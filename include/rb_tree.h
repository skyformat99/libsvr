#pragma once
#include <stddef.h>
#include "./type_def.h"
#ifdef  __cplusplus
extern "C" {
#endif

typedef struct rb_node* HRBNODE;
typedef struct rb_tree* HRBTREE;
typedef const struct rb_node* CONST_HRBNODE;
typedef const struct rb_tree* CONST_HRBTREE;

extern HRBTREE (create_rb_tree)(user_key_cmp cmp_func);
extern void (destroy_rb_tree)(HRBTREE tree);

extern HRBNODE (rb_tree_insert_int)(HRBTREE tree, size_t key, void* value);
extern bool (rb_tree_try_insert_int)(HRBTREE tree, size_t key, void* value, HRBNODE* insert_or_exist_node);
extern HRBNODE (rb_tree_find_int)(HRBTREE tree, size_t key);
extern size_t (rb_node_key_int)(HRBNODE node);

extern HRBNODE (rb_tree_insert_int64)(HRBTREE tree, unsigned long long key, void* value);
extern bool (rb_tree_try_insert_int64)(HRBTREE tree, unsigned long long key, void* value, HRBNODE* insert_or_exist_node);
extern HRBNODE (rb_tree_find_int64)(HRBTREE tree, unsigned long long key);
extern unsigned long long (rb_node_key_int64)(HRBNODE node);

extern HRBNODE (rb_tree_insert_str)(HRBTREE tree, const char* key, void* value);
extern bool (rb_tree_try_insert_str)(HRBTREE tree, const char* key, void* value, HRBNODE* insert_or_exist_node);
extern HRBNODE (rb_tree_find_str)(HRBTREE tree, const char* key);
extern const char* (rb_node_key_str)(HRBNODE node);

extern HRBNODE (rb_tree_insert_user)(HRBTREE tree, void* key, void* value);
extern bool (rb_tree_try_insert_user)(HRBTREE tree, void* key, void* value, HRBNODE* insert_or_exist_node);
extern HRBNODE (rb_tree_find_user)(HRBTREE tree, void* key);
extern void* (rb_node_key_user)(HRBNODE node);

extern void* (rb_node_value)(HRBNODE node);
extern void (rb_node_set_value)(HRBNODE node, void* new_value);

extern size_t (rb_node_value_int)(HRBNODE node);
extern void (rb_node_set_value_int)(HRBNODE node, size_t int_value);

extern unsigned long long (rb_node_value_int64)(HRBNODE node);
extern void (rb_node_set_value_int64)(HRBNODE node, unsigned long long int64_value);

extern void (rb_tree_erase)(HRBTREE tree, HRBNODE node);
extern void (rb_tree_clear)(HRBTREE tree);

extern user_key_cmp (rb_tree_cmp_func_ptr)(HRBTREE tree);

extern HRBNODE (rb_first)(CONST_HRBTREE tree);
extern HRBNODE (rb_last)(CONST_HRBTREE tree);
extern HRBNODE (rb_next)(CONST_HRBNODE node);
extern HRBNODE (rb_prev)(CONST_HRBNODE node);


extern size_t (rb_tree_size)(HRBTREE tree);

#ifdef  __cplusplus
}
#endif