//
// Created by darren on 10/8/20.
// http://www.martinbroadhurst.com/trim-a-string-in-c.html
//

#include "../include/strutils.h"
#include <math.h>
#include "regex.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

char *ltrim(char *str, const char *seps)
{
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

char *trim(char *str, const char *seps)
{
    return ltrim(rtrim(str, seps), seps);
}

int isnumbers(char *s) {
    while (*s != 0) {
        if (*s < '0' || *s > '9')
            return 0;
        s++;
    }
    return 1;
}


int tohex(char *buf, long long value)
{
    return sprintf(buf, "0x%08llx",  value);
}

int tolong(char *buf, long long value)
{
    return sprintf(buf, "%lld",  value);
}

int toDouble(char *buf, double value)
{
    if (isnan(value)) {
        return sprintf(buf, "NaN");
    } else {
        return sprintf(buf, "%.3f,", value);
    }
}

char * getHex(char *buf, long long value)
{
    sprintf(buf, "0x%08llx",  value);
    return buf;
}

char * getLong(char *buf, long long value)
{
    sprintf(buf, "%lld",  value);
    return buf;
}

char * getDouble(char *buf, double value)
{
    if (isnan(value)) {
        sprintf(buf, "NaN");
    } else {
        sprintf(buf, "%.3f", value);
    }
    return buf;
}
char * printable(char * buf){
    if(buf) {
        int i = 0, len = strlen(buf);
        for (i = 0; i < len; i++)
            if (buf[i] == '\n' || iscntrl(buf[i]))
                buf[i] = ' ';

    }
    return buf;
}

//from njmon
char * compress_whitepsace(char *src)
{

    int whitepaces = 0;
    char *dest, *orig=src;

    //skip initial whitespace
    while (src && (*src==' ' || *src=='\t')){src++;};

    for (dest = orig; *src; src++) {

        if (*src == ' ' || *src == '\t') {
            whitepaces += 1;
        } else {
            whitepaces = 0;
        }
        if (whitepaces < 2) {
            *dest =*src;
            dest++;
        }
    }
    if (whitepaces && dest>orig) {
        dest--;
    }

    *dest=0;
    return orig;
}

char *replace_chars(char *src, char * to_replace, int replace_len,char replace_with) {

        if (!src || !remove){
            return src;
        }
        char *  orig = src;
        int x,replace;
        while ( (*src) ){
            replace=0;
            for ( x = 0 ; x < replace_len ; x++){
                if (*src==to_replace[x]){
                    replace=1;
                    break;
                }
            }
            if (replace){
                *src=replace_with;
            }
            src++;
        }
        return orig;
    }
char *replace_char(char *src, char  to_replace, char replace_with) {
    if (!src) {
        return src;
    }
    char *orig=src;

    while (*src++) {
        if (*src == to_replace) {
            *src = replace_with;
        }
    }
    return orig;
}
char *replace_char_len(void *src,int slen, char  to_replace, char replace_with) {
    char *orig=(char *)src ;
    char *s =(char * )src;
    for (int i = 0 ; i < slen ; i ++,*s++){
        if (*s==to_replace){
            *s=replace_with;
        }
    }
    return orig;
}
char *remove_char(char *src, char  remove) {

    if (!src) {
        return src;
    }
    char *dest=src,*orig = src;


    while ((*src)) {
        if (*src != remove) {
            *dest = *src;
            dest++;
        }
        src++;
    }
    dest=0;
    return orig;
}

char * remove_chars(char *src, char * remove,int remove_len) {

    if (!src){
        return src;
    }
    char * dest =src, *orig = src;
    int x, skip;

    while ( (*src) ){
        skip=0;
        for ( x = 0 ; x < remove_len ; x++){
            if (*src==remove[x]){
                skip=1;
                break;
            }
        }
        if (!skip){
            *dest=*src;
            dest++;
        }
        src++;
    }
    dest=0;
    return orig;
}

DICT * parse_into_dict(FILE *fp,char * repattern,DICT * dict){
    char buf[4096 + 1];

    regex_t regex;
    int reti;
    size_t maxGroups = 5;
    regex_t regexCompiled;
    regmatch_t groupArray[maxGroups];

    /* Compile regular expression */
    reti = regcomp(&regex, repattern, REG_EXTENDED);
    if (reti){
        //FIXME assert , NULL or silent or replace string arg with already re compiled arg
        return dict;
    }


    while (fgets(buf, 4096, fp) != NULL) {
        //remove \n
        buf[strlen(buf)-1]=0;

        if (!buf[strlen(buf) - 1]) {
            //flush tags and create a new section
        }

        /* Execute regular expression */
        reti = regexec(&regex, buf, maxGroups, groupArray, 0);
        if (!reti) {
            if (groupArray[2].rm_so != (size_t) -1) {
                char *key = strndup(buf+groupArray[1].rm_so, groupArray[1].rm_eo - groupArray[1].rm_so);
                char *val = strndup(buf+groupArray[2].rm_so, groupArray[2].rm_eo - groupArray[2].rm_so);
                dict_put_kvp(dict, trim(key, NULL), trim(val, NULL), CHAR);
                free(key);
                free(val);
            }
        }
    }
    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(&regex);
    return dict;
}


uint32_t crc32_for_byte(uint32_t r) {
    for(int j = 0; j < 8; ++j)
        r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if(!*table)
        for(size_t i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    for(size_t i = 0; i < n_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

int char_crc32(char* data,size_t len) {
    uint32_t crc = 0;
    crc32(data, len, &crc);
    return crc;

}