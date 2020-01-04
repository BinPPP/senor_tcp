#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>
#include "sbuffer.h"
#include "config.h"

/*
 * All data that can be stored in the sbuffer should be encapsulated in a
 * structure, this structure can then also hold extra info needed for your implementation
 */

pthread_mutex_t mutex;
pthread_mutex_t mutex_remove;
pthread_cond_t not_zero;
pthread_barrier_t reading_barrier;
struct sbuffer_data {
    sensor_data_t data;
};

typedef struct sbuffer_node {
  struct sbuffer_node * next;
  sbuffer_data_t element;
} sbuffer_node_t;

struct sbuffer {
  sbuffer_node_t * head;
  sbuffer_node_t * tail;
};	


int sbuffer_init(sbuffer_t ** buffer)
{
  *buffer = malloc(sizeof(sbuffer_t));
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;
  return SBUFFER_SUCCESS; 
}


int sbuffer_free(sbuffer_t ** buffer)
{
  sbuffer_node_t * dummy;
  while( (*buffer)->head != NULL )
  {
    sleep(1);
  }
  if ((buffer==NULL) || (*buffer==NULL)) 
  {
    return SBUFFER_FAILURE;
  } 
  while ( (*buffer)->head )
  {
    dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;
  return SBUFFER_SUCCESS;		
}


int sbuffer_remove(sbuffer_t * buffer,sensor_data_t * data)
{
  sbuffer_node_t * dummy;
  int ret;
  time_t now;
    struct timespec ts_timeout;

    ts_timeout.tv_sec=time(&now)+(time_t)5;
    ts_timeout.tv_nsec=0;
  if (buffer == NULL) return SBUFFER_FAILURE;
  pthread_mutex_lock(&mutex);
  while(buffer->head == NULL)
  {
    int result=pthread_cond_timedwait( &not_zero,&mutex, &ts_timeout);
    if(result==ETIMEDOUT)
    {
       pthread_mutex_unlock(&mutex);
       return SBUFFER_NO_DATA;
    }
  }
  pthread_mutex_unlock(&mutex);
  if (buffer->head == NULL) return SBUFFER_NO_DATA;
  *data = buffer->head->element.data;
  dummy = buffer->head;
  //printf("successfully read data sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data->id, data->value, (long int)data->ts);
  pthread_barrier_wait(&reading_barrier);
  ret=pthread_mutex_trylock(&mutex_remove);
  if(ret==0) {
    pthread_mutex_lock(&mutex);
    if (buffer->head == buffer->tail) // buffer has only one node
    {
      buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
      buffer->head = buffer->head->next;
    }
    free(dummy);
    pthread_mutex_unlock(&mutex);
    return SBUFFER_SUCCESS;
  }
  else
  {pthread_mutex_unlock(&mutex_remove);
    return SBUFFER_SUCCESS;
  }

}


int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data)
{
    pthread_mutex_lock(&mutex);
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL) return SBUFFER_FAILURE;
  dummy->element.data = *data;
  dummy->next = NULL;
  if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
  {
    buffer->head = buffer->tail = dummy;
  } 
  else // buffer not empty
  {
    buffer->tail->next = dummy;
    buffer->tail = buffer->tail->next; 
  }
  //printf("successfully insert data sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data->id, data->value, (long int)data->ts);
  pthread_mutex_unlock(&mutex);
  pthread_cond_broadcast(&not_zero);

  return SBUFFER_SUCCESS;
}
 int sbuffer_mutex_init()
{
  int err;
       err =  pthread_mutex_init(&mutex, NULL);
       if (err != 0) {
            printf("cannot init mutex\n");
            return 1;
        }
        err = pthread_mutex_init(&mutex_remove, NULL);
        if (err != 0) {
            printf("cannot init mutex_remove\n");
            return 1;
        }
        err = pthread_barrier_init(&reading_barrier, NULL, 2);
        if (err != 0) {
            printf("cannot init reading_barrier\n");
            return 1;
        }
        err = pthread_cond_init(&not_zero, NULL);
        if (err != 0) {
            printf("cannot init not_zero\n");
            return 1;
        }
  return 0;
}

int sbuffer_mutex_destroy()
{
   int err;
   err = pthread_barrier_destroy(&reading_barrier);
   if (err != 0) {
            printf("cannot destroy reading_barrier\n");
            return 1;
        }
   pthread_cond_destroy(&not_zero);
   if (err != 0) {
            printf("cannot destroy not_zero\n");
            return 1;
        }
   pthread_mutex_destroy(&mutex_remove);
   if (err != 0) {
            printf("cannot destroy mutex_remove\n");
            return 1;
        }
   pthread_mutex_destroy(&mutex);
   if (err != 0) {
            printf("cannot destroy mutex\n");
            return 1;
        }
   return 0;
}


















