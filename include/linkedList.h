#ifndef LINKED_LIST
#define LINKED_LIST

#include <pthread.h>

struct list_node{
  struct list_node *next;
  struct list_node *prev;

  int priority;

  void *val;
};


struct list{
  pthread_mutex_t mut;

  struct list_node *root;
  struct list_node *tail;

  int size;
};


void ll_initialize(struct list *l);
struct list_node *ll_insert_val(struct list *l, void *val);
struct list_node *ll_search_node(struct list *l, void *val, size_t cmp_sz);
void ll_remove_val(struct list *l, void *val, size_t cmp_sz);
void *ll_remove_root(struct list *l);
struct list_node *ll_remove_last_node(struct list *l);
struct list_node *ll_pq_enqueue(struct list *l, void *val, int priority);
void *ll_pq_dequeue(struct list *l);



#endif
