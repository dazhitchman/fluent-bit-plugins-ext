//
// Created by darren on 10/8/20.
//

#ifndef CAPM_STRUTILS_H
#define CAPM_STRUTILS_H

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include "kvp.h"
#include <ctype.h>

char *ltrim(char *str, const char *seps);

char *rtrim(char *str, const char *seps);

char *trim(char *str, const char *seps);

int isnumbers(char *s);

int tohex(char *buf, long long value);

int tolong(char *buf, long long value);

int toDouble(char *buf, double value);

char * getHex(char *buf, long long value);

char * getLong(char *buf, long long value);

char * getDouble(char *buf, double value);

char * printable(char * buf);

char *remove_chars(char *src, char * remove,int remove_len) ;
char *replace_chars(char *src, char * to_replace, int replace_len,char replace_with) ;
char *replace_char_len(void *src,int slen, char  to_replace, char replace_with) ;

char *remove_char(char *src, char  remove);
char *replace_char(char *src, char  to_replace, char replace_with);
char * compress_whitepsace(char * src);

DICT * parse_into_dict(FILE *fp,char * repattern,DICT * dict);

int get_crc32(void* data,size_t len);

#endif //NJMON_STRUTILS_H
