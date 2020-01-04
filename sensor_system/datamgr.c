#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "lib/dplist.h"
#include "datamgr.h"

static dplist_t *list;

extern FILE * fp_fifo;

typedef struct{
  sensor_id_t sensor_id;
  uint16_t room_id;
  double last_modified[RUN_AVG_LENGTH];
  double avrg; 
  int count;
  int count_flag;
  time_t timestamp; 
}element_t;


void * element_copy(void * element) {
    element_t* copy = malloc(sizeof (element_t));
    assert(copy != NULL);
    copy->sensor_id = ((element_t*)element)->sensor_id;
    copy->room_id = ((element_t*)element)->room_id;
    copy->timestamp = ((element_t*)element)->timestamp;
    return (void *) copy;
}
void element_free(void ** element) {
    free(*element);
    *element = NULL;
}

int element_compare(void * x, void * y) {
    return ((((element_t*)x)->sensor_id < ((element_t*)y)->sensor_id) ? -1 : (((element_t*)x)->sensor_id == ((element_t*)y)->sensor_id) ? 0 : 1);
}



void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t ** buffer)
{
  list = dpl_create(element_copy,element_free,element_compare);
  while(!feof(fp_sensor_map))
  {
    element_t *sensor_element = malloc(sizeof(element_t));
    fscanf(fp_sensor_map,"%" SCNu16 " %" SCNu16 "\n", &(sensor_element -> room_id), &(sensor_element -> sensor_id));
    sensor_element -> avrg = 0;
    sensor_element -> count = 0;
    sensor_element -> count_flag = 0;
    dpl_insert_at_index(list, sensor_element, dpl_size(list), 0);
  }
  //init the nodes
  int i = 0;
  int j;
  sensor_data_t sensor_data;
  while(sbuffer_remove(*buffer, &sensor_data)!= SBUFFER_NO_DATA)
  {
    dplist_node_t * node = dpl_get_first_reference(list);
    element_t * sensor_element;

    for(i = 0; i < dpl_size(list); i++)
    {
      sensor_element = dpl_get_element_at_reference(list, node);
      if(sensor_element -> sensor_id == sensor_data.id)
      {
        sensor_element -> last_modified[sensor_element -> count] = sensor_data.value;
 
        sensor_element -> timestamp = sensor_data.ts;
 
        (sensor_element -> count)++;
        if((sensor_element -> count) >= RUN_AVG_LENGTH)
        {
          sensor_element -> count_flag = 1;
          sensor_element -> count = 0;
        }
        break;
      }
      node = dpl_get_next_reference(list, node);
    }
    if(i == dpl_size(list) && node == NULL)
    {
        fprintf(fp_fifo, "Received sensor data with invalid sensor node ID: %d!\n", sensor_data.id);
        fflush(fp_fifo);
    }
    if(sensor_element -> count_flag == 1)
    {
      sensor_element -> avrg = 0;
      for(j = 0; j < RUN_AVG_LENGTH; j++)
      {
        sensor_element -> avrg = sensor_element -> avrg + sensor_element -> last_modified[j];
        //printf("%lf ", sensor_element -> last_modified[j]);
      }
      sensor_element -> avrg = sensor_element -> avrg / RUN_AVG_LENGTH;
      if(sensor_element -> avrg < SET_MIN_TEMP)
      fprintf(fp_fifo,"Room %hd  is too cold with avrg temp = %lf\n", sensor_element -> room_id, sensor_element -> avrg);
      fflush(fp_fifo);
      if(sensor_element -> avrg > SET_MAX_TEMP)
      fprintf(fp_fifo,"Room %hd  is too hot with avrg temp = %lf\n", sensor_element -> room_id, sensor_element -> avrg);
      fflush(fp_fifo);
    }
    
  }
  datamgr_free();
}

void datamgr_free()
{
  dpl_free(&list,true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id)
{
  int i;
  uint16_t room_id;
  for(i = 0; i < dpl_size(list); i++)
  {
    element_t * sensor_element = dpl_get_element_at_index( list, i );
    if(sensor_element -> sensor_id == sensor_id)
    {
      room_id = sensor_element -> room_id;
      break;
    } 
    else continue;
  }
  return room_id;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id)
{
  int i;
  double avrg;
  for(i = 0; i < dpl_size(list); i++)
  {
    element_t * sensor_element = dpl_get_element_at_index( list, i );
    if(sensor_element -> sensor_id == sensor_id)
    {
      avrg = sensor_element -> avrg;
      break;
    } 
    else continue;
  }
  return avrg;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id)
{
  int i;
  time_t timestamp;
  for(i = 0; i < dpl_size(list); i++)
  {
    element_t * sensor_element = dpl_get_element_at_index( list, i );
    if(sensor_element -> sensor_id == sensor_id)
    {
      timestamp = sensor_element -> timestamp;
      break;
    } 
    else continue;
  }
  return timestamp;
}

int datamgr_get_total_sensors()
{
  int number_of_sensor;
  number_of_sensor = dpl_size( list );
  return number_of_sensor;
}





















