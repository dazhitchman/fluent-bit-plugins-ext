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

#include "metadata.h"
#include "../include/kvp.h"
#include "../include/strutils.h"


char *get_firstline_from_file(char *file,char * buf) {
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

DICT *get_metadata() {
    FILE *fp ;
    char buf[1024 + 1];
    char *tmpptr;
    DICT *meta;

    unsigned long FLAGS = 0;


    //meta_data = dict_create("Metadata", ALLOCATE_MEMORY_FOR_NEW_TAGS);
    meta = dict_create("meta");
    dict_put_kvp(meta, "fullhostname", (gethostname(buf, 256) == 0) ? buf : "unknown-hostname", CHAR);

    tmpptr = strchr(buf, '.');
    if (tmpptr) {
        tmpptr[0] = 0;
    }
    dict_put_kvp(meta, "hostname", (tmpptr) ? tmpptr : buf, CHAR);

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
        dict_put_kvp(meta, "Architecture", "noArch-lscpu-failed", CHAR);
    }

    //FIXME get_firstline_from_file returns a dup string but we never free it. Should be OK for meta data


    if (FLAGS & IS_POWER) {
        if ((fp = fopen("/proc/ppc64/lparcfg", "r")) != NULL) {
            FLAGS |= IS_POWER_VM;
            meta = parse_into_dict(fp, "([^:]*):\\s*(.*)", meta);
            //FIXME remove = and replace with space
            fclose(fp);
        }
        dict_put_kvp(meta, "compatible", get_firstline_from_file("/proc/device-tree/compatible",buf), CHAR);
        dict_put_kvp(meta, "model", get_firstline_from_file("/proc/device-tree/model",buf), CHAR);
        dict_put_kvp(meta, "part-number", get_firstline_from_file("/proc/device-tree/part-number",buf), CHAR);
        dict_put_kvp(meta, "serial-number", get_firstline_from_file("/proc/device-tree/serial-number",buf), CHAR);
        dict_put_kvp(meta, "system-id", get_firstline_from_file("/proc/device-tree/system-id",buf), CHAR);
        dict_put_kvp(meta, "vendor", get_firstline_from_file("/proc/device-tree/vendor",buf), CHAR);
    }
    if (FLAGS & IS_POWER & IS_POWER_VM) {
        if (access("/proc/device-tree", R_OK) == 0) {
            dict_put_kvp(meta, "model", get_firstline_from_file("/proc/device-tree/model",buf), CHAR);
            dict_put_kvp(meta, "system-id", get_firstline_from_file("/proc/device-tree/system-id",buf), CHAR);
        }
    }


    if (!FLAGS & IS_POWER) {
        /* x86_64 and AMD64 and Arm - dmi files requires root user */
        if (access("/sys/devices/virtual/dmi/id/", R_OK) == 0) {
            dict_put_kvp(meta, "serial-number", get_firstline_from_file
                    ("/sys/devices/virtual/dmi/id/product_serial",buf), CHAR);
            char *sn = get_firstline_from_file("/sys/devices/virtual/dmi/id/product_name",buf);
            dict_put_kvp(meta, "model", sn == NULL || strlen(sn) == 0 ? "not-root" : sn, CHAR);
        }
    }

    if ((fp = fopen("/etc/os-release", "r")) != NULL) {
        parse_into_dict(fp, "([^=]*)=[\"']*(.*)[\"']$", meta);
        //FIXME   replace with spaces all double and single quotes

        fclose(fp);
    }
    return meta;
}


static int cb_metadata_collect(struct flb_input_instance *ins,
                               struct flb_config *config, void *in_context) {

    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;
    struct flb_time tm;
    char buf [1024];
    flb_metadata *ctx = (flb_metadata * ) in_context;

    ctx->counter+=1;

    /* Get metadata into a dict */
    DICT *metadata = get_metadata();


    if (!metadata ) {
        flb_plg_error(ins, "error retrieving overall system CPU stats");
        return -1;
    }


    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

    /* Get current time and add it to our msgPack */
    flb_time_get(&tm);
    msgpack_pack_array(&mp_pck, 2);
    flb_time_append_to_msgpack(&tm, &mp_pck, 0);

    dict_to_msgpack(metadata, &mp_pck);

    flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
    msgpack_sbuffer_destroy(&mp_sbuf);

    dict_free(metadata,true);
    return 0;
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
    ctx->counter=0;

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
