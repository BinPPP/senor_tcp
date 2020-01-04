#define _GNU_SOURCE

#include<poll.h>
#include<stdio.h>
#include <inttypes.h>
#include"connmgr.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "config.h"

#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)



static dplist_t * sockets;

extern FILE * fp_fifo;

void * tcp_element_copy(void * element) {
  /* tcpsock_t* copy = malloc(sizeof (tcpsock_t));
    assert(copy != NULL);
    copy->cookie = ((tcpsock_t*)element)->cookie;
    copy->sd = ((tcpsock_t*)element)->sd;
    copy->ip_addr = ((tcpsock_t*)element)->ip_addr;
    copy->port = ((tcpsock_t*)element)->port;*/
    return element;
}
void tcp_element_free(void ** element) {
    free(*element);
    *element = NULL;
}

int tcp_element_compare(void * x, void * y) {
    return 1;}





void connmgr_listen(int port_number, sbuffer_t ** buffer)
{
  FILE * fp_bin;
  tcpsock_t * server, * client;
  sensor_data_t data;
  int socket_num = 0;
  fp_bin = fopen("sensor_data_recv", "w");
  FILE_ERROR(fp_bin,"Couldn't create sensor_data\n");  
  sockets = dpl_create(tcp_element_copy,tcp_element_free,tcp_element_compare);
  printf("server is started\n");
  if (tcp_passive_open(&server,port_number)!=TCP_NO_ERROR) 
  {printf("cannot open server\n");exit(EXIT_FAILURE);}
  else{
      fprintf(fp_fifo, "server is started\n");
      fflush(fp_fifo);
  }
  dpl_insert_at_index(sockets, (void*)server, dpl_size(sockets), false);
  socket_num++;
  while(1)
  {
    struct pollfd fds[socket_num];
    dplist_node_t * node = dpl_get_first_reference(sockets);
    int i;
    for(i = 0; i < socket_num; i++)
    {
      struct tcpsock_t * sock = dpl_get_element_at_reference(sockets, node);
      tcp_get_sd((tcpsock_t*)sock, &(fds[i].fd));
      fds[i].events = POLLIN;
      node = dpl_get_next_reference(sockets, node);
    }
    //init the array of pollfd
    if(i>0)
    {
    int active_num;
    active_num = poll(fds, socket_num, DTIMEOUT);
    if(active_num < 0)
    {printf("poll error\n");}
    if(active_num == 0)
    {
      connmgr_free();
      printf("buffer is freed\n");
      sbuffer_free(buffer);
      fprintf(fp_fifo, "buffer is freed\n");
      fflush(fp_fifo);
      printf("server is closed\n");
      fprintf(fp_fifo, "server is closed\n");
      fflush(fp_fifo);
      break;
    }
    if(active_num >0)
    {
      if(fds[0].revents == POLLIN)
      {
        if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");
        fprintf(fp_fifo, "Incoming client connection\n");
        fflush(fp_fifo);
        socket_num++;
        dpl_insert_at_index(sockets, (void*)client, dpl_size(sockets), false);
      }
      for(i = 1; i < socket_num; i++)
      {
         if(fds[i].revents == POLLIN)
         {
           int bytes, result;
           client = dpl_get_element_at_index(sockets, i);
           bytes = sizeof(data.id);
           result = tcp_receive(client,(void *)&data.id,&bytes);
           bytes = sizeof(data.value);
           tcp_receive(client,(void *)&data.value,&bytes);
           bytes = sizeof(data.ts);
           tcp_receive(client, (void *)&data.ts,&bytes);
           if((result == TCP_NO_ERROR) && bytes)
           {
            fprintf(fp_fifo, "measurement received: sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);
            fflush(fp_fifo);
           sbuffer_insert(*buffer, &data);
           }
           if(result == TCP_CONNECTION_CLOSED)
           {
             fprintf(fp_fifo, "sensor id = %" PRIu16 " is disconnected\n", data.id);
             fflush(fp_fifo);
             dpl_remove_at_index(sockets, i, true);
             socket_num--;
           }
           
         }
      }
      
    }

    }
    
  }
}

void connmgr_free()
{
  tcpsock_t * server;
  server = dpl_get_element_at_index(sockets, 0);
  tcp_close(&server);
  printf("server is freed\n");
}


















