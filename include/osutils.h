//
// Created by darren on 31/8/20.
//

#ifndef FLUENT_BIT_PLUGINS_EXT_OSUTILS_H
#define FLUENT_BIT_PLUGINS_EXT_OSUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>
#include <dirent.h>
#include <regex.h>

int get_filename_uid(char *filename) ;
int get_file_uid(FILE * file) ;
int get_uid_name(int uid, char *buf);


typedef int (*__accept_fn_t) (const char *, struct dirent * entry, int depth , int reftime, void * filters);


typedef struct accept_filters{
    regex_t * name_pattern;
    regex_t * dir_pattern;
    int max_depth ;
    int max_age ;
    int min_age ;
    void * userdefined ;
    __accept_fn_t user_accept_fn ;
    int depth;

}accept_filters;


#endif //FLUENT_BIT_PLUGINS_EXT_OSUTILS_H
