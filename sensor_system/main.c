#define _GNU_SOURCE
#include<stdio.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include "datamgr.h"
#include "sbuffer.h"
#include "sensor_db.h"
#include "connmgr.h"
#include "config.h"

static sbuffer_t * shared_data;
int node_num;
FILE * fp_fifo;
void * writing_tcpdata(void * value)
{
    printf("port number is %d\n",*((int*)value));
  connmgr_listen(*((int*)value), &shared_data);
  pthread_exit( NULL );
}
//listen to incoming tcp data

void * data_reading(void * value)
{
  FILE * fp_sensor_map;
  fp_sensor_map = fopen("room_sensor.map", "r");
  if(fp_sensor_map != NULL) printf("sensor map open successfully!\n");
  datamgr_parse_sensor_data(fp_sensor_map, &shared_data);
  pthread_exit( NULL );
}

void * storage_reading(void * value)
{
  DBCONN *con = init_connection('1');
  storagemgr_parse_sensor_data(con, &shared_data);
  disconnect(con);
  pthread_exit( NULL );
}
int main(int argc, char * argv[])
{
    
    if(argc != 2) {
        printf("****************************************************************************************************************************\n");
        printf("****That moment when u realize you spent a whole evening tryna figure out why all of a sudden u got a segmentation fault****\n");
        printf("*************And it turns out it was all because u forgot to add that stupid port number as an argument*********************\n");
        printf("****************************************************************************************************************************\n");
        exit(EXIT_SUCCESS);
    }
    mkfifo("logFifo", 0666);
    pid_t log_process;
    log_process=fork();
    if(log_process==0)
    {

        printf("fork process created\n");
        FILE * fp_fifo_read, * fp_gate;
        char *str_result = NULL;
        char recv_buf[80];
        int i=1;
        time_t current_time;
        fp_fifo_read = fopen("logFifo", "r");
        fp_gate = fopen("gateway.log", "w");
        do
        {
            str_result = fgets(recv_buf, 80, fp_fifo_read);
            if ( str_result != NULL )
            {
                time(&current_time);
                fprintf(fp_gate,"%d %ld %s", i,current_time,recv_buf);
                fflush(fp_gate);
                i++;
            }
        } while ( str_result != NULL );
        fclose(fp_fifo_read);
        fclose(fp_gate);
        exit(EXIT_SUCCESS);
    }
    if(log_process < 0)
    { 
      printf("fork process creating failed, closing program\n");
            return 1;
    }
        fp_fifo = fopen("logFifo", "w");   
        int err;
        if (sbuffer_init(&shared_data) != SBUFFER_SUCCESS) return 1;
        //init the shared buffer, we are going to use the shared buffer solution
        err = sbuffer_mutex_init();
        if(err != 0)
        {
          printf("mutex init failed, exiting program\n");
          return 1;
        }
        pthread_t connection_manager, storage_manager, data_manager;
        int port_num = atoi(argv[1]);
        err = pthread_create(&connection_manager, NULL, writing_tcpdata, (void *) &port_num);
        if (err != 0) {
            printf("create thread error\n");
            return 1;
        }
        err = pthread_create(&data_manager, NULL, data_reading, NULL);
        if (err != 0) {
            printf("create thread error\n");
            return 1;
        }
        err = pthread_create(&storage_manager, NULL, storage_reading, NULL);
        if (err != 0) {
            printf("create thread error\n");
            return 1;
        }
        //creating all three threads
        err = pthread_join(connection_manager, NULL);
        if (err != 0) {
            printf("join thread error\n");
            return 1;
        }
        err = pthread_join(data_manager, NULL);
        if (err != 0) {
            printf("join thread error\n");
            return 1;
        }
        err = pthread_join(storage_manager, NULL);
        if (err != 0) {
            printf("join thread error\n");
            return 1;
        }
        err = sbuffer_mutex_destroy();
        
        printf("done\n");
        return 1;

}



















