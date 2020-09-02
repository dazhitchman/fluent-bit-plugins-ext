//
// Created by darren on 28/8/20.
//

#ifndef FLUENT_BIT_PLUGINS_EXT_FLUENT_KVP_H
#define FLUENT_BIT_PLUGINS_EXT_FLUENT_KVP_H

#include <fluent-bit.h>
#include "kvp.h"


int pack_dict(DICT * dict,  msgpack_packer * mp_pck);

void pack_entry (PENTRY entry, msgpack_packer * mp_pck);


void pack_long( msgpack_packer * mp_pck,char * key,long val);
void pack_longlong( msgpack_packer * mp_pck,char * key,long long val);

void pack_double( msgpack_packer * mp_pck,char * key,double val);
void pack_char( msgpack_packer * mp_pck,char * key,char val);

void pack_str( msgpack_packer * mp_pck,char * key,char * val);
void pack_head_key( msgpack_packer * mp_pck,char * key);
#endif //FLUENT_BIT_PLUGINS_EXT_FLUENT_KVP_H
