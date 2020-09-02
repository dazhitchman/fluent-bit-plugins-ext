/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019-2020 The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_input_plugin.h>
#include <fluent-bit/flb_config.h>

#include <stdio.h>
#include <unistd.h>

#include <msgpack.h>
#include <fluent-bit/flb_time.h>
#include <ifaddrs.h>
#define LINUX
#ifdef LINUX
    #include <linux/if_link.h>
    #include <linux/if_packet.h>
#endif
#include "metadata.h"
#include "../include/kvp.h"
#include "../include/fluent_kvp.h"
#include "../include/strutils.h"


char *get_firstline_from_file(char *file, char *buf) {
    FILE *fp;
    //char buf[1024 + 1];

    if ((fp = fopen(file, "r")) != NULL) {
        if (fgets(buf, 1024, fp) != NULL) {
            if (buf[strlen(buf) - 1] == '\n')    /* remove last char = newline */
                buf[strlen(buf) - 1] = 0;
        }
        fclose(fp);
    }
    return buf;
}

char *get_uptime(char *buf) {
    FILE *fp;
    char secs[64];
    double uptime;
    if ((fp = fopen("/proc/uptime", "r")) != NULL) {
        fgets(buf,32,fp);
        if (sscanf(buf, "%lf", &uptime) != 1) {
            sprintf(buf, "NaN");
        }else {
            sprintf(buf, "%lf",uptime);
        }
        close(fp);
    }
    return buf;
}

DICT *get_interfaces() {

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];
    char buf[32];
    DICT *interfaces = dict_create("interfaces");

    if (getifaddrs(&ifaddr) == -1) {
        return NULL;
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        DICT *interface = dict_get(interfaces, ifa->ifa_name);
        if (interface == NULL) {
            dict_put_kvp(interfaces, ifa->ifa_name, interface = dict_create(ifa->ifa_name), DICTREF);
        }

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                dict_put_kvp(interface, (family == AF_INET) ? "inet" : "inet6", host, STR);
            }

        } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
            struct rtnl_link_stats *stats = ifa->ifa_data;
            struct sockaddr_ll *s = (struct sockaddr_ll *) ifa->ifa_addr;

            int len = 0;
            for (int i = 0; i < 6; i++)
                len += sprintf(&buf[len], "%02X%s", s->sll_addr[i], i < 5 ? ":" : "");
            buf[len] = 0;
            dict_put_kvp(interface, "ether", buf, STR);
        }
    }

    freeifaddrs(ifaddr);
    return interfaces;

}


DICT *get_metadata() {
    FILE *fp;
    char buf[1024 + 1];
    char *tmpptr;
    DICT *meta;

    unsigned long FLAGS = 0;


    //meta_data = dict_create("Metadata", ALLOCATE_MEMORY_FOR_NEW_TAGS);
    meta = dict_create("meta");
    dict_put_kvp(meta, "type", "metadata", STR);
    dict_put_kvp(meta, "fqn", (gethostname(buf, 256) == 0) ? buf : "unknown-hostname", STR);

    if (tmpptr = strchr(buf, '.')) {
        tmpptr[0] = 0;
    }
    dict_put_kvp(meta, "name", (tmpptr) ? tmpptr : buf, STR);

    if ((fp = popen("/usr/bin/lscpu", "r")) != NULL) {
        meta = parse_into_dict(fp, "([^:]*):\\s*(.*)", meta);
        char *archType = dict_get(meta, "Architecture");
        if (archType) {
            if (!strstr("ppc64", archType))
                FLAGS |= IS_POWER;
            if (!strstr("x86_64", archType))
                FLAGS |= IS_AMD64;
            if (!strstr("arm", archType))
                FLAGS |= IS_ARM;

        }
        pclose(fp);
    } else {
        dict_put_kvp(meta, "Architecture", "noArch-lscpu-failed", STR);
    }


    //FIXME get_firstline_from_file returns a dup string but we never free it. Should be OK for meta data


    if (FLAGS & IS_POWER) {
        if ((fp = fopen("/proc/ppc64/lparcfg", "r")) != NULL) {
            FLAGS |= IS_POWER_VM;
            meta = parse_into_dict(fp, "([^:]*):\\s*(.*)", meta);
            //FIXME remove = and replace with space
            fclose(fp);
        }
        dict_put_kvp(meta, "compatible", get_firstline_from_file("/proc/device-tree/compatible", buf), STR);
        dict_put_kvp(meta, "model", get_firstline_from_file("/proc/device-tree/model", buf), STR);
        dict_put_kvp(meta, "part-number", get_firstline_from_file("/proc/device-tree/part-number", buf), STR);
        dict_put_kvp(meta, "serial-number", get_firstline_from_file("/proc/device-tree/serial-number", buf), STR);
        dict_put_kvp(meta, "system-id", get_firstline_from_file("/proc/device-tree/system-id", buf), STR);
        dict_put_kvp(meta, "vendor", get_firstline_from_file("/proc/device-tree/vendor", buf), STR);
    }
    if (FLAGS & IS_POWER & IS_POWER_VM) {
        if (access("/proc/device-tree", R_OK) == 0) {
            dict_put_kvp(meta, "model", get_firstline_from_file("/proc/device-tree/model", buf), STR);
            dict_put_kvp(meta, "system-id", get_firstline_from_file("/proc/device-tree/system-id", buf), STR);
        }
    }


    if (!FLAGS & IS_POWER) {
        /* x86_64 and AMD64 and Arm - dmi files requires root user */
        if (access("/sys/devices/virtual/dmi/id/", R_OK) == 0) {
            dict_put_kvp(meta, "serial-number", get_firstline_from_file
                    ("/sys/devices/virtual/dmi/id/product_serial", buf), STR);
            char *sn = get_firstline_from_file("/sys/devices/virtual/dmi/id/product_name", buf);
            dict_put_kvp(meta, "model", sn == NULL || strlen(sn) == 0 ? "not-root" : sn, STR);
        }
    }

    if ((fp = fopen("/etc/os-release", "r")) != NULL) {
        parse_into_dict(fp, "([^=]*)=[\"']*(.*)[\"']$", meta);
        //FIXME   replace with spaces all double and single quotes

        fclose(fp);
    }

    dict_put_kvp(meta, "uptime", get_uptime(buf), STR);

    return meta;
}

void input_chunk_append_dict(const struct flb_input_instance *ins, const DICT *metadata,  struct flb_time *tm) {

    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;

    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);


    //msgpack_pack_array(&mp_pck, 2+(interfaces)?(interfaces->count):0);
    msgpack_pack_array(&mp_pck, 2);
    flb_time_append_to_msgpack(tm, &mp_pck, 0);
    pack_dict(metadata,  &mp_pck);


    flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
    msgpack_sbuffer_destroy(&mp_sbuf);
}

static int cb_metadata_collect_real(struct flb_input_instance *ins,
                               struct flb_config *config, void *in_context) {


    struct flb_time tm;
    char buf[1024];
    flb_metadata *ctx = (flb_metadata *) in_context;

    ctx->counter += 1;

    /* Get metadata into a dict */
    DICT *metadata = get_metadata();
    DICT * interfaces = get_interfaces();

    if (!metadata) {
        flb_plg_error(ins, "error retrieving metadata");
        return -1;
    }

    /* Get current time and add it to our msgPack */
    flb_time_get(&tm);

    input_chunk_append_dict(ins, metadata,  &tm);




    //pack_dict(metadata, NULL,&mp_pck);
    if (interfaces) {
        for (int i = 0, todo = interfaces->count; i < todo; i++) {
            PENTRY entry = interfaces->entry[i];
            input_chunk_append_dict(ins, entry->value, &tm);
        }
    }



    dict_free(metadata, true);
    dict_free(interfaces, true);
    return 0;
}


static int cb_metadata_collect(struct flb_input_instance *ins,
                               struct flb_config *config, void *in_context) {


    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;
    struct flb_time tm;

    flb_metadata *ctx = (flb_metadata *) in_context;

    ctx->counter += 1;


    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

    meta_fns items[] = {
            {.name= "metadata", .dict_fn=get_metadata},
            {.name= "interfaces", .dict_fn=get_interfaces}
    };



    /* Get current time and add it to our msgPack */
    flb_time_get(&tm);

    //msgpack_pack_array(&mp_pck, 2+(interfaces)?(interfaces->count):0);
    msgpack_pack_array(&mp_pck, 2);
    flb_time_append_to_msgpack(&tm, &mp_pck, 0);

    //INNER ARRAY WORKS .. msgpack_pack_array(&mp_pck, 2);
    unsigned int count = sizeof(items)/sizeof(meta_fns);
    msgpack_pack_map(&mp_pck,  count);
    for (int idx = 0 ; idx < count ; idx ++){
        char * name = items[idx].name;
        int nameLen = strlen(name);
        PDICT (*dict_fn)(void)  = items[idx].dict_fn;
        msgpack_pack_str(&mp_pck, nameLen);
        msgpack_pack_str_body(&mp_pck, name, nameLen);
        DICT * dict = dict_fn();
        pack_dict(dict,  &mp_pck);
        dict_free(dict,true);
    }

/*
    //Map with 2 keys.. meta and interfaces.
    msgpack_pack_str(&mp_pck, strlen("metadata"));
    msgpack_pack_str_body(&mp_pck, "metadata", strlen("metadata"));
    DICT * meta = get_metadata();
    pack_dict(meta, NULL, &mp_pck);
    dict_free(meta,true);

    //Next map, Interfaces
    msgpack_pack_str(&mp_pck, strlen("Interfaces"));
    msgpack_pack_str_body(&mp_pck, "Interfaces", strlen("Interfaces"));

    DICT * interfaces = get_interfaces();
    pack_dict(interfaces, NULL, &mp_pck);
    dict_free(interfaces,true);
*/
    flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
    msgpack_sbuffer_destroy(&mp_sbuf);
}


/* Init Metadata input */
static int cb_metadata_init(struct flb_input_instance *in,
                            struct flb_config *config, void *data) {

    int ret;
    flb_metadata *ctx;
    (void) data;
    const char *pval = NULL;

    /* Allocate space for the configuration */
    ctx = flb_calloc(1, sizeof(struct flb_metadata));
    if (!ctx) {
        flb_errno();
        return -1;
    }
    ctx->ins = in;
    ctx->counter = 0;

    int polling_interval = 1;
    /* Collection time setting */
    pval = flb_input_get_property("interval_sec", in);
    if (pval != NULL && atoi(pval) >= 0) {
        polling_interval = atoi(pval);
    }


    /* Set the context */
    flb_input_set_context(in, ctx);

    /* Set our collector based on time, CPU usage every 1 second */
    ret = flb_input_set_collector_time(in,
                                       cb_metadata_collect,
                                       polling_interval,
                                       0,
                                       config);
    if (ret == -1) {
        flb_plg_error(ctx->ins, "could not set collector for Metadata input plugin");
        return -1;
    }
    ctx->coll_fd = ret;


    return 0;
}

static void cb_metadata_pause(void *data, struct flb_config *config) {
    struct flb_metadata *ctx = data;
    flb_input_collector_pause(ctx->coll_fd, ctx->ins);
}

static void cb_metadata_resume(void *data, struct flb_config *config) {
    struct flb_metadata *ctx = data;
    flb_input_collector_resume(ctx->coll_fd, ctx->ins);
}

static int cb_metadata_exit(void *data, struct flb_config *config) {
    struct flb_cpu *ctx = data;

    /* done */
    flb_free(ctx);

    return 0;
}

/* Plugin reference */
struct flb_input_plugin in_metadata_plugin = {
        .name         = "metadata",
        .description  = "MetaData Collector",
        .cb_init      = cb_metadata_init,
        .cb_pre_run   = NULL,
        .cb_collect   = cb_metadata_collect,
        .cb_flush_buf = NULL,
        .cb_pause     = cb_metadata_pause,
        .cb_resume    = cb_metadata_resume,
        .cb_exit      = cb_metadata_exit
};
