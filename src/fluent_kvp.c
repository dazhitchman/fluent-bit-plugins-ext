//
// Created by darren on 28/8/20.
//

#include "fluent_kvp.h"

void pack_key(PENTRY entry, msgpack_packer * mp_pck){
    msgpack_pack_str(mp_pck, entry->keylen);
    msgpack_pack_str_body(mp_pck, entry->key, entry->keylen);

}
void pack_val (PENTRY entry, msgpack_packer * mp_pck){

    switch (entry->type) {
        case DICTREF :
        case PTR :
        case FREEABLE_PTR:
        case DOUBLE :
        case LONGLONG :
            msgpack_pack_double(mp_pck,*(double *)(entry->value) );
            break;
        case STRING :
            msgpack_pack_str(mp_pck, entry->vallen);
            msgpack_pack_str_body(mp_pck, entry->value, entry->vallen);

            break;
        case LONG :
            msgpack_pack_long(mp_pck,(long)(entry->value) );
            break;
        case CHAR :
            msgpack_pack_char(mp_pck,(char)(entry->value) );
            break;
        default:
            assert("Unknown"=="hardfail");
    }
}

void pack_entry (PENTRY entry, msgpack_packer * mp_pck){
    pack_key(entry,mp_pck);
    pack_val(entry,mp_pck);

}


/* Flattern the dict to suport fluent-bit types*/

int pack_dict(DICT * dict, msgpack_packer * mp_pck ) {

    msgpack_pack_map(mp_pck, dict->count);

    for (int i = 0, todo = dict->count; i < todo; i++) {
        PENTRY entry = dict->entry[i];
        pack_key(entry, mp_pck);
        if (entry->type == DICTREF) {
           pack_dict((DICT *) entry->value, mp_pck);
        } else {
            pack_val( entry,mp_pck);
        }
    }

return 0;
}

void pack_head_key( msgpack_packer * mp_pck,char * key){
    int len = strlen(key);
    msgpack_pack_str(mp_pck, len);
    msgpack_pack_str_body(mp_pck, key,len);
}

void pack_long( msgpack_packer * mp_pck,char * key,long val){
    pack_head_key(mp_pck,key);
    msgpack_pack_long(mp_pck,val);
}

void pack_longlong( msgpack_packer * mp_pck,char * key,long long val){
    pack_head_key(mp_pck,key);
    msgpack_pack_long_long(mp_pck,val);
}


void pack_double( msgpack_packer * mp_pck,char * key,double val){
    pack_head_key(mp_pck,key);
    msgpack_pack_double(mp_pck,val );
}
void pack_char( msgpack_packer * mp_pck,char * key,char val){
    pack_head_key(mp_pck,key);
    msgpack_pack_char(mp_pck,val );
}
void pack_str( msgpack_packer * mp_pck,char * key,char * val){
    pack_head_key(mp_pck,key);
    pack_head_key(mp_pck,val);
}
