//
// Created by darren on 4/8/20.
//

#ifndef CAPM_KVP_H
#define CAPM_KVP_H

#include <stdlib.h>


#define DICT_FREE_SHALLOW 0
#define DICT_FREE_DEEP 1

enum value_type {STRING,CHAR,LONG,DOUBLE,LONGLONG,PTR,FREEABLE_PTR,DICTREF};

typedef struct ENTRY {
    enum value_type type;
    int keylen;
    char *key;
    int vallen;
    void *value;
    //fn toString
    // allow each tag to be wither owned or not.. unsigned char copy_mode;
} ENTRY, *PENTRY;

typedef struct {
    char * name ;
    size_t length;
    size_t count;
    PENTRY  * entry  ;
} DICT, * PDICT;

//todo replace ENTRY in the DICT with DICT_STACK of ENTRY
typedef struct DICT_STACK{
    size_t index;
    size_t count;
    void * * items  ;
} DICT_STACK;



DICT* dict_create(const char * name );

void   dict_free(DICT *dict,unsigned int recursive);
DICT*  dict_put(DICT *dict, PENTRY entry);
void* dict_get(DICT *dict, char * key);
void *dict_get_default(DICT *dict, char *key, void * defolt) ;
char *dict_getchar_default(DICT *dict, char *key, char * defolt) ;
PENTRY dict_entry_create(char * key , void * value , enum value_type type);
PENTRY dict_get_entry(DICT *dict, char * key);

DICT *dict_put_kvp(DICT *dict, char *key, void *value, enum value_type type);

DICT *dict_put_long(DICT *dict, char *key,long valuwe);
DICT *dict_put_longlong(DICT *dict, char *key,long long value);
DICT *dict_put_double(DICT *dict, char *key,double value);
DICT *dict_put_str(DICT *dict, char *key,char * value);
DICT *dict_put_char(DICT *dict, char *key,char  value);








#endif