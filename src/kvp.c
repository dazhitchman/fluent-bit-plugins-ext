//
// Created by darren on 4/8/20.
// NOT a SET , Nothing happening with regards to sorted / binary search , hashing buckets etc... its just a list of ENTRY
//

#include "../include/kvp.h"
#include <stdio.h>
#include <string.h>
#include <fluent-bit.h>

//static const size_t tag_size = sizeof(ENTRY *);
static const size_t entry_size = sizeof(ENTRY );
static const size_t pentry_size = sizeof(ENTRY *);

static const size_t tags_size = sizeof(DICT);


DICT *dict_create(const char *name) {
    DICT *dict = calloc(tags_size, 1);
    dict->name = (name) ? strdup(name) : NULL;
    return dict;
}


void dict_free(DICT *dict, unsigned int recursive) {
    if (!dict) {
        return;
    }
    PENTRY node=dict->entry[0];


    for (int i = 0; i < dict->count; i++) {
        node=dict->entry[i];
        if (node){
            free(node->key);
            if (node->value) {
                switch (node->type) {
                    case PTR :
                        break;
                    case CHAR:
                    case DOUBLE:
                    case LONGLONG :
                        free(node->value);
                        break;
                    case DICTREF :
                        if (recursive) {
                            dict_free(node->value, recursive);
                            //free(entry->value);
                        }
                        break;
                };
            }
            free(node);
        }
    }
   free(dict->entry);
   if (dict->name) {
        free(dict->name);
   }
   free(dict);
}

int ensure_capacity(DICT *dict) {
    const int to_add = 16;

    if (dict->count == dict->length) {

            dict->entry = (dict->count) ? realloc(dict->entry, (dict->length + to_add) * pentry_size) : calloc(pentry_size,
                                                                                                             to_add);

        if (dict->entry) {
            dict->length += to_add;
        } else {
            return 0;
        }
    }
    return 1;
}

/*
void get_tag(char *key,  char *value , int memory_mgmt_policy) {
    ENTRY *tag;
}

ENTRY * tag_setkey(ENTRY * tag , char * key , int memory_mgmt_policy){
    if (!tag){
        tag=calloc(1,tag_size);
    }
    switch (memory_mgmt_policy) {
        case REF :
        case FREE : tag->key=key ; break;

        case DUP : (key)? strdup(key): NULL ; break;
    }

    tag->key=(key)? strdup(key): NULL;
    tag->keymem=memory_mgmt_policy;
    return tag;
}
*/

PENTRY dict_entry_create(char *key, void *value, enum value_type type) {
    PENTRY entry;
    entry = calloc(1, entry_size);
    if (!entry) {
        return NULL;
    }
    entry->key = (key) ? strdup(key) : strdup("");  //todo const char NULL_KEY rather than ""
    entry->type = type;
    switch (type) {
        case DICTREF :
        case PTR :
            entry->value = value;
            break;
        case CHAR :
            entry->value = (value) ? strdup(value) : NULL;
            break;
        default  :
            //FIX ME . Currently we pass in formatted char * rather than raw value.. This needs to change
            entry->value = (value) ? strdup(value) : NULL;
            break;
    }
    return entry;
}



DICT *dict_put_kvp(DICT *dict, char *key, char *value, enum value_type type) {
    if (!dict){
        return NULL;
    }
    return dict_put(dict, dict_entry_create(key, value, type));

}

DICT *dict_put(DICT *dict, ENTRY *entry) {
    if (!entry || !dict) {
        return dict;
    }

    unsigned int todo;
    if (ensure_capacity(dict)) {
        dict->entry[dict->count]=entry;
        dict->count++;
    }
    return dict;
}

ENTRY *dict_get_entry(DICT *dict, char *key) {
    if (dict && key && dict->count) {
        ENTRY * * entry = dict->entry;
        for (int i = 0; i < dict->count; i++, entry++) {
            if (!strcmp((*entry)->key, key)) {
                return *entry;
            }
        }
    }

    return NULL;
}
void *dict_get_default(DICT *dict, char *key, void * defolt) {
    void * ret = dict_get(dict,key);
    return (ret) ? ret  : defolt;
}

char *dict_getchar_default(DICT *dict, char *key, char * defolt) {
    char * ret = (char *)dict_get(dict,key);
    return (ret) ? ret  : defolt;
}

void *dict_get(DICT *dict, char *key) {
    ENTRY * entry= dict_get_entry(dict,key);
    return (entry) ? entry->value : NULL;

}



int dict_to_msgpack(DICT * dict,msgpack_packer * mp_pck) {
    int tmpint = 0;


    msgpack_pack_map (mp_pck,dict->count);

    for (int i = 0, todo = dict->count; i < todo; i++) {
        PENTRY entry = dict->entry[i];
        if (entry->type == DICTREF) {
            dict_to_msgpack((DICT *) entry->value, mp_pck);
        } else {
            /* append new keys */
            tmpint = strlen(entry->key);
            msgpack_pack_str(mp_pck, tmpint);
            msgpack_pack_str_body(mp_pck, entry->key, tmpint);
            switch (entry->type) {
                case CHAR :
                    tmpint = strlen(entry->value);
                    msgpack_pack_str(mp_pck, tmpint);
                    msgpack_pack_str_body(mp_pck, entry->value, tmpint);
                    break;
                default:
                    //not implemented
                    assert(false);

            }
        }
    }
    return 0;
}