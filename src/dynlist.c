//
// Created by darren on 1/9/20.
//

#include "dynlist.h"
#include "../in_cpux/cpux.h"

//
// Created by darren on 4/8/20.
// NOT a SET , Nothing happening with regards to sorted / binary search , hashing buckets etc... its just a list of ENTRY
//

#include <stdio.h>
#include <string.h>
#include <fluent-bit.h>

//static const size_t tag_size = sizeof(ENTRY *);


DYNLIST *dynlist_create(__compar_fn_t comparator) { //sort fn / comparable fn
    DYNLIST *dict = calloc(1,sizeof(DYNLIST));
    dict->comparator=comparator;
    return dict;
}


void dynlist_free_all(DYNLIST *dynlist) {
    if (!dynlist) {
        return;
    }
    
    for (int i = 0; i < dynlist->count; i++) {
        void * node=dynlist->entries[i];
        if (node) {
            free(node);
        }
    }
    free(dynlist->entries);
    free(dynlist);
}

int dynlist_ensure_capacity(DYNLIST *dynlist) {
    const int to_add = 16;

    if (dynlist->count == dynlist->length) {

        dynlist->entries = (dynlist->count) ? realloc(dynlist->entries, (dynlist->length + to_add) * sizeof (void *) ) : calloc(sizeof (void *),
                                                                                                           to_add);

        if (dynlist->entries) {
            dynlist->length += to_add;
        } else {
            return 0;
        }
    }
    return 1;
}

int dynlist_insertpos(DYNLIST *dynlist, void *entry, int * is_exact_match){
    if (dynlist->count==0){
        return 0;
    }
    if (! dynlist->comparator){
        //no comparator , so not a sorted list
        return dynlist->count;
    }

    int left = 0;
    int right =  dynlist->count-1;
    int middle = 0;


    while (left <= right){
        int middle = left + (right- left )/2;
        int compare=dynlist->comparator(dynlist->entries[middle],entry);

        if (compare==0 ){
            if (is_exact_match){
                *is_exact_match= 1;
            }
            return middle;
        }
        if (left==right){
            if (is_exact_match){
                *is_exact_match= 0;
            }
            return (compare>0) ? middle +1 : middle;
        }

        if (compare > 0)
            left = middle + 1;
        else
            right = middle - 1;
    }
    return middle;


}

DYNLIST *dynlist_add(DYNLIST *dynlist, void *entry) {
    if (!entry || !dynlist) {
        return dynlist;
    }

    unsigned int todo;
    if (dynlist_ensure_capacity(dynlist)) {
        int pos = dynlist_insertpos(dynlist, entry,NULL);

        void * * ent  = dynlist->entries;
        ent+=(pos);
        if (pos < dynlist->count) {
            //make room, shuffle up
            memmove(ent+1, ent, sizeof(void *) * (dynlist->count - pos));
        }
        dynlist->entries[pos]=entry;
        dynlist->count++;
    }
    return dynlist;
}

void  * dynlist_get(DYNLIST *dynlist, int pos) {
    if (dynlist && dynlist->count && pos < dynlist->count && pos >= 0) {
        return dynlist->entries[pos];
    }
    return NULL;
}



int dynlist_indexOf(DYNLIST *dynlist, void * item) {
    if (!dynlist || dynlist->count ==0){
        return -1;
    }
    int match=0;
    int pos = dynlist_insertpos(dynlist, item,&match);
    return (match) ? pos : -1;
}

void * dynlist_find (DYNLIST * dynlist , void * item){
    int pos = dynlist_indexOf(dynlist,item);
    return (pos==-1) ? NULL : dynlist_get(dynlist,pos);
}
int dynlist_linear_indexOf (DYNLIST * dynlist , void * item){
    if (!dynlist) {
        return -1;
    }

    for (int i = 0; i < dynlist->count; i++) {
        void * node=dynlist->entries[i];
        if (dynlist->comparator(node,item)==0){
            return i;
        }
    }
    return -1;
}