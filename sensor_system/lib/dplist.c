#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)

        
/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
  dplist_node_t * prev, * next;
  void * element;
};

struct dplist {
  dplist_node_t * head;
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
  while ( (*list)->head != NULL)  
  {
    dpl_remove_at_index(*list, 0, free_element);
  }
  free(*list);
  *list = NULL;
     // add your code here
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{ 
  dplist_node_t * ref_at_index, * list_node;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  list_node = malloc(sizeof(dplist_node_t));
  if(insert_copy == true)
  {
    DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
    list_node ->element = (list -> element_copy)(element);
  }
  else
  {
    DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
    list_node->element = element;
  }
  if (list->head == NULL)  
  { 
    list_node -> prev = NULL;
    list_node -> next = NULL;
    list -> head = list_node;
  } 
  else if(index <= 0)
  {
    list_node -> prev = NULL;
    list_node -> next = list->head;
    list -> head -> prev = list_node;
    list -> head = list_node;
  }
  else 
  {
    ref_at_index = dpl_get_reference_at_index(list, index);  
    assert( ref_at_index != NULL);
    // pointer drawing breakpoint
    if (index < dpl_size(list))
    { // covers case 4
      list_node->prev = ref_at_index->prev;
      list_node->next = ref_at_index;
      ref_at_index->prev->next = list_node;
      ref_at_index->prev = list_node;
      // pointer drawing breakpoint
    } 
    else
    { // covers case 3 
      assert(ref_at_index->next == NULL);
      list_node->next = NULL;
      list_node->prev = ref_at_index;
      ref_at_index->next = list_node;    
      // pointer drawing breakpoint
    }
  } 
  return list;
  // add your code here
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
  dplist_node_t * ref_at_index, * dummy;
  if(list -> head == NULL)
  return list;
  ref_at_index = dpl_get_reference_at_index(list, index);
  if(ref_at_index == list -> head)
  {
    dummy = ref_at_index;
    list -> head = list -> head -> next;//case 1 index at the start
    if(list -> head != NULL)
    list -> head -> prev = NULL;
    //case 2 index at the start with only one node in the list
  }
  else if(ref_at_index -> next != NULL)
  {
    dummy = ref_at_index;
    ref_at_index -> next -> prev = ref_at_index -> prev;
    ref_at_index ->prev -> next = ref_at_index -> next;
    //case 2 index not at the end
  }
  else
  {
    dummy = ref_at_index;
    ref_at_index -> prev -> next = NULL;
    //case 3 index at the end 
  }
  if(free_element==true)
  (*(list -> element_free))(&(dummy -> element));
  free(dummy);  
  return list;     // add your code here

  
}

int dpl_size( dplist_t * list )
{
  if(list -> head == NULL)
  return 0;
  int size;
  size = 1;
  dplist_node_t * list_node;
  list_node = list -> head;
  while(list_node -> next != NULL)
  {
    size++;
    list_node = list_node -> next;
  }
  return size;
  // add your code here
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
  int count;
  count = 0;
  dplist_node_t * list_node;
  if(list -> head == NULL)
  return NULL; 
  list_node = list -> head;
  if(index <= 0)
  return list -> head;
  else if(index >= (dpl_size(list) - 1))
  return dpl_get_last_reference(list);
  else
  {
    while(1)
    {
      list_node = list_node -> next;
      count++;
      if(count == index)
      break;
    }
  }
  return list_node;
   // add your code here
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
   dplist_node_t * list_node;
   list_node = dpl_get_reference_at_index(list, index);
   return list_node ? list_node->element : NULL;
    // add your code here
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
  int index;
  index = 0;
  dplist_node_t * list_node;
  list_node = list -> head;
  while(list_node != NULL)
  {
    if((*(list -> element_compare))(list_node -> element, element) == 0) 
    return index;
    list_node = list_node -> next;
    index ++;
  }
  return -1;
  // add your code here
}

// HERE STARTS THE EXTRA SET OF OPERATORS //

// ---- list navigation operators ----//
  
dplist_node_t * dpl_get_first_reference( dplist_t * list )
{
  if(list -> head == NULL )
  return NULL;
  dplist_node_t * list_node;
  list_node = list -> head;
  return list_node;
    // add your code here
}

dplist_node_t * dpl_get_last_reference( dplist_t * list )
{
  if(list -> head == NULL )
  return NULL;
  dplist_node_t * list_node;
  list_node = list -> head;
  while (list_node -> next != NULL)
  {
    list_node = list_node -> next;
  }
  return list_node; // add your code here
}

dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference )
{
  if(list -> head == NULL )
  return NULL;
  dplist_node_t * list_node;
  if(reference == NULL)
  { 
    list_node = dpl_get_last_reference(list);
    return list_node;
  }
  for (list_node = list -> head; list_node != NULL; list_node = list_node -> next)
  {
    if (list_node == reference)
    return list_node -> next; 
  }
  return NULL;
  // add your code here
}

dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference )
{
  if(list -> head == NULL )
  return NULL;
  dplist_node_t * list_node;
  if(reference == NULL)
  { 
    list_node = dpl_get_last_reference(list);
    return list_node;
  }
  for (list_node = list -> head; list_node != NULL; list_node = list_node -> next)
  {
    if (list_node == reference)
    return list_node -> prev; 
  }
  return NULL;
   // add your code here
}

// ---- search & find operators ----//  
  
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
  if(list -> head == NULL )
  return NULL;
  dplist_node_t * list_node;
  if(reference == NULL)
  { 
    list_node = dpl_get_last_reference(list);
    return list_node -> element;
  }
  for (list_node = list -> head; list_node != NULL; list_node = list_node -> next)
  {
    if (list_node == reference)
    return list_node -> element; 
  }
  return NULL;  
  // add your code here
}

dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element )
{
  if(list -> head == NULL )
  return NULL;
  int index;
  index = dpl_get_index_of_element(list, element);
  if(index == -1)
  return NULL;
  else
  return dpl_get_reference_at_index(list, index);

  // add your code here
}

int dpl_get_index_of_reference( dplist_t * list, dplist_node_t * reference )
{
  if(list -> head == NULL )
  return -1;
  if(reference == NULL)
  return dpl_size(list) - 1;
  int index;
  index = 0;
  dplist_node_t * list_node;
  for (list_node = list -> head; list_node != NULL; list_node = list_node -> next, index++)
  {
    if (list_node == reference)
    return index; 
  }
  return -1;
  // add your code here
}
  
// ---- extra insert & remove operators ----//

dplist_t * dpl_insert_at_reference( dplist_t * list, void * element, dplist_node_t * reference, bool insert_copy )
{
  if(reference == NULL)
  {
   dpl_insert_at_index(list, element, dpl_size(list), insert_copy);
   return list;
  }
  int index;
  index = dpl_get_index_of_reference(list, reference);
  if(index == -1)
  return list;
  else
  {
    list = dpl_insert_at_index(list, element, index, insert_copy);
    return list;
  }
  // add your code here
}

dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy )
{
  int count;
  dplist_node_t * dummy;
  DPLIST_ERR_HANDLER((list==NULL),DPLIST_INVALID_ERROR);
  if(list->head == NULL) dpl_insert_at_index(list, element, 0, insert_copy);
  for(dummy = list->head, count = 0; dummy != NULL; dummy = dummy->next, count++)
  {
    int compare = (*(list->element_compare))(element,dummy->element);
    if(compare == 0) 
    return list;
    else if(compare == -1) 
    return dpl_insert_at_index(list, element, count, insert_copy);
  }
    return dpl_insert_at_index(list, element, dpl_size(list), insert_copy);
    // add your code here
}

dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element )
{
  if (list -> head == NULL) 
  return list;
  int index; 
  index = dpl_get_index_of_reference(list, reference);
  if(index == -1) 
  return list;
  else
  list = dpl_remove_at_index(list, index, free_element); 
  return list;
  // add your code here
}

dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element )
{
  int index;
  index = dpl_get_index_of_element(list, element);
  if(index == -1)
  return list;
  else
  list = dpl_remove_at_index(list, index, free_element); 
  return list;
  
  // add your code here
  
    // add your code here
}
  
// ---- you can add your extra operators here ----//



