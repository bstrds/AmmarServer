//
//hashmap implementation for C , original (master) repository -> https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/hashmap/hashmap.c
//written by Ammar Qammaz
//


#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */


#include "hashmap.h"

/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
unsigned long hashFunction(const char *str)
{
 if (str==0) return 0;
 if (str[0]==0) return 0;

 unsigned long hash = 5381; //<- magic
 int c=1;

 while (c != 0)
        {
            c = *str++;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

 return hash;
}


int hashMap_Grow(struct hashMap * hm,unsigned int growthSize)
{
  struct hashMapEntry * newentries = realloc(hm->entries , sizeof(struct hashMapEntry) * ( hm->maxNumberOfEntries + growthSize ) );
  if (newentries!=0)
     {
       hm->entries=newentries;
       memset(newentries+hm->maxNumberOfEntries,0,growthSize * sizeof(struct hashMapEntry));
       hm->maxNumberOfEntries += growthSize;
       return 1;
     } else
     {
       fprintf(stderr,"Could not grow hashMap (running out of memory?)");
     }
  return 0;
}

struct hashMap * hashMap_Create(unsigned int initialEntries,unsigned int entryAllocationStep,void * clearItemFunction)
{
  struct hashMap * hm = (struct hashMap *)  malloc(sizeof(struct hashMap));
  if (hm==0)  { fprintf(stderr,"Could not allocate a new hashmap"); return 0; }

  memset(hm,0,sizeof(struct hashMap));

  hm->entries=0;
  hm->entryAllocationStep=entryAllocationStep;
  hm->maxNumberOfEntries=0;
  hm->curNumberOfEntries=0;

  if (!hashMap_Grow(hm,initialEntries) )
  {
    fprintf(stderr,"Could not grow new hashmap for the first time");
    free(hm);
    return 0;
  }

  hm->clearItemCallbackFunction = clearItemFunction;

    #if HASHMAP_BE_THREAD_SAFE
      pthread_mutex_init(&hm->hm_addLock,0);
    #endif // HASHMAP_BE_THREAD_SAFE

  return hm;
}

int hashMap_IsOK(struct hashMap * hm)
{
    if (hm == 0)  { return 0; }
    if (hm->entries == 0)  { return 0; }
    if (hm->maxNumberOfEntries == 0)  { return 0; }
    return 1;
}

int hashMap_GetCurrentNumberOfEntries(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  return hm->curNumberOfEntries;
}

int hashMap_GetMaxNumberOfEntries(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  return hm->maxNumberOfEntries;
}


int hashMap_IsSorted(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  unsigned int i=1;
    while ( i < hm->curNumberOfEntries )
    {
      if (hm->entries[i-1].keyHash > hm->entries[i].keyHash)
         { return 0; /*We got ourself a non sorted entry!*/ }
      ++i;
    }
  return 1;
}


void hashMap_Clear(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return; }
  void ( *hashMapClearCallback) ( void * )=0 ;
  hashMapClearCallback = hm->clearItemCallbackFunction;
  unsigned int i=0;
  unsigned int entryNumber = hm->curNumberOfEntries; //cur

  hm->curNumberOfEntries = 0;
  fprintf(stderr,"hashMap_Clear with %u , %u ( %u max ) entries \n", i , entryNumber ,  hm->maxNumberOfEntries );

  while (i < entryNumber)
  {
    //Clear the payload , if we have a callback function for that use it
    if (hm->clearItemCallbackFunction != 0 )
         { hashMapClearCallback(hm->entries[i].payload); } else
    //If payload is not just a pointer ( length != 0 )  , free it
    if (hm->entries[i].payloadLength!=0)
         {
           //fprintf(stderr,"Clearing %u/%u entry payload\n", i , entryNumber );
           free(hm->entries[i].payload);
           hm->entries[i].payload=0;
         }


    if (hm->entries[i].key!=0)
    {
      //fprintf(stderr,"Clearing %u/%u entry key\n", i , entryNumber );
      free(hm->entries[i].key);
      hm->entries[i].key=0;
    }

    hm->entries[i].keyHash=0;
    hm->entries[i].keyLength=0;


    ++i;
  }

  return;
}

void hashMap_Destroy(struct hashMap * hm)
{
  if (hm==0) { return ; }

  if ( hm->entries != 0)
  {
   hashMap_Clear(hm);
   hm->maxNumberOfEntries=0;
   free(hm->entries);
   hm->entries=0;
  }
  hm->clearItemCallbackFunction=0;
  free(hm);

    #if HASHMAP_BE_THREAD_SAFE
      pthread_mutex_destroy(&hm->hm_addLock);
    #endif // HASHMAP_BE_THREAD_SAFE

  return ;
}


/* qsort struct comparision function (price float field) ( See qsort call in main ) */
int cmpHashTableItems(const void *a, const void *b)
{
    struct hashMapEntry *ia = (struct hashMapEntry *)a;
    struct hashMapEntry *ib = (struct hashMapEntry *)b;

    return (ia->keyHash > ib->keyHash);
}

int hashMap_Sort(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  qsort( hm->entries , hm->curNumberOfEntries , sizeof(struct hashMapEntry), cmpHashTableItems);
  return 1;
}


int hashMap_Add(struct hashMap * hm,const char * key,void * val,unsigned int valLength)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  int clearToAdd=1;

    #if HASHMAP_BE_THREAD_SAFE
       pthread_mutex_lock (&hm->hm_addLock); // LOCK PROTECTED OPERATION -------------------------------------------
    #endif // HASHMAP_BE_THREAD_SAFE


  if (hm->curNumberOfEntries >= hm->maxNumberOfEntries)
  {
    if  (!hashMap_Grow(hm,hm->entryAllocationStep))
    {
      fprintf(stderr,"Could not grow new hashmap for adding new values");
      clearToAdd = 0;
    }
  }

  if (clearToAdd)
  {
    if (hm->entries==0) { fprintf(stderr,"While Adding a new key to hashmap , entries not allocated"); return 0; }

    unsigned int our_index=hm->curNumberOfEntries++;
    if (hm->entries[our_index].key!=0)
    {
     fprintf(stderr,"While Adding a new key to hashmap , entry was not clean");
     free(hm->entries[our_index].key);
     hm->entries[our_index].key=0;
    }

    if (key!=0)
    {
     hm->entries[our_index].key = (char *) malloc(sizeof(char) * (strlen(key)+1) );
     if (hm->entries[our_index].key == 0)
         {
           --hm->curNumberOfEntries;

             #if HASHMAP_BE_THREAD_SAFE
              pthread_mutex_unlock (&hm->hm_addLock); // LOCK PROTECTED OPERATION -------------------------------------------
             #endif // HASHMAP_BE_THREAD_SAFE

             fprintf(stderr,"While Adding a new key to hashmap , could not allocate key");
           return 0;
         }
     hm->entries[our_index].keyLength = strlen(key);
     hm->entries[our_index].keyHash = hashFunction(key);
     strncpy(hm->entries[our_index].key , key , hm->entries[our_index].keyLength);
     hm->entries[our_index].key[strlen(key)]=0; //Force null termination
    }

    if (valLength==0)
    {
      //We store and serve direct pointers! :)
      hm->entries[our_index].payload = val;
      hm->entries[our_index].payloadLength = 0;
    } else
    {
      hm->entries[our_index].payload = (void *) malloc(sizeof(char) * (valLength) );
      if (hm->entries[our_index].payload == 0)
      {
        if (hm->entries[our_index].key!=0 ) { free(hm->entries[our_index].key); }
        hm->entries[our_index].key=0;
        --hm->curNumberOfEntries;
        fprintf(stderr,"While Adding a new key to hashmap , couldn't allocate payload");

          #if HASHMAP_BE_THREAD_SAFE
           pthread_mutex_unlock (&hm->hm_addLock); // LOCK PROTECTED OPERATION -------------------------------------------
          #endif // HASHMAP_BE_THREAD_SAFE
      }
      memcpy(hm->entries[our_index].payload,val,valLength);
      hm->entries[our_index].payloadLength = valLength;
    }

    //fprintf(stderr,"Added %s => %p ( %u ) \n",hm->entries[our_index].key,hm->entries[our_index].payload,hm->entries[our_index].payload);

  }

  #if HASHMAP_BE_THREAD_SAFE
   pthread_mutex_unlock (&hm->hm_addLock); // LOCK PROTECTED OPERATION -------------------------------------------
  #endif // HASHMAP_BE_THREAD_SAFE

  return 1;
}


int hashMap_AddULong(struct hashMap * hm,const char * key,unsigned long val)
{
  void * valPTRForm=0;
  valPTRForm = (void *) val;
  return hashMap_Add(hm,key,valPTRForm,0);
}


int hashMap_FindIndex(struct hashMap * hm,const char * key,unsigned long * index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned long i=0;
  unsigned long keyHash = hashFunction(key);

  //Maybe the index was the answer all along
  if (*index< hm->curNumberOfEntries)
  {
    if ( hm->entries[*index].keyHash == keyHash )
    {
     ++hm->entries[*index].hits;
     return 1;
    }
  }

 //Stupid and slow serial search
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].keyHash == keyHash )
         {
          ++hm->entries[i].hits;
          *index = i;
          return 1;
         }
    ++i;
  }
  return 0;
}


int hashmap_SwapRecords(struct hashMap * hm , unsigned int index1,unsigned int index2)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (index1 >= hm->curNumberOfEntries ) { return 0; }
  if (index2 >= hm->curNumberOfEntries ) { return 0; }

  struct hashMapEntry * buf= (struct hashMapEntry *) malloc(sizeof(struct hashMapEntry));
  if (buf==0) { return 0; }
  memcpy(buf,                   &hm->entries[index1] , sizeof(struct hashMapEntry));
  memcpy(&hm->entries[index1] , &hm->entries[index2] , sizeof(struct hashMapEntry));
  memcpy(&hm->entries[index2] , buf                 , sizeof(struct hashMapEntry));
  free(buf);
  return 1;
}


char * hashMap_GetKeyAtIndex(struct hashMap * hm,unsigned int index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (index >= hm->curNumberOfEntries ) { return 0; }
  return hm->entries[index].key;
}


unsigned long hashMap_GetHashAtIndex(struct hashMap * hm,unsigned int index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (index >= hm->curNumberOfEntries ) { return 0; }
  return hm->entries[index].keyHash;
}

int hashMap_GetPayload(struct hashMap * hm,const char * key,void * payload)
{
  unsigned long i=0;
  if (hashMap_FindIndex(hm,key,&i))
    {
       fprintf(stderr,"Payload was pointing to %p and now it is pointing to ",payload);
        payload = hm->entries[i].payload;
       fprintf(stderr,"%p \n",payload);
       return 1;
    }
  return 0;
}


int hashMap_GetULongPayload(struct hashMap * hm,const char * key,unsigned long * payload)
{
  unsigned long i=*payload;
  if (hashMap_FindIndex(hm,key,&i)) {  *payload = (unsigned long) hm->entries[i].payload; return 1; }
  return 0;
}



int hashMap_ContainsKey(struct hashMap * hm,const char * key)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned int i=0;
  unsigned long keyHash = hashFunction(key);
  //fprintf(stderr,"Key we are searching for has value %lu ( %s ) \n",keyHash,key);
  while ( i < hm->curNumberOfEntries )
  {
    //fprintf(stderr,"Element %u has value %lu  ( %s )\n",i,hm->entries[i].keyHash,hm->entries[i].key);
    if ( hm->entries[i].keyHash == keyHash )
         {
          //fprintf(stderr,"Found Match\n");
          return 1;
         }
    ++i;
  }

  //fprintf(stderr,"Could not Find Match");
  return 0;
}


int hashMap_ContainsValue(struct hashMap * hm,void * val)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned int i=0;
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].payload == val ) { return 1; }
    ++i;
  }
  return 0;
}




int hashMap_SaveToFile(struct hashMap * hm,const char * filename)
{
  if (!hashMap_IsOK(hm)) { return 0;}

  #if HASHMAP_BE_THREAD_SAFE
     pthread_mutex_lock (&hm->hm_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
  #endif // HASHMAP_BE_THREAD_SAFE

  int result = 0;
  FILE * pFile=0;
  pFile = fopen (filename,"wb");
  unsigned int zero=0;
  if (pFile!=0)
   {
    unsigned int i=0;

    char uintsize=sizeof(hm->entries[0].keyLength);
    fwrite(&uintsize,1,1,pFile);
    fwrite(&hm->curNumberOfEntries,sizeof(unsigned int),1, pFile);
    fwrite(&hm->entryAllocationStep,sizeof(unsigned int),1, pFile);
     while (i<hm->curNumberOfEntries)
       {
         if  ( (hm->entries[i].keyLength!=0)&&(hm->entries[i].key!=0) )
           {
            fwrite(&hm->entries[i].keyLength,sizeof(unsigned int),1, pFile);
            fwrite(hm->entries[i].key,sizeof(char),hm->entries[i].keyLength, pFile);
           } else
           { fwrite(&zero,sizeof(unsigned int),1, pFile); }

         if  ( (hm->entries[i].payloadLength!=0)&&(hm->entries[i].payload!=0) )
           {
            fwrite(&hm->entries[i].payloadLength,sizeof(unsigned int),1, pFile);
            fwrite(hm->entries[i].payload,sizeof(char),hm->entries[i].payloadLength, pFile);
           } else
           { fwrite(&zero,sizeof(unsigned int),1, pFile); }

         ++i;
       }

     fclose (pFile);
     result=1;
    }

  #if HASHMAP_BE_THREAD_SAFE
    pthread_mutex_unlock (&hm->hm_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
  #endif // HASHMAP_BE_THREAD_SAFE

  return result;
}


int hashMap_LoadToFile(struct hashMap * hm,const char * filename)
{
    if (hm==0) { fprintf(stderr,"hashMap_LoadToFile cannot load file `%s` without an allocated hashmap structure \n",filename); return 0; }
    fprintf(stderr,"hashMap_LoadToFile not implemented ( max entries when called %u ) \n",hm->maxNumberOfEntries);
    return 0;
    FILE * pFile;
    pFile = fopen (filename,"rb");
    if (pFile!=0)
    {
      //TODO IMPLEMENT!
     fclose (pFile);
     return 0;
    }
    return 0;
}






