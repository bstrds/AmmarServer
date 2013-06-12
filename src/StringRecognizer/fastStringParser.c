#include "fastStringParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct fastStringParser * fspHTTPHeader = 0;

#define MAXIMUM_LINE_LENGTH 1024
#define MAXIMUM_LEVELS 123
#define ACTIVATED_LEVELS 3

enum fpsHTTPHeaderCodes
{
  NO_STRING = 0 ,
  AUTHORIZATION ,
  RANGE ,
  REFERRER ,
  REFERER ,
  HOST,
  ACCEPT_ENCODING ,
  DEFLATE,
  USER_AGENT,
  COOKIE,
  KEEP_ALIVE,
  IF_NONE_MATCH ,
  IF_MODIFIED_SINCE

};


inline void convertTo_ENUM_ID(char *sPtr)
{
  unsigned int source=0 , target=0 , holdIt=0;

  while ( (sPtr[source] != 0 ) && (sPtr[target] != 0 ) )
   {
     sPtr[target] = toupper((unsigned char) sPtr[source]);

     if  (sPtr[source]=='_')  {  } else
     if  (sPtr[source]=='-')  { sPtr[target]='_'; } else
     if  ( (sPtr[source]<'A') || (sPtr[source]>'Z' ) )
            {
              holdIt=1;
              ++target;
              sPtr[source]=sPtr[target];
              ++target;
            }

    if (!holdIt) { ++source; ++target; }
     holdIt=0;
   }
}



int fastStringParser_addString(struct fastStringParser * fsp, char * str)
{
  unsigned int ourNum = fsp->stringsLoaded++;
  fsp->contents[ourNum].strLength=strlen(str);

  if ( (ourNum==0) || (fsp->shortestStringLength<fsp->contents[ourNum].strLength) )
      { fsp->shortestStringLength = fsp->contents[ourNum].strLength; }

  if ( (ourNum==0) || (fsp->longestStringLength>fsp->contents[ourNum].strLength) )
      { fsp->longestStringLength  = fsp->contents[ourNum].strLength; }

  fsp->contents[ourNum].str = (char *) malloc(sizeof(char) * (fsp->contents[ourNum].strLength+1) );
  if (fsp->contents[ourNum].str != 0 )
  {
    strncpy(fsp->contents[ourNum].str,str,fsp->contents[ourNum].strLength);
    fsp->contents[ourNum].str[fsp->contents[ourNum].strLength]=0; // Null terminator
  } else
  {
    return 0;
  }


  fsp->contents[ourNum].strIDFriendly = (char *) malloc(sizeof(char) * (fsp->contents[ourNum].strLength+1) );
  if (fsp->contents[ourNum].strIDFriendly != 0 )
  {
    strncpy(fsp->contents[ourNum].strIDFriendly,str,fsp->contents[ourNum].strLength);
    fsp->contents[ourNum].strIDFriendly[fsp->contents[ourNum].strLength]=0; // Null terminator
    convertTo_ENUM_ID(fsp->contents[ourNum].strIDFriendly);
  } else
  {
    if (fsp->contents[ourNum].str!=0) { free(fsp->contents[ourNum].str); fsp->contents[ourNum].str=0; }
    return 0;
  }





  return 0;
}

struct fastStringParser *  fastStringParser_initialize(unsigned int totalStrings)
{
   fspHTTPHeader = (struct fastStringParser * ) malloc(sizeof( struct fastStringParser ));
   if (fspHTTPHeader== 0 ) { return 0; }

   fspHTTPHeader->stringsLoaded = 0;
   fspHTTPHeader->MAXstringsLoaded = totalStrings;
   fspHTTPHeader->contents = (struct fspString * ) malloc(sizeof( struct fspString )*fspHTTPHeader->MAXstringsLoaded);
   if (fspHTTPHeader->contents== 0 ) { return 0; }

  return fspHTTPHeader;
}

int fastStringParser_initializeHardCoded()
{
   fspHTTPHeader = fastStringParser_initialize(20);

   fastStringParser_addString(fspHTTPHeader,"AUTHORIZATION:");
   fastStringParser_addString(fspHTTPHeader,"ACCEPT-ENCODING:");
   fastStringParser_addString(fspHTTPHeader,"COOKIE:");
   fastStringParser_addString(fspHTTPHeader,"CONNECTION:");
   fastStringParser_addString(fspHTTPHeader,"DEFLATE");
   fastStringParser_addString(fspHTTPHeader,"HOST:");
   fastStringParser_addString(fspHTTPHeader,"IF-NONE-MATCH:");
   fastStringParser_addString(fspHTTPHeader,"IF-MODIFIED-SINCE:");
   fastStringParser_addString(fspHTTPHeader,"KEEP-ALIVE");
   fastStringParser_addString(fspHTTPHeader,"RANGE:");
   fastStringParser_addString(fspHTTPHeader,"REFERRER:");
   fastStringParser_addString(fspHTTPHeader,"REFERER:");
   fastStringParser_addString(fspHTTPHeader,"USER-AGENT:");

   return 1;
}


int fastStringParser_hasStringsWithNConsecutiveChars(struct fastStringParser * fsp,unsigned int * resStringResultIndex, char * Sequence,unsigned int seqLength)
{
  int res = 0;
  unsigned int i=0,count = 0,correct=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    char * str1 = fsp->contents[i].str;
    char * str2 = Sequence;
    correct=0;
    for ( count=0; count<seqLength; count++ )
    {
      if (*str1==*str2) { ++correct; }
      ++str1;
      ++str2;
    }


    if ( correct == seqLength ) {
                                   *resStringResultIndex = i;
                                   ++res;
                                   fprintf(stderr,"Comparing %s with %s : ",Sequence,fsp->contents[i].str);
                                   fprintf(stderr,"HIT\n");
                                } else
                                { /*fprintf(stderr,"MISS\n");*/ }

  }
  return res ;
}


unsigned int fastStringParser_countStringsForNextChar(struct fastStringParser * fsp,unsigned int * resStringResultIndex,char * Sequence,unsigned int seqLength)
{
 unsigned int res=0;
 Sequence[seqLength+1]=0;
 Sequence[seqLength]='A';
 while (Sequence[seqLength] <= 'Z')
  {
   res+=fastStringParser_hasStringsWithNConsecutiveChars(fsp,resStringResultIndex,Sequence,seqLength+1);
   ++Sequence[seqLength];
  }

  Sequence[seqLength]=0;
  fprintf(stderr,"%u strings with prefix %s ( length %u ) \n",res,Sequence,seqLength);

  return res;
}

void addLevelSpaces(FILE * fp , unsigned int level)
{
  int i=0;
  for (i=0; i<level*4; i++)
  {
    fprintf(fp," ");
  }
}



int printIfAllPossibleStrings(FILE * fp , struct fastStringParser * fsp , char * Sequence,unsigned int seqLength)
{
  unsigned int i=0,count=0 , correct = 0 , results =0 ;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    char * str1 = fsp->contents[i].str;
    char * str2 = Sequence;
    correct=0;
    for ( count=0; count<seqLength; count++ )
    {
      if (*str1==*str2) { ++correct; }
      ++str1;
      ++str2;
    }

    if ( correct == seqLength ) {
                                  addLevelSpaces(fp , seqLength);
                                  if (results>0) { fprintf(fp," else "); }
                                  fprintf(fp," if ( strncasecmp(str,\"%s\",%u) == 0 ) { return %s_%s; } \n",
                                          fsp->contents[i].str ,
                                          strlen(fsp->contents[i].str) ,
                                          fsp->functionName,
                                          fsp->contents[i].strIDFriendly );

                                  ++results;
                                }
  }
  return 1;
}



int printAllEnumeratorItems(FILE * fp , struct fastStringParser * fsp,char * functionName)
{
  fprintf(fp,"enum { \n");
  char enumStr[MAXIMUM_LINE_LENGTH]={0};

  fprintf(fp," %s_EMPTY=0,\n",fsp->functionName);

  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    fprintf(fp," %s_%s,\n",fsp->functionName,fsp->contents[i].strIDFriendly);
  }

  fprintf(fp," %s_END_OF_ITEMS\n",fsp->functionName);

  fprintf(fp,"};\n\n");

  return 1;
}



int recursiveTraverser(FILE * fp,struct fastStringParser * fsp,char * functionName,char * cArray,unsigned int level)
{
  if (level>=ACTIVATED_LEVELS) { return 0; }

  unsigned int resStringResultIndex=0;
  unsigned int nextLevelStrings=fastStringParser_countStringsForNextChar(fsp,&resStringResultIndex,cArray,level);

  if ( nextLevelStrings>1 )
     {
      unsigned int cases=0;
      addLevelSpaces(fp , level);
      fprintf(fp," switch (toupper(str[%u])) { \n",level);

      cArray[level]='A';
      cArray[level+1]=0;
      //TODO: Add '-' character for strings like IF-MODIFIED-ETC
      while (cArray[level] <= 'Z')
       {
        if ( fastStringParser_hasStringsWithNConsecutiveChars(fsp,&resStringResultIndex,cArray,level+1)  )
        {
          addLevelSpaces(fp , level);
          fprintf(fp," case \'%c\' : \n",cArray[level]);
           if ( level < ACTIVATED_LEVELS-1 ) { recursiveTraverser(fp,fsp,functionName,cArray,level+1); } else
                                             { printIfAllPossibleStrings(fp , fsp , cArray, level+1); }
          addLevelSpaces(fp , level);
          fprintf(fp," break; \n");
          ++cases;
        }
        ++cArray[level];
       }
       cArray[level]=0;


       if (cases==0) { fprintf(fp,"//BUG :  nextLevelStrings were supposed to be non-zero"); }

       addLevelSpaces(fp , level);
       fprintf(fp,"}; \n");
     } else
     if ( nextLevelStrings==1 )
     {
       addLevelSpaces(fp , level);
       fprintf(fp," if ( strncasecmp(str,\"%s\",%u) == 0 ) { return %s_%s; } \n",
                   fsp->contents[resStringResultIndex].str ,
                   strlen(fsp->contents[resStringResultIndex].str),
                   fsp->functionName,
                   fsp->contents[resStringResultIndex].strIDFriendly );
     }
     else
     {
       fprintf(fp," //Error ( %s ) \n",fsp->contents[resStringResultIndex].str);
     }

 return 1;
}




int export_C_Scanner(struct fastStringParser * fsp,char * functionName)
{

  fsp->functionName  = (char* ) malloc(sizeof(1+strlen(functionName)));
  strcpy(fsp->functionName,functionName);
  convertTo_ENUM_ID(fsp->functionName);


  char filenameWithExtension[1024]={0};


  //PRINT OUT THE HEADER

  sprintf(filenameWithExtension,"%s.h",functionName);
  FILE * fp = fopen(filenameWithExtension,"w");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",functionName); return 0; }

  fprintf(fp,"#ifndef %s_H_INCLUDED\n",fsp->functionName);
  fprintf(fp,"#define %s_H_INCLUDED\n\n\n",fsp->functionName);
      printAllEnumeratorItems(fp, fsp, functionName);
  fprintf(fp,"\n\nint scanFor_%s(char * str); \n\n",functionName);
  fprintf(fp,"#endif\n",fsp->functionName);
  fclose(fp);


  //PRINT OUT THE MAIN FILE

  sprintf(filenameWithExtension,"%s.c",functionName);
  fp = fopen(filenameWithExtension,"w");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",functionName); return 0; }

  char cArray[MAXIMUM_LEVELS]={0};
  int i=0;
  for (i=0; i<MAXIMUM_LEVELS; i++ ) { cArray[i]=0;/*'A';*/ }


  fprintf(fp,"#include <stdio.h>\n");
  fprintf(fp,"#include <string.h>\n");
  fprintf(fp,"#include <ctype.h>\n");
  fprintf(fp,"#include \"%s.h\"\n\n",functionName);

  fprintf(fp,"int scanFor_%s(char * str) \n{\n",functionName);

     recursiveTraverser(fp,fsp,functionName,cArray,0);

  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n");

  /*
  fprintf(fp,"\n\nint main(int argc, char *argv[]) \n {\n");
  fprintf(fp,"  if (argc<1) { fprintf(stderr,\"No parameter\\n\"); return 1; }\n");
  fprintf(fp,"  if ( scanFor_%s(argv[0]) ) { fprintf(stderr,\"Found it\"); } \n  return 0; \n }\n",functionName);
*/

  fclose(fp);

  return 1;
}






struct fastStringParser * fastSTringParser_createRulesFromFile(char* filename,unsigned int totalStrings)
{
  FILE * fp = fopen(filename,"r");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",filename); return 0; }

  struct fastStringParser *  fsp  = fastStringParser_initialize(totalStrings);
  if (fsp==0) { return 0; }

  char line[MAXIMUM_LINE_LENGTH]={0};
  unsigned int lineLength=0;
  while (fgets(line,MAXIMUM_LINE_LENGTH,fp)!=0)
  {
      lineLength = strlen(line);
      if ( lineLength > 0 )
        {
         if (line[lineLength-1]==10) { line[lineLength-1]=0; --lineLength; }
         if (line[lineLength-1]==13) { line[lineLength-1]=0; --lineLength; }
        }
      if ( lineLength > 1 )
        {
         if (line[lineLength-2]==10) { line[lineLength-2]=0; --lineLength; }
         if (line[lineLength-2]==13) { line[lineLength-2]=0; --lineLength; }
        }

    //fprintf(stderr,"LINE : `%s`\n",line);
    fastStringParser_addString(fsp,line);
  }
  fclose(fp);


  return fsp;
}





int fastStringParser_close()
{


    return 0;
}




