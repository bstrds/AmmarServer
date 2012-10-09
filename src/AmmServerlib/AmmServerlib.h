#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "configuration.h"

struct AmmServer_RH_Context
{
   unsigned long MAX_content_size;
   unsigned long content_size;
   unsigned long GET_request_size;
   unsigned long POST_request_size;

   void * prepare_content_callback;

   char web_root_path[MAX_FILE_PATH];
   char resource_name[MAX_RESOURCE];

   char * content;
   char * GET_request;
   char * POST_request;
};

enum AmmServInfos
{
    AMMINF_ACTIVE_CLIENTS=0,
    AMMINF_ACTIVE_THREADS
};


enum AmmServSettings
{
    AMMSET_PASSWORD_PROTECTION=0,
    AMMSET_TEST
};


enum AmmServStrSettings
{
    AMMSET_USERNAME_STR=0,
    AMMSET_PASSWORD_STR,
    AMMSET_TESTSTR
};


int AmmServer_Start(char * ip,unsigned int port,char * conf_file,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context, char * resource_name , char * web_root, unsigned int allocate_mem_bytes,unsigned int callback_every_x_msec,void * callback);
int AmmServer_RemoveResourceHandler(struct AmmServer_RH_Context * context,unsigned char free_mem);


int AmmServer_Get_GETArguments(struct AmmServer_RH_Context * context,unsigned int associated_var_id,char * value,unsigned int max_value_length);
int AmmServer_Get_POSTArguments(struct AmmServer_RH_Context * context,unsigned int associated_var_id,char * value,unsigned int max_value_length);

int AmmServer_GetInfo(unsigned int info_type);

int AmmServer_GetIntSettingValue(unsigned int set_type);
int AmmServer_SetIntSettingValue(unsigned int set_type,int set_value);

char * AmmServer_GetStrSettingValue(unsigned int set_type);
int AmmServer_SetStrSettingValue(unsigned int set_type,char * set_value);


int AmmServer_SelfCheck();

#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
