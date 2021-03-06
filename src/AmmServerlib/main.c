/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "version.h"
#include "AmmServerlib.h"
#include "AString/AString.h"
#include "threads/threadedServer.h"
#include "threads/prespawnedThreads.h"
#include "cache/file_caching.h"
#include "cache/dynamic_requests.h"
#include "version.h"
#include "tools/http_tools.h"
#include "tools/logs.h"

//This is for calling back a client function after receiving
//a sigkill or other signal , after using AmmServer_RegisterTerminationSignal
void ( *TerminationCallback) (  )=0 ;


char * AmmServer_Version()
{
  return (char*) FULLVERSION_STRING;
}

int AmmServer_CheckIfHeaderBinaryAreTheSame(int headerSpec)
{
  if (AMMAR_SERVER_HTTP_HEADER_SPEC != headerSpec)
    {
      fprintf(stderr,RED "Please note that an inconsistency of binary and its header has been detected\n" NORMAL);
      return 0;
    }
  return 1;
}


void AmmServer_GeneralPrint( char * color,char * label,const char *format , va_list * arglist)
{
   unsigned int freeAtTheEnd=1;
   unsigned int formatLength = 30+strlen(format);
   char * coloredFormat= (char *) malloc( sizeof(char) * formatLength );
   if (coloredFormat==0) { coloredFormat=(char*) format; freeAtTheEnd=0; }
   coloredFormat[0]=0;
   strcpy(coloredFormat,color);
   strcat(coloredFormat,label);
   strcat(coloredFormat,": ");
   strcat(coloredFormat,format);
   strcat(coloredFormat," \n ");
   strcat(coloredFormat,NORMAL );

   vfprintf(stderr,coloredFormat, *arglist );

   fflush(stderr);

   //Maybe log this somewhere ? , just saying

   if (freeAtTheEnd) free(coloredFormat);
}

void AmmServer_Warning( const char *format , ... )
{
  va_list arglist;
  va_start( arglist, format );
  AmmServer_GeneralPrint(YELLOW,"Warning",format, &arglist );
  va_end( arglist );
}

void AmmServer_Error( const char *format , ... )
{
  va_list arglist;
  va_start( arglist, format );
  AmmServer_GeneralPrint(RED,"Error",format, &arglist );
  va_end( arglist );
}

void AmmServer_Success( const char *format , ... )
{
  va_list arglist;
  va_start( arglist, format );
  AmmServer_GeneralPrint(GREEN,"Success",format, &arglist );
  va_end( arglist );
}


int AmmServer_Stop(struct AmmServer_Instance * instance)
{
  if (!instance) { return 0; }
  StopHTTPServer(instance);
  cache_Destroy(instance);

  if (instance->threads_pool!=0) { free(instance->threads_pool); instance->threads_pool=0; }
  if (instance->prespawned_pool!=0) { free(instance->prespawned_pool); instance->prespawned_pool=0; }
  if (instance!=0) { free(instance); }
  return 1;
}

struct AmmServer_Instance * AmmServer_Start( const char * name ,
                                             const char * ip,
                                             unsigned int port,
                                             const char * conf_file,
                                             const char * web_root_path,
                                             const char * templates_root_path
                                            )
{
  fprintf(stderr,"Binding AmmarServer v%s to %s:%u\n",FULLVERSION_STRING,ip,port);



  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not meant for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");


  //Allocate and Clear instance..
  struct AmmServer_Instance * instance = (struct AmmServer_Instance *) malloc(sizeof(struct AmmServer_Instance));
  if (!instance) { fprintf(stderr,"AmmServer_Start failed to allocate a new instance \n"); } else
                 { memset(instance,0,sizeof(struct AmmServer_Instance)); }
  fprintf(stderr,"Initial AmmServer_Start instance pointing @ %p \n",instance);//Clear instance..!

  instance->threads_pool = (pthread_t *) malloc( sizeof(pthread_t) * MAX_CLIENT_THREADS);
  if (!instance->threads_pool) { fprintf(stderr,"AmmServer_Start failed to allocate %u records for a thread pool\n",MAX_CLIENT_THREADS);  } else
                               {  memset(instance->threads_pool,0,sizeof(pthread_t)*MAX_CLIENT_THREADS); }


  strncpy(instance->instanceName , name , MAX_INSTANCE_NAME_STRING); // TODO: check for MAX_INSTANCE_NAME_STRING



  fprintf(stderr,"Initial AmmServer_Start ( name %s ) thread pool pointing @ %p \n",instance->instanceName,instance->threads_pool);//Clear instance..!

  instance->prespawned_pool = (void *) malloc( sizeof(struct PreSpawnedThread) * MAX_CLIENT_PRESPAWNED_THREADS);
  if (!instance->prespawned_pool) { fprintf(stderr,"AmmServer_Start failed to allocate %u records for a prespawned thread pool\n",MAX_CLIENT_PRESPAWNED_THREADS);  }else
                                  {
                                    if (MAX_CLIENT_PRESPAWNED_THREADS>0)
                                     {
                                      memset(instance->prespawned_pool,0,sizeof(pthread_t)*MAX_CLIENT_PRESPAWNED_THREADS);
                                     }
                                  }



  //LoadConfigurationFile happens before dropping root id so we are more sure that we will manage to read the configuration file..
  LoadConfigurationFile(instance,conf_file);

  //LoadConfigurationFile may set a binding port but if the parent call set a nonzero a port setting here it overrides configuration file....
  if (port!=0) { instance->settings.BINDING_PORT = port; }

  //This line explains configuration conflicts in a user understandable manner :p
  EmmitPossibleConfigurationWarnings(instance);


  cache_Initialize( instance,
                   /*These are the file cache settings , file caching is the mechanism that holds dynamic content and
                     speeds up file serving by not accessing the whole disk drive subsystem ..*/
                   MAX_SEPERATE_CACHE_ITEMS , /*Seperate items*/
                   MAX_CACHE_SIZE_IN_MB   , /*MB Limit for the WHOLE Cache*/
                   MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB    /*MB Max Size of Individual File*/
                  );

   if (StartHTTPServer(instance,ip,instance->settings.BINDING_PORT,web_root_path,templates_root_path))
      {
          //All is well , we return a valid instance
          return instance;
      } else
      {
          AmmServer_Stop(instance);
          return 0;
      }

  return 0;
}


struct AmmServer_Instance * AmmServer_StartWithArgs(const char * name ,
                                                    int argc,
                                                    char ** argv ,
                                                    const char * ip,
                                                    unsigned int port,
                                                    const char * conf_file,
                                                    const char * web_root_path,
                                                    const char * templates_root_path)
{
   //First prepare some buffers with default values for all the arguments
   char serverName[MAX_FILE_PATH]="default";
   char webserver_root[MAX_FILE_PATH]="public_html/";
   char templates_root[MAX_FILE_PATH]="public_html/templates/";
   char configuration_file[MAX_FILE_PATH]={0};
   char bindIP[MAX_IP_STRING_SIZE]="0.0.0.0";
   unsigned int bindPort=8080;

   //If we have arguments we change our buffers
   if (name!=0)           {  strncpy(serverName,name,MAX_FILE_PATH); }
   if (web_root_path!=0)  {  strncpy(webserver_root,web_root_path,MAX_FILE_PATH); }
   if (templates_root!=0) {  strncpy(templates_root,templates_root_path,MAX_FILE_PATH); }
   if (conf_file!=0)      {  strncpy(configuration_file,conf_file,MAX_FILE_PATH); }
   if (ip!=0)             {  strncpy(bindIP,ip,MAX_IP_STRING_SIZE); }
   if (port!=0)           {  bindPort=port; }


   //If we have a command line arguments we overwrite our buffers
  int i=0;
  for (i=0; i<argc; i++)
  {
    if ((strcmp(argv[i],"-bind")==0)&&(argc>i+1)) { strncpy(bindIP,argv[i+1],MAX_IP_STRING_SIZE); fprintf(stderr,"Binding to %s \n",bindIP); } else
    if ((strcmp(argv[i],"-p")==0)&&(argc>i+1)) { bindPort = atoi(argv[i+1]); fprintf(stderr,"Binding to Port %u \n",bindPort); } else
    if ((strcmp(argv[i],"-port")==0)&&(argc>i+1)) { bindPort = atoi(argv[i+1]); fprintf(stderr,"Binding to Port %u \n",bindPort); } else
    if ((strcmp(argv[i],"-rootdir")==0)&&(argc>i+1)) { strncpy(webserver_root,argv[i+1],MAX_FILE_PATH); fprintf(stderr,"Setting web server root directory to %s \n",webserver_root); } else
    if ((strcmp(argv[i],"-templatedir")==0)&&(argc>i+1)) { strncpy(templates_root,argv[i+1],MAX_FILE_PATH); fprintf(stderr,"Setting web template directory to %s \n",templates_root); } else
    if (strcmp(argv[i],"-conf")==0)  { strncpy(configuration_file,conf_file,MAX_FILE_PATH); fprintf(stderr,"Reading Configuration file %s \n",configuration_file); }
  }

  return AmmServer_Start(name,bindIP,bindPort,configuration_file,webserver_root,templates_root);
}


int AmmServer_Running(struct AmmServer_Instance * instance)
{
  return HTTPServerIsRunning(instance);
}



//This call , calls  callback every time a request hits the server..
//The outer layer of the server can do interesting things with it :P
//request_type is supposed to be GET , HEAD , POST , CONNECT , etc..
int AmmServer_AddRequestHandler(struct AmmServer_Instance * instance,struct AmmServer_RequestOverride_Context * RequestOverrideContext,const char * request_type,void * callback)
{
  if ( (instance==0)||(RequestOverrideContext==0)||(request_type==0)||(callback==0) ) { return 0; }
  strncpy( RequestOverrideContext->requestHeader , request_type , 64 /*limit declared on AmmServerlib.h*/) ;
  RequestOverrideContext->request=0;

  instance->clientRequestHandlerOverrideContext = RequestOverrideContext;
  RequestOverrideContext->request_override_callback = callback;

  AmmServer_Warning("AmmServer_AddRequestHandler could potentially be buggy\n");

  return 1;
}


int AmmServer_AddResourceHandler
     ( struct AmmServer_Instance * instance,
       struct AmmServer_RH_Context * context,
       const char * resource_name ,
       const char * web_root,
       unsigned int allocate_mem_bytes,
       unsigned int callback_every_x_msec,
       void * callback,
       unsigned int scenario
    )
{
   if ( context->requestContext.content!=0 )
    {
      AmmServer_Warning("Context in AmmServer_AddResourceHandler for %s appears to have an already initialized memory part\n",resource_name);
      AmmServer_Warning("Make sure that you are using a seperate context for each AmmServer_AddResourceHandler call you make..\n");
    }
   memset(context,0,sizeof(struct AmmServer_RH_Context));
   strncpy(context->web_root_path,web_root,MAX_FILE_PATH);
   strncpy(context->resource_name,resource_name,MAX_RESOURCE);
   context->requestContext.MAXcontentSize=allocate_mem_bytes;
   context->callback_every_x_msec=callback_every_x_msec;
   context->last_callback=0; //This is important because a random value here will screw up things with callback_every_x_msec..
   context->callback_cooldown=0;
   context->RH_Scenario = scenario;

   if ( allocate_mem_bytes>0 )
    {
       context->requestContext.content = (char*) malloc( sizeof(char) * allocate_mem_bytes );
       if (context->requestContext.content==0) { AmmServer_Warning("Could not allocate space for request Context"); }
    }

   context->dynamicRequestCallbackFunction=callback;
   if (callback==0) { AmmServer_Warning("No callback passed for a new AmmServer_AddResourceHandler "); }


   int returnValue = cache_AddMemoryBlock(instance,context);

   if (!returnValue)
   {
     AmmServer_Error("Failed adding new resource handler\n Resource name `%s` will be unavailable\n",resource_name);
   }

  return returnValue;
}


int AmmServer_PreCacheFile(struct AmmServer_Instance * instance,const char * filename)
{
  unsigned int index=0;
  return cache_AddFile(instance,filename,&index,0);
}


int AmmServer_DoNOTCacheResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{
    char resource_name[MAX_FILE_PATH]={0};

    snprintf(resource_name,MAX_FILE_PATH,"%s%s",context->web_root_path,context->resource_name);

    if (! cache_AddDoNOTCacheRuleForResource(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResourceHandler for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}



int AmmServer_DoNOTCacheResource(struct AmmServer_Instance * instance,const char * resource_name)
{
    if (! cache_AddDoNOTCacheRuleForResource(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResource for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}


int AmmServer_RemoveResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
  return cache_RemoveContextAndResource(instance,context,free_mem);
}



int AmmServer_GetInfo(struct AmmServer_Instance * instance,unsigned int info_type)
{
  switch (info_type)
   {
     case AMMINF_ACTIVE_CLIENTS : return instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED; break;
   };
  return 0;
}


int AmmServer_POSTArg(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  if (instance==0) { return 0; }
  if  (  ( rqst->POST_request !=0 ) && ( rqst->POST_request_length !=0 ) &&  ( var_id_IN !=0 ) &&  ( var_value_OUT !=0 ) && ( max_var_value_OUT !=0 )  )
   {
     return StripVariableFromGETorPOSTString(rqst->POST_request,var_id_IN,var_value_OUT,max_var_value_OUT);
   } else
   { fprintf(stderr,"AmmServer_POSTArg failed , called with incorrect parameters..\n"); }
  return 0;
}

int AmmServer_GETArg(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  if (instance==0) { return 0; }
  if  (  ( rqst->GET_request !=0 ) && ( rqst->GET_request_length !=0 ) &&  ( var_id_IN !=0 ) &&  ( var_value_OUT !=0 ) && ( max_var_value_OUT !=0 )  )
   {
     return StripVariableFromGETorPOSTString(rqst->GET_request,var_id_IN,var_value_OUT,max_var_value_OUT);
   } else
   { fprintf(stderr,"AmmServer_GETArg failed , called with incorrect parameters..\n"); }
  return 0;
}

int AmmServer_FILES(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  if (instance==0) { return 0; }
  if ( (rqst==0) || (var_id_IN==0) || (var_value_OUT==0) || (max_var_value_OUT==0) )  { return 0; }
  fprintf(stderr,"AmmServer_FILES failed , called with incorrect parameters..\n");
  return 0;
}

/*User friendly aliases of the above calls.. :P */

int _POST(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_POSTArg(instance,rqst,var_id_IN,var_value_OUT,max_var_value_OUT);
}

int _GET(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_GETArg(instance,rqst,var_id_IN,var_value_OUT,max_var_value_OUT);
}

int _FILES(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_FILES(instance,rqst,var_id_IN,var_value_OUT,max_var_value_OUT);
}


int AmmServer_SignalCountAsBadClientBehaviour(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst)
{
   if ( (instance==0) || (rqst==0) ) { return 0; }
   fprintf(stderr,"AmmServer_SignalCountAsBadClientBehaviour is a stub ..\n");
   return 0;
}

int AmmServer_SaveDynamicRequest(const char* filename , struct AmmServer_Instance * instance  , struct AmmServer_DynamicRequest * rqst)
{
    return saveDynamicRequest(filename,instance,rqst);
}



/*
The following calls are not implemented yet

   ||
  \||/
   \/
*/

int AmmServer_GetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION : return instance->settings.PASSWORD_PROTECTION; break;
   };
  return 0;
}

int AmmServer_SetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,int set_value)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION :  instance->settings.PASSWORD_PROTECTION=set_value; return 1; break;
     case AMMSET_RANDOMIZE_ETAG_BEGINNING :  return cache_RandomizeETAG(instance);  break;
   };
  return 0;
}


char * AmmServer_GetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :    return instance->settings.USERNAME; break;
     case AMMSET_PASSWORD_STR :    return instance->settings.PASSWORD; break;
   };
  return 0;
}

int AmmServer_SetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,const char * set_value)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :  AssignStr(&instance->settings.USERNAME,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
     case AMMSET_PASSWORD_STR :  AssignStr(&instance->settings.PASSWORD,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
   };
  return 0;
}


struct AmmServer_Instance *  AmmServer_StartAdminInstance(const char * ip,unsigned int port)
{
  fprintf(stderr,"Admin instance asked to open at %s:%u , but this is not implemented\n",ip,port);
  return 0;
}



int AmmServer_SelfCheck(struct AmmServer_Instance * instance)
{
  fprintf(stderr,"No Checks Implemented in this version , instance pointer is %p ..\n",instance);
  return 0;
}




void AmmServer_ReplaceCharInString(char * input , char findChar , char replaceWith)
{
  char * cur = input;
  char * inputEnd = input+strlen(input);
  while ( cur < inputEnd )
  {
     if (*cur == findChar ) { *cur = replaceWith; }
     ++cur;
  }
  return ;
}



int AmmServer_ReplaceVarInMemoryFile(char * page,unsigned int pageLength,const char * var,const char * value)
{
  return astringReplaceVarInMemoryFile(page,pageLength,var,value);
}


int AmmServer_ReplaceAllVarsInMemoryFile(char * page,unsigned int instances,unsigned int pageLength,const char * var,const char * value)
{
  return astringReplaceAllInstancesOfVarInMemoryFile(page,instances,pageLength,var,value);
}


void AmmServer_GlobalTerminationHandler(int signum)
{
        fprintf(stderr,"Terminating AmmarServer with signum %i .. \n",signum);
          GLOBAL_KILL_SERVER_SWITCH=1;
        //&
        if (TerminationCallback!=0) { TerminationCallback(); }
        fprintf(stderr,"done\n");
        exit(0);
}


int AmmServer_RegisterTerminationSignal(void * callback)
{
  TerminationCallback = callback;

  unsigned int failures=0;
  if (signal(SIGINT, AmmServer_GlobalTerminationHandler) == SIG_ERR)  { AmmServer_Warning("AmmarServer cannot handle SIGINT!\n");  ++failures; }
  if (signal(SIGHUP, AmmServer_GlobalTerminationHandler) == SIG_ERR)  { AmmServer_Warning("AmmarServer cannot handle SIGHUP!\n");  ++failures; }
  if (signal(SIGTERM, AmmServer_GlobalTerminationHandler) == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGTERM!\n"); ++failures; }
  if (signal(SIGKILL, AmmServer_GlobalTerminationHandler) == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGKILL!\n"); ++failures; }
  return (failures==0);
}


/*
  ---------------------------------------------------

  LAST , SOME GENERIC TOOLS THAT ARE HANDY AND COMMON

  ---------------------------------------------------
*/



int AmmServer_ExecuteCommandLineNum(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize,unsigned int lineNumber)
{
 /* Open the command for reading. */
 FILE * fp = popen(command, "r");
 if (fp == 0 )
       {
         fprintf(stderr,"Failed to run command (%s) \n",command);
         return 0;
       }

 /* Read the output a line at a time - output it. */
  unsigned int i=0;
  while (fgets(what2GetBack, what2GetBackMaxSize , fp) != 0)
    {
        ++i;
        if (lineNumber==i) { break; }
    }
  /* close */
  pclose(fp);
  return 1;
}


int AmmServer_ExecuteCommandLine(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize)
{
  return AmmServer_ExecuteCommandLineNum(command,what2GetBack,what2GetBackMaxSize,1);
}

char * AmmServer_ReadFileToMemory(const char * filename,unsigned int *length )
{
  return astringReadFileToMemory(filename,length);
}




int AmmServer_WriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength)
{
  return astringWriteFileFromMemory(filename,memory,memoryLength);
}

int AmmServer_CopyOverlappingDataContent(unsigned char * buffer , unsigned int totalSize  , unsigned char * from , unsigned char * to , unsigned int blockSize)
{
  return astringCopyOverlappingDataContent(buffer,totalSize,from,to,blockSize);
}

int AmmServer_InjectDataToBuffer(unsigned char * entryPoint , unsigned char * data , struct AmmServer_MemoryHandler * mh )
{
  return astringInjectDataToMemoryHandler(mh,entryPoint,data);
}

int AmmServer_ReplaceVarInMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value)
{
  return astringInjectDataToMemoryHandler(mh,var,value);
  //return astringReplaceVarInMemoryFile(mh->content,mh->contentCurrentLength,var,value);
}


int AmmServer_ReplaceAllVarsInMemoryHandler(struct AmmServer_MemoryHandler * mh ,unsigned int instances,const char * var,const char * value)
{
  return astringReplaceAllInstancesOfVarInMemoryFile(mh->content,instances,mh->contentCurrentLength,var,value);
}



struct AmmServer_MemoryHandler * AmmServer_AllocateMemoryHandler(unsigned int initialBufferLength, unsigned int growStep)
{
 struct AmmServer_MemoryHandler * mh = ( struct AmmServer_MemoryHandler * ) malloc(sizeof(struct AmmServer_MemoryHandler));
 if (mh==0) { fprintf(stderr,"Could not allocate a memory handler of %u bytes length\n",initialBufferLength); return 0; }


 mh->content = (char*) malloc( initialBufferLength * sizeof(char));
 if (mh->content==0) { fprintf(stderr,"Could not allocate the buffer of the allocated memory handler\n"); free(mh); return 0; }

 mh->contentSize = initialBufferLength;
 mh->contentCurrentLength = initialBufferLength;

 return mh;
}


struct AmmServer_MemoryHandler *  AmmServer_ReadFileToMemoryHandler(const char * filename)
{
  struct AmmServer_MemoryHandler * mh = ( struct AmmServer_MemoryHandler * ) malloc(sizeof(struct AmmServer_MemoryHandler));
  if (mh==0) { return 0; }

   mh->content = AmmServer_ReadFileToMemory(filename,&mh->contentSize);
   mh->contentCurrentLength = mh->contentSize;

   return mh;
}


int AmmServer_FreeMemoryHandler(struct AmmServer_MemoryHandler ** mh)
{
  if (*mh==0) { return 0; }

  struct AmmServer_MemoryHandler * tmp = *mh;
  free(tmp->content);
  free(*mh);
  return 0;
}



int AmmServer_ConvertBufferToMemoryHandler(struct AmmServer_MemoryHandler * mh, unsigned char * buffer,unsigned int bufferLength)
{
  if (mh==0) { return 0; }
  mh->content = buffer;
  mh->contentSize = bufferLength;
  mh->contentCurrentLength;
  return 1;
}



int AmmServer_DirectoryExists(const char * filename)
{
 return DirectoryExistsAmmServ(filename);
}

int AmmServer_FileExists(const char * filename)
{
 return FileExistsAmmServ(filename);
}

int AmmServer_EraseFile(const char * filename)
{
 FILE *fp = fopen(filename,"w");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}


unsigned int AmmServer_StringIsHTMLSafe( const char * str)
{
  unsigned int i=0;
  while(i<strlen(str)) { if ( ( str[i]<'!' ) || ( str[i]=='<' ) || ( str[i]=='>' ) ) { return 0;} ++i; }
  return 1;
}




