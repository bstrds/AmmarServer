#ifndef TEXTFILES_H_INCLUDED
#define TEXTFILES_H_INCLUDED


enum { 
 TEXTFILES_EMPTY=0,
 TEXTFILES_HTML,
 TEXTFILES_HTM,
 TEXTFILES_CSS,
 TEXTFILES_TXT,
 TEXTFILES_DOC,
 TEXTFILES_RTF,
 TEXTFILES_ODF,
 TEXTFILES_ODT,
 TEXTFILES_END_OF_ITEMS
};



int scanFor_textFiles(char * str,unsigned int strLength); 

#endif