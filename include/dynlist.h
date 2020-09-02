//
// Created by darren on 1/9/20.
//

#ifndef FLUENT_BIT_PLUGINS_EXT_DYNLIST_H
#define FLUENT_BIT_PLUGINS_EXT_DYNLIST_H


#include <stdlib.h>

typedef struct {
    size_t length;
    size_t count;
    void  *  * entries  ;
    __compar_fn_t comparator ;
} DYNLIST, * PDYNLIST;


DYNLIST *dynlist_create(__compar_fn_t comparator) ;

void dynlist_free_all(DYNLIST *dynlist) ;

int ensure_capacity(DYNLIST *dynlist) ;

DYNLIST *dynlist_add(DYNLIST *dynlist, void *entry);

void  * dynlist_get(DYNLIST *dynlist, int pos) ;

void  * dynlist_find(DYNLIST *dynlist, void * item);

int dynlist_indexOf(DYNLIST *dynlist, void * item) ;

int dynlist_linear_indexOf (DYNLIST * dynlist , void * item);

#define dynlist_foreach(list) \
    void * item; for (int idx = 0 ; idx < list->count ; idx ++, item =list->entries[idx])

#endif //FLUENT_BIT_PLUGINS_EXT_DYNLIST_H
