#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "../server_configuration.h"
#include "directory_lists.h"
#include "http_tools.h"
#include <sys/stat.h>
#include <errno.h>


char * path_cat (const char *str1, char *str2)
{
size_t str1_len = strlen(str1);
size_t str2_len = strlen(str2);
char *result;
result = malloc((str1_len+str2_len+1)*sizeof *result);
strcpy (result,str1);
unsigned int i,j;
for(i=str1_len, j=0; ((i<(str1_len+str2_len)) && (j<str2_len));i++, j++) {
result[i]=str2[j];
}
result[str1_len+str2_len]='\0';
return result;
}



char * GenerateDirectoryPage(char * system_path,char * client_path,unsigned long * memoryUsed)
{
if (!ENABLE_DIRECTORY_LISTING) { return 0; }
fprintf(stderr,"Generating path for directory %s \n",system_path);

*memoryUsed=INITIAL_DIRECTORY_LIST_RESPONSE_BODY+1;
char * memory=(char*) malloc( sizeof(char) * ( *memoryUsed ) );

unsigned int mem_remaining=*memoryUsed;

char * starting="<html><head>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\
<title>AmmarServer Directory listing</title>\n </head>\n<body>\n<h1>AmmarServer Directory Listing</h1><a name=\"top\"></a><hr>\
<!-- TODO : Add some clientside javascript search and sorting capabilities! :P -->\
<table><tr><td></td><td><a href=\"#todo2\">Filename</a></td><td><a href=\"#todo3\">Byte Size</a></td><td><a href=\"#todo4\">Modification Date</a></td></tr>\n";
strncpy(memory,starting,mem_remaining);
mem_remaining-=strlen(starting);

char * tag_pre_image="<tr><td><img src=\"/";
unsigned int tag_pre_image_size=strlen(tag_pre_image);
char * tag_after_image="\">";
unsigned int tag_after_image_size=strlen(tag_after_image);
// Image Filename
char image_file[512]={0};
// Image Filename
char * tag_pre_link="</td><td class=\"link\"><a href=\"";
unsigned int tag_pre_link_size=strlen(tag_pre_link);
char * tag_after_link="\">"; // target=\"_new\"
unsigned int tag_after_link_size=strlen(tag_after_link);
 // Filename
char * tag_after_filename="</a></td>\n";
unsigned int tag_after_filename_size=strlen(tag_after_filename);


char * tag_pre_filesize="</td><td class=\"size\">";
unsigned int tag_pre_filesize_size=strlen(tag_pre_filesize);

char * tag_after_filesize="</td>\n";
unsigned int tag_after_filesize_size=strlen(tag_after_filesize);

char * tag_pre_date="</td><td class=\"date\">";
unsigned int tag_pre_date_size=strlen(tag_pre_date);

char * tag_after_date="</td></tr>\n";
unsigned int tag_after_date_size=strlen(tag_after_date);

struct stat st;
struct dirent *dp={0};
// enter existing path to directory below
DIR *dir = opendir(system_path);
if (dir==0) { return 0; }
while ((dp=readdir(dir)) != 0)
  {
    //TODO: remove // from requests.. of dp->d_name is like /filename.ext
    char *tmp = path_cat(client_path, dp->d_name);

    if ( (strcmp(dp->d_name,".")!=0) && (strcmp(dp->d_name,"..")!=0) )
    {

     //If we reached our memory limit we'll just stop serving and return what we already have with an error message in the end..
     if (*memoryUsed>=MAX_DIRECTORY_LIST_RESPONSE_BODY)
     {
        closedir(dir);
        strncat(memory,"</table><hr><h2>Error, reached memory limit</h2></body></html>",mem_remaining);
        return memory;
     }
       else
     // If we haven't reached the limit just check if we have enough memory for line , if we don't , reallocate a bigger chunk
     if (mem_remaining < GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY)
     {
        warning("Growing directory list memory");
        char * moreMemory= (int*) realloc (memory, sizeof(char) * ( *memoryUsed + GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY ) );
        if (moreMemory==0)
             {
               closedir(dir);
               strncat(memory,"</table><hr><h2>Error,  could not reallocate memory</h2></body></html>",mem_remaining);
               return memory;
             } else
             {
               memory = moreMemory;
               *memoryUsed += GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY;
               mem_remaining+= GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY;
             }

     }

     //We should have enough memory to keep filling buffer

     //<img src=\"
     strncat(memory,tag_pre_image,mem_remaining);
     mem_remaining-=tag_pre_image_size;

     //Image Filename
     GetExtensionImage(dp->d_name,image_file,512);
     strncat(memory,TemplatesInternalURI,mem_remaining);
     mem_remaining-=strlen(TemplatesInternalURI);

     strncat(memory,image_file,mem_remaining);
     mem_remaining-=strlen(image_file);

     //\">
     strncat(memory,tag_after_image,mem_remaining);
     mem_remaining-=tag_after_image_size;

     //<a href=\"
     strncat(memory,tag_pre_link,mem_remaining);
     mem_remaining-=tag_pre_link_size;

     //Filename
     strncat(memory,tmp,mem_remaining);
     mem_remaining-=strlen(tmp);

     //\"></a>
     strncat(memory,tag_after_link,mem_remaining);
     mem_remaining-=tag_after_link_size;

     //User Filename
     strncat(memory,dp->d_name,mem_remaining);
     mem_remaining-=strlen(dp->d_name);

     //Now lets try to get filesize and modification date using stat.h
     char * fullpath = path_cat(system_path,dp->d_name);
     if (fullpath!=0 )
     {
      if ( stat(fullpath, &st) == 0 )
      {
       strncat(memory," ",mem_remaining);
       mem_remaining-=1;
       char sizeStr[128]={0};
       sprintf(sizeStr,"%li",st.st_size);

       //Append FileSize information
       strncat(memory,tag_pre_filesize,mem_remaining);
       mem_remaining-=tag_pre_filesize_size;
       strncat(memory,sizeStr,mem_remaining);
       mem_remaining-=strlen(sizeStr);
       strncat(memory,tag_after_filesize,mem_remaining);
       mem_remaining-=tag_after_filesize_size;

       //Append Modification Date information
       strncat(memory,tag_pre_date,mem_remaining);
       mem_remaining-=tag_pre_date_size;
       strftime(sizeStr, 128, "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime ) );
       strncat(memory,sizeStr,mem_remaining);
       mem_remaining-=strlen(sizeStr);
       strncat(memory,tag_after_date,mem_remaining);
       mem_remaining-=tag_after_date_size;

     } else
     {
       fprintf(stderr,"Error stating file %s -> %s\n",fullpath,strerror(errno));
     }
      free(fullpath);
      fullpath=0;
     }
     //---------------------------------

     //<br>
     strncat(memory,tag_after_filename,mem_remaining);
     mem_remaining-=tag_after_filename_size;
    }

    if (tmp!=0 ) { free(tmp); tmp=0; }

   }

  /* BACK BUTTON --------------------------------------------*/
  char * pre_back="<tr><td><a href=\"#\" onclick=\"javascript:history.back()\"><img src=\"/";
  strncat(memory,pre_back,mem_remaining);
  mem_remaining-=strlen(pre_back);

  strncat(memory,TemplatesInternalURI,mem_remaining);
  mem_remaining-=strlen(TemplatesInternalURI);

  char * after_back="back.gif\"></a></td><td><a href=\"#\" onclick=\"javascript:history.back()\">Back</a></tr>";
  strncat(memory,after_back,mem_remaining);
  mem_remaining-=strlen(after_back);

  /* UP BUTTON --------------------------------------------*/
  char * pre_up="<tr><td><a href=\"#top\"><img src=\"/";
  strncat(memory,pre_up,mem_remaining);
  mem_remaining-=strlen(pre_up);

  strncat(memory,TemplatesInternalURI,mem_remaining);
  mem_remaining-=strlen(TemplatesInternalURI);

  char * after_up="up.gif\"></a></td><td><a href=\"#top\">Up</a></tr>\n";
  strncat(memory,after_up,mem_remaining);
  mem_remaining-=strlen(after_up);

  /* END OF HTML --------------------------------------------*/
  char * ending="</table><hr></body></html>";
  strncat(memory,ending,mem_remaining);
  mem_remaining-=strlen(ending);

 closedir(dir);
 return memory;
}


