#include <stdlib.h>
#include <string.h>
#include <linkedList.h>
#include <pthread.h>

#include <stdio.h>

void ll_initialize(struct list *l){
  l->root = NULL;
  l->size = 0;
  l->mut = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_unlock(&(l->mut));
}

struct list_node *ll_create_node(void * val){
  struct list_node *mk = (struct list_node *)malloc(sizeof(struct list_node));
  mk->next = NULL;
  mk->prev = NULL;
  mk->val = val;
  return mk;
}


struct list_node *ll_create_node_pq(void * val, int priority){
  struct list_node *mk = (struct list_node *)malloc(sizeof(struct list_node));
  mk->next = NULL;
  mk->prev = NULL;
  mk->val = val;
  mk->priority = priority;
  return mk;
}

struct list_node *ll_insert_node(struct list *l, struct list_node *ln){
  if(l->root == NULL){
    l->root = ln;
    l->tail = ln;
    l->size = 1;
  } else {
    l->root->prev = ln;
    ln->next = l->root;
    l->root = ln;
    l->size++;
  }

  return ln;
}

struct list_node *ll_insert_val(struct list *l, void *val){
  pthread_mutex_lock(&(l->mut));
  struct list_node *ret = ll_insert_node(l, ll_create_node(val));
  pthread_mutex_unlock(&(l->mut));

  return ret;
}


struct list_node *ll_list_search(struct list *l, void *val, size_t cmp_sz){
  struct list_node *mk = l->root;
  
  while(mk->next != NULL){
    if(memcmp(mk->val, val, cmp_sz) == 0)
      break;
    else
      mk = mk->next;    
  }

  return mk;
}


struct list_node *ll_search_node(struct list *l, void *val, size_t cmp_sz){
  pthread_mutex_lock(&(l->mut));

  struct list_node *mk;
  mk = ll_list_search(l, val, cmp_sz);

  mk = (memcmp(mk->val, val, cmp_sz) == 0)? mk : NULL;

  pthread_mutex_unlock(&(l->mut));

  return mk;
}



void *ll_search_value(struct list *l, void *val, size_t cmp_sz){
  pthread_mutex_lock(&(l->mut));

  struct list_node *mk;
  mk = ll_list_search(l, val, cmp_sz);

  void * ret = (memcmp(mk->val, val, cmp_sz) == 0)? mk->val : NULL;

  pthread_mutex_unlock(&(l->mut));

  return ret;
}


struct list_node *ll_disconnect_node(struct list *l, struct list_node *ln){
  if(l->root == ln)
    l->root = ln->next;
  if(ln->prev != NULL)
    ln->prev->next = ln->next;
  if(ln->next != NULL)
    ln->next->prev = ln->prev;
  if(l->tail == ln)
    l->tail = ln->prev;
  
  l->size--;
  
  return ln;
}


struct list_node *ll_remove_node(struct list *l, struct list_node *ln){
  struct list_node *ret;

  pthread_mutex_lock(&(l->mut));
  ret = ll_disconnect_node(l, ln);
  pthread_mutex_unlock(&(l->mut));
  
  return ret;
}


void ll_remove_val(struct list *l, void *val, size_t cmp_sz){
  pthread_mutex_lock(&(l->mut));
  struct list_node *mk;
  mk = ll_list_search(l, val, cmp_sz);

  if(memcmp(mk->val, val, cmp_sz) == 0)
    mk = ll_disconnect_node(l, mk);

  pthread_mutex_unlock(&(l->mut));

  void *ret = mk->val;

  free(mk);
}

void *ll_remove_root(struct list *l){

  pthread_mutex_lock(&(l->mut));
  struct list_node *mk;

  void *ret = NULL;
 
  mk = l->root;
  if(mk != NULL){
    ret = mk->val;
    ll_disconnect_node(l, mk);
  };

  pthread_mutex_unlock(&(l->mut));
  return ret;
}


struct list_node *ll_remove_last_node(struct list *l){
  pthread_mutex_lock(&(l->mut));
  struct list_node *ret = l->tail;

  if(ret != NULL)
    ll_disconnect_node(l, ret);

  pthread_mutex_unlock(&(l->mut));

  return ret;
}


struct list_node *ll_pq_insert(struct list *l, struct list_node *ln){

   if(l->root == NULL){
    l->root = ln;
    l->tail = ln;
    l->size = 1;
  } else {
     struct list_node *mk = l->root;

     while(mk->next != NULL && ln->priority < mk->next->priority){
       mk = mk->next;

     }

     
     
     //first element is prioritized over the new one
     if(mk->priority < ln->priority){
       //ASSERT
       if(mk->prev != NULL || l->root != mk){
	 return NULL;
       }
       ln->next = mk;
       l->root = ln;
       mk->prev = ln;
       ln->prev = NULL;
     } else {
       ln->next = mk->next;
       if(mk->next != NULL) mk->next->prev = ln;
       ln->prev = mk;
       mk->next = ln;
     }
       
     
     if(ln->next == NULL)
       l->tail = ln;

     
     l->size++;
  }

  return ln;

}


struct list_node *ll_pq_enqueue(struct list *l, void *val, int priority){
  pthread_mutex_lock(&(l->mut));

  struct list_node * ret = ll_pq_insert(l, ll_create_node_pq(val, priority));

  pthread_mutex_unlock(&(l->mut));

  return ret;
}



void *ll_pq_dequeue(struct list *l){
  void *ret = NULL;

  pthread_mutex_lock(&(l->mut));
  if(l->tail != NULL){
    ret = l->tail->val;
    ll_disconnect_node(l, l->tail);
  }
  pthread_mutex_unlock(&(l->mut));

  return ret;
}
