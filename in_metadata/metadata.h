#ifndef CAPM_METADATA_H
#define CAPM_METADATA_H


#include <kvp.h>

#define IS_POWER  1
#define IS_AMD64 2
#define IS_ARM 4
#define IS_POWER_VM  8

/* CPU Input configuration & context */
typedef struct flb_metadata {
    /* setup */
    int coll_fd;        /* collector id/fd            */
    struct flb_input_instance *ins;
    int counter;
}flb_metadata;


typedef struct meta_fns {
    char * name ;
    PDICT (*dict_fn)(void) ;
}meta_fns;
#endif