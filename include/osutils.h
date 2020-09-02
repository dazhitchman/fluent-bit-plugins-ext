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

int get_filename_uid(char *filename) ;
int get_file_uid(FILE * file) ;
int get_uid_name(int uid, char *buf);

#endif //FLUENT_BIT_PLUGINS_EXT_OSUTILS_H
