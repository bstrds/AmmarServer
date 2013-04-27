
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prespawnedThreads.h"
#include "freshThreads.h"

#include "../AmmServerlib.h"



struct PassToPreSpawnedThread
{
    struct AmmServer_Instance * instance;
    unsigned int i_adapt;
};


void * PreSpawnedThread(void * ptr)
{
  //We are a thread so lets retrieve our variables..
  struct PassToPreSpawnedThread * incoming_context = (struct PassToPreSpawnedThread *) ptr;

  struct AmmServer_Instance * instance = incoming_context->instance;
  unsigned int i = incoming_context->i_adapt;
  incoming_context->i_adapt = MAX_CLIENT_PRESPAWNED_THREADS+1; // <-- This signals we got the i value..


  if (instance==0) { fprintf(stderr,"Prespawned thread did not receive a valid instance context\n"); return 0; }
  //We will also spawn our own threads so lets prepare their variables..
  struct PassToHTTPThread context; // <-- This is the static copy of the context we will pass through
  memset(&context,0,sizeof(struct PassToHTTPThread)); // We clear it out


  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data;
  prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

  while ( (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
   {
      //fprintf(stderr,"Prespawned Thread %u waiting ( its %u's turn ) \n",i,prespawn_turn_to_serve);
          /*It is our turn!!*/
          if (prespawned_data->busy) //Master thread considers us busy again , this means there is work to be done..!
          {
            ++instance->prespawn_jobs_started;
            /*We have something to do , lets fill our context..*/
             context.instance=instance;
             context.clientsock=prespawned_data->clientsock;
             context.client=prespawned_data->client;
             context.clientlen=prespawned_data->clientlen;
             context.pre_spawned_thread = 1; // THIS IS A !!!!PRE SPAWNED!!!! THREAD
             strncpy(context.webserver_root,prespawned_data->webserver_root,MAX_FILE_PATH);
             strncpy(context.templates_root,prespawned_data->templates_root,MAX_FILE_PATH);
             context.keep_var_on_stack=1;

              //ServeClient from this thread ( without forking..! )
              fprintf(stderr,"Prespawned thread %u/%u starting to serve new client\n",i,MAX_CLIENT_PRESPAWNED_THREADS);
                ServeClient((void *)  &context);
              //---------------------------------------------------

             prespawned_data->busy=0; // <- This signals we finished our task ..!
             ++instance->prespawn_jobs_finished;
           }

      if (instance->prespawn_turn_to_serve==i)
            { usleep(THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT); /*It is our turn next so lets stay vigilant ( But not use a crazy lot of CPU time ) */ }  else
               { usleep(THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS); /*It is not our turn so lets chill for more time..*/ }
  } // while the server doesn't stop..

  return 0;
}

void PreSpawnThreads(struct AmmServer_Instance * instance)
{
  if (MAX_CLIENT_PRESPAWNED_THREADS==0) { fprintf(stderr,"PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n"); }

  if ( (instance==0)||(instance->prespawned_pool==0) ) { fprintf(stderr,"PreSpawnThreads called on an invalid instance..\n"); return; }

  struct PassToPreSpawnedThread context;
  memset(&context,0,sizeof(struct PassToPreSpawnedThread));

  struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
  struct PreSpawnedThread * prespawned_data=0;

  unsigned int i=0;
  for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
   {

      prespawned_data = (struct PreSpawnedThread *) &prespawned_pool[i];

      context.instance = instance;
      context.i_adapt = i;
      prespawned_data->busy=0; // We do this here (and not in the PreSpawnedThread ) to make sure a clean state is sure to be initialized , not having race conditions , locks etc...
      int retres = pthread_create(&prespawned_data->thread_id,0,PreSpawnedThread,(void*) &context );
      if ( retres==0 ) { while (context.i_adapt==i) { usleep(1); } } // <- Keep i value the same for long enough without locks
   }
}


inline void TrimPrespawnedCountersToPreventTruncation(struct AmmServer_Instance * instance)
{
  //Job Start Counter and Job Finished counter keeps rising and rising so we trim it every 1000 numbers
  if ( (instance->prespawn_jobs_started>1000) &&
        (instance->prespawn_jobs_finished>1000)  )
         {
             instance->prespawn_jobs_started-=1000;
             instance->prespawn_jobs_finished-=1000;
         }
}



int UsePreSpawnedThreadToServeNewClient(struct AmmServer_Instance * instance,int clientsock,struct sockaddr_in client,unsigned int clientlen,char * webserver_root,char * templates_root)
{
    if (MAX_CLIENT_PRESPAWNED_THREADS==0)
        {
          fprintf(stderr,"PreSpawning Threads is disabled , alter MAX_CLIENT_PRESPAWNED_THREADS to enable it..\n");
          return 0;
        }

   //Please note that this must only get called from the main process/thread..
   fprintf(stderr,"UsePreSpawnedThreadToServeNewClient instance pointing @ %p \n",instance);

   struct PreSpawnedThread * prespawned_pool = (struct PreSpawnedThread *) instance->prespawned_pool;
   struct PreSpawnedThread * prespawned_data=0;

   //TrimPrespawnedCountersToPreventTruncation(instance);

   /* This doesnt work as it was supposed to!
   if ( instance->prespawn_jobs_started < instance->prespawn_jobs_finished )
   {
       warning("Prespawn jobs counters truncated (?) \n");
       fprintf(stderr,"Prespawn Trunc Details ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
    } else
    */
   if (instance->prespawn_jobs_started-instance->prespawn_jobs_finished<MAX_CLIENT_PRESPAWNED_THREADS)
    {
        prespawned_data = &prespawned_pool[instance->prespawn_turn_to_serve];

        //Attempt to find another prespawned context
        if (prespawned_data->busy)
         {
            unsigned int i=0;
            for (i=0; i<MAX_CLIENT_PRESPAWNED_THREADS; i++)
            {
              prespawned_data = &prespawned_pool[i];
              if (!prespawned_data->busy) { break; }
            }
         }

        if (prespawned_data->busy)
         {
            fprintf(stderr,"Seems that the prespawned thread is still busy (  %u/%u ) ..\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
            return 0;
         }

        if (!prespawned_data->busy)
         {
             fprintf(stderr,"Decided to use prespawned thread %u/%u to serve new client\n",instance->prespawn_turn_to_serve,MAX_CLIENT_PRESPAWNED_THREADS);
             prespawned_data->clientsock=clientsock;
             prespawned_data->client=client;
             prespawned_data->clientlen=clientlen;
             strncpy(prespawned_data->webserver_root,webserver_root,MAX_FILE_PATH);
             strncpy(prespawned_data->templates_root,templates_root,MAX_FILE_PATH);
             // The busy byte gets filled in last because it is what causes the client thread to wake up..!
             prespawned_data->busy=1;

             ++instance->prespawn_turn_to_serve;
             instance->prespawn_turn_to_serve = instance->prespawn_turn_to_serve % MAX_CLIENT_PRESPAWNED_THREADS; // <- Round robin next thread..

             return 1;
         }
    } else
    {
        fprintf(stderr,"All prespawned threads are busy.. ( start %u , end %u , max %u) \n",instance->prespawn_jobs_started,instance->prespawn_jobs_finished,MAX_CLIENT_PRESPAWNED_THREADS);
    }
  return 0;
}

