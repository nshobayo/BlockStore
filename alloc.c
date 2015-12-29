#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "alloc.h"
#include <stdio.h>


#define ALIGNMENT 8 // must be power of 2
typedef unsigned long address_t; // must be same size as a pointer

struct m_header {

  int size ; 
  struct m_header* prev;
  struct m_header* next;
  void * data;
  unsigned int occupied;
};


/* Align *p_addr to the next multiple of 8. Reduce *p_size accordingly.
 */
void addr_align(void **p_addr, int *p_size){
  assert(sizeof(void *) == sizeof(address_t));
  unsigned int delta = (address_t) *p_addr & (ALIGNMENT - 1);
  if (delta == 0) {
    return;
  }
  *p_addr = (char *) *p_addr + (ALIGNMENT - delta);
  *p_size -= ALIGNMENT - delta;
}



int alloc_init(void * memarea, int size) {
  size = size - sizeof(struct m_header);
  void * data = (void *)(((char *)memarea)+ sizeof(struct m_header));
  addr_align(&data, &size);

  if( size <= 0){
    return 1;
  }

  *(struct m_header*) memarea = (struct m_header){size,NULL, NULL, data,0}; 
  
  return 0;
}

void * alloc_get(void * memarea, int size) {
  size = size + (8 - (size%8));
  
  struct m_header *curHeader = (struct m_header *)memarea;
  while(curHeader != NULL){
    if(curHeader-> occupied ==0 && curHeader->size>= (size+ sizeof(struct m_header))){
      struct m_header *newHeader = (struct m_header *)(((char *)curHeader->data)+size);
      *(struct m_header *)newHeader = *curHeader;
      newHeader->prev = curHeader;

      int leftoverSize = curHeader->size - size - sizeof(struct m_header);
      void *data = (newHeader+1);
      addr_align(&data, &leftoverSize);
      if(leftoverSize <= 0){
	newHeader->size = 0; 
	newHeader->data = NULL;
      }
      else{
	newHeader->size = leftoverSize; 
	newHeader->data = data;
      }
      curHeader-> next = newHeader; 
      curHeader->size = size;
      curHeader->occupied = 1; 
      if(newHeader ->next != NULL){
	curHeader->next->next->prev = curHeader->next;
      }
      return curHeader->data;      
    }
    curHeader = curHeader->next;
  }
  return 0;
}

void alloc_release(void * memarea, void * mem) {
  if( mem !=0){   
    mem = (void *)((char *)mem - sizeof(struct m_header));
    struct m_header *myHeader = (struct m_header *)mem;
    struct m_header *parent;
    struct m_header *next;
    int offsetSize = 0;
    if(myHeader->prev !=NULL || myHeader->next!= NULL){
      if(myHeader->prev != NULL){
	parent = myHeader->prev;
	if(parent->occupied == 0){
	  void *fauxData = (void *)(((char *)myHeader)+ sizeof(struct m_header));
	  addr_align(&(fauxData), &offsetSize);
	  
	  parent->size = parent->size +  sizeof(struct m_header) - offsetSize + myHeader->size;
	  parent->next = myHeader->next;
	  (myHeader-> next)->prev = parent; 
	  myHeader = parent;
	}
	else{
	  myHeader->occupied = 0;
	}
      }
      if(myHeader->next != NULL){
	next = myHeader->next;
	if(next->occupied == 0){
	  offsetSize = 0; 
	  void *fauxData = (void *)( ((char *)myHeader)+ sizeof(struct m_header));
	  addr_align(&(fauxData), &offsetSize);
	  
	  myHeader->size = myHeader->size - offsetSize  +  sizeof(struct m_header) + next->size;
	  myHeader->next = next->next;
	  if(next->next != NULL){
	    (next->next)->prev = myHeader;
	  }
	  myHeader->occupied =0;
	}
	else {
	  myHeader->occupied = 0;
	}
      }
    }
    else {
      myHeader->occupied = 0;
    }
  }
}
void * alloc_resize(void * memarea, void * mem, int size) {

  if(mem == 0){
    return alloc_get(memarea, 0);
  }
  else{
    mem = (void *)((char *)mem - sizeof(struct m_header));
    
    struct m_header *curMem  = (struct m_header *)mem; 
    struct m_header *curHeader  = (struct m_header *)mem;
    struct m_header *next = NULL;
    struct m_header *prev = NULL;
    
    
    if(size<= curMem->size){
      if(curMem->size == size){
	return mem;
      }
      if(curMem->size - 8< size){
	return mem;
      }
      else if(curMem->next != NULL){
	if (curMem->next->occupied ==0){
	  int bigSize = (curMem->next)->size +  (curMem ->size) - (size+(8-(size%8)));
	  void *expand = (void *)(((char *)(mem))+sizeof(struct m_header)+(size+(8-(size%8))));
	  void * nxt = (void *) curMem->next;
	  memmove(expand, nxt, (curMem ->next)->size);
	  if(((struct m_header *)expand)->next  != NULL){ 
	    ((struct m_header *)expand)->next->prev = ((struct m_header *)expand);
	  }
	  struct m_header *xpnd = (struct m_header *) expand;
	  curMem ->next = expand;
	  curMem ->size = (size + (8-(size%8 )));
	  xpnd -> size = (bigSize);
	  xpnd -> data = (void *)(((char *)(expand)) + sizeof(struct m_header));
	  return (void *)(curMem->data);
	}
	else{
	  void *newLocation = (void *)alloc_get(memarea, size);
	  if( newLocation == 0){
	    return 0;
	  }
	  memmove(newLocation, curMem->data, size);
	  alloc_release(memarea, (((struct m_header *)mem)->data) );
	return newLocation;
	}   
      }
      else{
	void *newLocation = (void *)alloc_get(memarea, size);
	if( newLocation == 0){
	  return 0;
	}
	memmove(newLocation, curMem->data, size);
	alloc_release(memarea, ((struct m_header *)mem)->data );
	return newLocation;
      }
    }
    else{
      size = size + (8-(size %8)); 
      if(curHeader->next != NULL || curHeader->prev != NULL){
	
	if(curHeader-> next != NULL)
	  next = curHeader->next;
	if( next->occupied == 0 && (next->size +curHeader->size)>= size){
	  int oldSize = curHeader->size;
	  curHeader -> size = size;
	  struct m_header *newHeader = (struct m_header *)(((char *)(curHeader->data)) +size);
	  int newSize = ((next->size)-((curHeader->size) - oldSize));
	  *newHeader  = (struct m_header ) {newSize, next->prev, next->next,  (void *)(((char *)newHeader )+ (sizeof(struct m_header))), 0}; 
	  newHeader->next->prev = newHeader;
	  curHeader->next = newHeader; 
	  return (void *)(curHeader->data); 
	}
	if(curHeader->prev != NULL){
	  prev = curHeader->prev;
	  if(prev->occupied == 0 && (prev->size +curHeader->size)>= size){
	    struct m_header *nextNext = curHeader->next;
	    int oldSize = curHeader->size;	  
	    memmove(prev->data, curHeader->data,(curHeader->size));
	    
	    struct m_header *newHeader = (struct m_header *)(((char *)(prev->data)) +size);
	    *newHeader  = (struct m_header) {( prev->size+oldSize - size), prev, nextNext, (void *)(((char *)newHeader)+ (sizeof(struct m_header))), 0};
	    if( nextNext != NULL){
	      nextNext->prev = newHeader;
	    }
	    prev->size = size;
	    prev -> next = newHeader;
	    prev->occupied = 1;
	    return (void *) (prev->data) ;
	  }
	  
	  if(curHeader ->prev !=NULL && curHeader ->next != NULL){
	    prev = curHeader->prev;
	    next = curHeader->next;
	  
	    if(prev->occupied == 0 && next->occupied == 0 && (prev->size + next->size + curHeader->size + sizeof(struct m_header))>= size ){
	      int leftover = prev->size +next->size + curHeader->size + sizeof(struct m_header) - size; 
	      memmove(prev->data, curHeader->data,(curHeader->size));
	    
	      struct m_header *nextNext = (curHeader->next)->next;	    
	      struct m_header *newHeader = (struct m_header *)(((char *)(prev->data)) +size);
	      *newHeader  = (struct m_header) {leftover, prev, nextNext, (void *)(((char *)newHeader)+ (sizeof(struct m_header))), 0};
	      
	      if( nextNext != NULL){
		nextNext->prev = newHeader;
	      }
	      
	      prev->size = size;
	      prev -> next = newHeader;
	      prev->occupied = 1;
	      return (void *) (prev->data);
	    }
	    else{
	      void  *newLocation = (void *)alloc_get(memarea, size);
	      if ( newLocation == 0){
		return 0;
	      }
	      memmove(newLocation, curHeader->data, size);
	      alloc_release(memarea, ((struct m_header *)mem)->data );
	      return newLocation; 
	    }
	  }
	}
      }      
    }
  }
  return 0;
}
