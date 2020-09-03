//
// Created by darren on 31/8/20.
//



#include "osutils.h"


int get_file_uid(FILE * file) {
    struct stat statbuf;
    int ret = fstat(fileno(file), &statbuf);
    if (ret == 0) {
        return statbuf.st_uid;
    }
    return -1;
}

int get_filename_uid(char *filename) {
    FILE * file = fopen(filename, "r");
    int uid=get_file_uid(file);
    fclose(file);
    return uid;
}

int get_uid_name(int uid, char *buf) {
    static struct passwd pw;
    static struct passwd *pwpointer;
    static char pwbuf[4096];
    int ret;
    if (uid == (uid_t) 0) {
        strcpy(buf, "root");
        return 4;
    }
    pwpointer = &pw;
    ret = getpwuid_r(uid, &pw, pwbuf, sizeof(pwbuf), &pwpointer);
    if (ret == 0 && pwpointer != 0) {
        unsigned int len = strlen(pw.pw_name);
        strncpy(buf, pw.pw_name, len);
        return len;
    }
    return 0;
}

void listdir(const char *fullpath, int depth ,int reftime , accept_filters * filters)
{
    DIR *dir;
    struct dirent *entry;
    int accept=0;
    char path[1024];
    struct stat statbuf;

    if (!(dir = opendir(fullpath)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            accept=1;

            if (filters){
                if (filters->user_accept_fn) {
                    accept = filters->user_accept_fn(fullpath, entry, depth, reftime, filters);
                }else {
                    if (depth > filters->max_depth) {
                        accept = 0;
                    }
                    if (accept && filters->dir_pattern){
                        accept=regexec(filters->dir_pattern, entry->d_name, (size_t) 0, NULL, 0)==0;
                    }
                }

            }
            if (accept){
                snprintf(path, sizeof(path), "%s/%s", fullpath, entry->d_name);
                listdir(path,depth+1,reftime,filters);
            }
        } else {
            if (filters){
                if (depth > filters->max_depth){
                    accept=0;
                }
                if (accept && filters->name_pattern) {
                    accept = regexec(filters->name_pattern, entry->d_name, (size_t) 0, NULL, 0) == 0;
                }

                if (accept && (filters->max_age || filters->min_age)){
                    snprintf(path, sizeof(path), "%s/%s", fullpath, entry->d_name);
                    FILE * file = fopen(path, "r");
                    int ret = fstat(fileno(file), &statbuf);
                    if (ret==0) {
                        if ( filters->max_age ) accept=reftime- statbuf.st_mtim.tv_sec < filters->max_age ;
                        if ( filters->min_age && accept ) accept=reftime- statbuf.st_mtim.tv_sec > filters->min_age ;
                    }
                    fclose(file);
                }

            }
            if (accept) { }

        }
    }
    closedir(dir);
}