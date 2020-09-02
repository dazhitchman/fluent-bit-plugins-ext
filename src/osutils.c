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