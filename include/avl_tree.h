#pragma once
#include "./type_def.h"
#ifdef  __cplusplus
extern "C" {
#endif

typedef struct avl_node* HAVLNODE;
typedef struct avl_tree* HAVLTREE;
typedef const struct avl_node* CONST_HAVLNODE;
typedef const struct avl_tree* CONST_HAVLTREE;

extern HAVLTREE (create_avl_tree)(user_key_cmp cmp_func);
extern void (destroy_avl_tree)(HAVLTREE tree);

extern HAVLNODE (avl_tree_insert_int)(HAVLTREE tree, size_t key, void* value);
extern bool (avl_tree_try_insert_int)(HAVLTREE tree, size_t key, void* value, HAVLNODE* insert_or_exist_node);
extern HAVLNODE (avl_tree_find_int)(HAVLTREE tree, size_t key);
extern size_t (avl_node_key_int)(HAVLNODE node);

extern HAVLNODE (avl_tree_insert_int64)(HAVLTREE tree, unsigned long long key, void* value);
extern bool (avl_tree_try_insert_int64)(HAVLTREE tree, unsigned long long key, void* value, HAVLNODE* insert_or_exist_node);
extern HAVLNODE (avl_tree_find_int64)(HAVLTREE tree, unsigned long long key);
extern unsigned long long (avl_node_key_int64)(HAVLNODE node);

extern HAVLNODE (avl_tree_insert_str)(HAVLTREE tree, const char* key, void* value);
extern bool (avl_tree_try_insert_str)(HAVLTREE tree, const char* key, void* value, HAVLNODE* insert_or_exist_node);
extern HAVLNODE (avl_tree_find_str)(HAVLTREE tree, const char* key);
extern const char* (avl_node_key_str)(HAVLNODE node);

extern HAVLNODE (avl_tree_insert_user)(HAVLTREE tree, void* key, void* value);
extern bool (avl_tree_try_insert_user)(HAVLTREE tree, void* key, void* value, HAVLNODE* insert_or_exist_node);
extern HAVLNODE (avl_tree_find_user)(HAVLTREE tree, void* key);
extern void* (avl_node_key_user)(HAVLNODE node);

extern void* (avl_node_value)(HAVLNODE node);
extern void (avl_node_set_value)(HAVLNODE node, void* new_value);

extern void (avl_tree_erase)(HAVLTREE tree, HAVLNODE node);
extern void (avl_tree_clear)(HAVLTREE tree);

extern HAVLNODE (avl_first)(CONST_HAVLTREE tree);
extern HAVLNODE (avl_last)(CONST_HAVLTREE tree);
extern HAVLNODE (avl_next)(CONST_HAVLNODE node);
extern HAVLNODE (avl_prev)(CONST_HAVLNODE node);


extern size_t (avl_tree_size)(HAVLTREE tree);

#ifdef  __cplusplus
}
#endif