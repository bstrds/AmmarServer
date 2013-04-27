#ifndef FRESHTHREADS_H_INCLUDED
#define FRESHTHREADS_H_INCLUDED

#include <netinet/in.h>
#include "../server_configuration.h"


struct PassToHTTPThread
{
     struct AmmServer_Instance * instance;

     char ip[256];
     char webserver_root[MAX_FILE_PATH];
     char templates_root[MAX_FILE_PATH];

     unsigned int port;
     unsigned int keep_var_on_stack;
     int pre_spawned_thread;

     int clientsock;
     struct sockaddr_in client;
     unsigned int clientlen;
     unsigned int thread_id;
};

#endif // FRESHTHREADS_H_INCLUDED
