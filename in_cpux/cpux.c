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
//#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_input_plugin.h>
#include <fluent-bit/flb_config.h>
#include <fluent-bit/flb_pack.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <msgpack.h>
#include <kvp.h>


#include "cpux.h"
#include "osutils.h"
#include "strutils.h"



static inline int get_procsinfo(struct flb_cpu *ctx, pid_t pid, procsinfo *proc) {

    FILE *fp;

    char filename[64];
    char buf[1024 * 4];
    char sm_buf[128];
    int size = 0;
    int ret = 0;
    int count = 0;
    int i;

    /* Read the process stats */
    snprintf(filename, sizeof(filename) - 1, "/proc/%d/stat", pid);
    fp = fopen(filename, "r");
    if (fp == NULL) {
        flb_plg_debug(ctx->ins, "error opening stats file %s presume proc ended or permissions ", buf);
        return -1;
    }

    if ( (size = fread( buf, 1,4096,fp))==-1){
        fclose(fp);
        return 0; // assuming process stopped
    }
    
    //s = (cstats->snap_active == CPU_SNAP_ACTIVE_A) ? cstats->snap_a: cstats->snap_b;
   

    errno = 0;

    ret = sscanf(buf, "%d (%s)", &proc->pi_pid,   &proc->pi_comm[0]);
    proc->pi_comm[strlen(proc->pi_comm)-1]=0;  // trim trailing )

    /* skip over ") "  */
    for (count = 0; count < size; count++) {
        if (buf[count] == ')' && buf[count + 1] == ' ')
            break;
    }
    if (count == size) {
        return 0; // failed to find end of command
    }
    count+=2;
    
    errno = 0;
    get_uid_name(get_file_uid(fp),sm_buf);

    ret = sscanf(&buf[count],
                 "%c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %lu %lu %llu",
                 &proc->pi_state,    /*3 numbers taken from "man proc" */
                 &proc->pi_ppid,    /*4 */
                 &proc->pi_pgrp,    /*5 */
                 &proc->pi_session,    /*6 */
                 &proc->pi_tty_nr,    /*7 */
                 &proc->pi_tty_pgrp,    /*8 */
                 &proc->pi_flags,    /*9 */
                 &proc->pi_minflt,    /*10 */
                 &proc->pi_child_min_flt,    /*11 */
                 &proc->pi_majflt,    /*12 */
                 &proc->pi_child_maj_flt,    /*13 */
                 &proc->pi_utime,    /*14 */
                 &proc->pi_stime,    /*15 */
                 &proc->pi_child_utime,    /*16 */
                 &proc->pi_child_stime,    /*18 */
                 &proc->pi_priority,    /*19 */
                 &proc->pi_nice,    /*20 */
                 &proc->pi_num_threads,    /*21 */
                 &proc->pi_it_real_value,    /*22 */
                 &proc->pi_start_time,    /*23 */
                 &proc->pi_vsize,    /*24 */
                 &proc->pi_rss,    /*25 */
                 &proc->pi_rsslimit,    /*26 */
                 &proc->pi_start_code,    /*27 */
                 &proc->pi_end_code,    /*28 */
                 &proc->pi_start_stack,    /*29 */
                 &proc->pi_esp,    /*29 */
                 &proc->pi_eip,    /*30 */
                 &proc->pi_signal_pending,    /*31 */
                 &proc->pi_signal_blocked,    /*32 */
                 &proc->pi_signal_ignore,    /*33 */
                 &proc->pi_signal_catch,    /*34 */
                 &proc->pi_wchan,    /*35 */
                 &proc->pi_swap_pages,    /*36 */
                 &proc->pi_child_swap_pages,    /*37 */
                 &proc->pi_signal_exit,    /*38 */
                 &proc->pi_last_cpu    /*39 */
            , &proc->pi_realtime_priority,    /*40 */
                 &proc->pi_sched_policy,    /*41 */
                 &proc->pi_delayacct_blkio_ticks    /*42 */
    );
    if (ret != 40) {
        //DEBUG fprintf(stderr, "procsinfo2 sscanf wanted 40 returned = %d pid=%d line=%s\n", ret, pid, buf);
        return 0;
    }
    snprintf(filename, 64, "/proc/%d/statm", pid);
    if ((fp = fopen(filename, "r")) == NULL) {
        //DEBUG fprintf(stderr, "failed to open file %s", filename);
        return 0;
    }
    size = fread( buf, 1,4096 ,fp);
    fclose(fp);            /* close it even if the read failed, the file could have been removed
				   between open & read i.e. the device driver does not behave like a file */

    if (size != -1) {
        ret = sscanf(&buf[0], "%lu %lu %lu %lu %lu %lu %lu",
                     &proc->statm_size,
                     &proc->statm_resident,
                     &proc->statm_share,
                     &proc->statm_trs,
                     &proc->statm_lrs,
                     &proc->statm_drs, &proc->statm_dt);
        if (ret != 7) {
            return 0;
        }
    }
    /*
    if (uid == (uid_t) 0) {
        p->procs[index].read_io = 0;
        p->procs[index].write_io = 0;
        sprintf(filename, "/proc/%d/io", pid);
        if ((fp = fopen(filename, "r")) != NULL) {
            for (i = 0; i < 6; i++) {
                if (fgets(buf, 1024, fp) == NULL) {
                    break;
                }
                if (strncmp("read_bytes:", buf, 11) == 0)
                    sscanf(&buf[12], "%lld", &procinfo.read_io);
                if (strncmp("write_bytes:", buf, 12) == 0)
                    sscanf(&buf[13], "%lld", &procinfo.write_io);
            }
        }

        if (fp != NULL)
            fclose(fp);
    }
     */
    return 1;
}


/* qsort compare func for pids */
int pid_compare(const void *a, const void *b)
{

    return (int) (((procsinfo  *) b)->pi_pid -
                  ((procsinfo  *) a)->pi_pid);
}


int get_procs(struct flb_cpu *ctx,DYNLIST * list , int max_records) {

    struct dirent *dent;
    DIR *procdir;
    int done = 0;


    if ((char *) (procdir = opendir("/proc")) == NULL) {
        flb_errno();
        flb_plg_error(ctx->ins, "failed to open /proc dir");
        return 0;
    }

    while ((char *) (dent = readdir(procdir)) != NULL) {
        if (dent->d_type == 4) {    /* is this a directlory */
            if (isnumbers(dent->d_name)) {
                if (done < max_records) {
                    procsinfo * proc = calloc(1,sizeof(struct procsinfo));
                    if (! get_procsinfo( ctx, atoi(dent->d_name), proc)){
                        free(proc);
                    }else{
                        dynlist_add(list,proc);
                    }
                    done++;
                }else{
                    break;
                }
            }
        }
    }
    closedir(procdir);

    return done;
}

void write_procsinfo( procsinfo prev, procsinfo current ,double elapsed,int pagesize,msgpack_packer * mp_pck){
#define TIMEDELTA(member) (current.member - prev.member)
#define COUNTDELTA(member)  ( (prev.member > current.member) ? 0 : current.member - prev.member)

    msgpack_pack_map(mp_pck, 4);

    pack_head_key(mp_pck,"cpu");
    msgpack_pack_map(mp_pck, 2);
    pack_double(mp_pck,"u", TIMEDELTA(pi_utime) / elapsed);
    pack_double (mp_pck,"s", TIMEDELTA(pi_stime) / elapsed);


    pack_head_key(mp_pck,"proc");
    msgpack_pack_map(mp_pck, 11);
    pack_long(mp_pck, "pid", current.pi_pid);
    pack_str(mp_pck, "cmd", current.pi_comm);
    pack_long(mp_pck,"ppid", current.pi_ppid);
    pack_long(mp_pck,"pgrp", current.pi_pgrp);
    pack_long(mp_pck,"pri", current.pi_priority);
    pack_long(mp_pck,"nice", current.pi_nice);
    pack_long(mp_pck,"session", current.pi_session);
    pack_long(mp_pck,"tty_nr", current.pi_tty_nr);
    pack_long(mp_pck,"flags", current.pi_flags);
    pack_char (mp_pck,"state", current.pi_state); // cast int as char ?
    pack_double (mp_pck,"uptime", (double) current.pi_start_time /  (double) sysconf(_SC_CLK_TCK));

    pack_head_key(mp_pck,"mem");
    msgpack_pack_map(mp_pck, 6);
    pack_long(mp_pck,"size", current.statm_size * pagesize / 1024);
    pack_long(mp_pck,"rss", current.statm_resident * pagesize / 1024);
    pack_long(mp_pck,"rtxt", current.statm_trs * pagesize / 1024);
    pack_long(mp_pck,"rdat", current.statm_drs * pagesize / 1024);
    pack_long(mp_pck,"share", current.statm_share * pagesize / 1024);
    pack_longlong(mp_pck,"vsize", (long long) (current.pi_vsize) / 1024);

    pack_head_key(mp_pck,"pages");
    msgpack_pack_map(mp_pck, 4);
    pack_long(mp_pck,"rss", current.pi_rss);
    pack_long(mp_pck,"rss_limit", current.pi_rsslimit);
    pack_long(mp_pck,"swap", current.pi_swap_pages);
    pack_long(mp_pck,"child_swap", current.pi_child_swap_pages);

    return ;

    pack_long(mp_pck, "pid", current.pi_pid);
    pack_str(mp_pck, "cmd", current.pi_comm);



    msgpack_pack_map(mp_pck, 3);

    pack_head_key(mp_pck,"cpu");
    msgpack_pack_map(mp_pck, 4);
    pack_double(mp_pck,"usr", TIMEDELTA(pi_utime) / elapsed);
    pack_double (mp_pck,"sys", TIMEDELTA(pi_stime) / elapsed);
    pack_double (mp_pck,"usr_secs",current.pi_utime / (double) sysconf(_SC_CLK_TCK));
    pack_double (mp_pck,"sys_secs",current.pi_stime / (double) sysconf(_SC_CLK_TCK));

    pack_head_key(mp_pck,"mem");
    msgpack_pack_map(mp_pck, 10);
    pack_long(mp_pck,"size_kb", current.statm_size * pagesize / 1024);
    pack_long(mp_pck,"resident_kb", current.statm_resident * pagesize / 1024);
    pack_long(mp_pck,"restext_kb", current.statm_trs * pagesize / 1024);
    pack_long(mp_pck,"resdata_kb", current.statm_drs * pagesize / 1024);
    pack_long(mp_pck,"share_kb", current.statm_share * pagesize / 1024);
    pack_longlong(mp_pck,"virtual_size_kb", (long long) (current.pi_vsize) / 1024);
    pack_long(mp_pck,"rss_pages", current.pi_rss);
    pack_long(mp_pck,"rss_limit", current.pi_rsslimit);
    pack_long(mp_pck,"swap_pages", current.pi_swap_pages);
    pack_long(mp_pck,"child_swap_pages", current.pi_child_swap_pages);

    pack_head_key(mp_pck,"proc");
    msgpack_pack_map(mp_pck, 10);
//    add_msgpack_double (mp_pck,"cpu_percent", topper[entry].time / elapsed);
    pack_long(mp_pck, "pid", current.pi_pid);
    pack_str(mp_pck, "cmd", current.pi_comm);
    pack_long(mp_pck,"ppid", current.pi_ppid);
    pack_long(mp_pck,"pgrp", current.pi_pgrp);
    pack_long(mp_pck,"priority", current.pi_priority);
    pack_long(mp_pck,"nice", current.pi_nice);
    pack_long(mp_pck,"session", current.pi_session);
    pack_long(mp_pck,"tty_nr", current.pi_tty_nr);
    pack_long(mp_pck,"flags", current.pi_flags);
    pack_char (mp_pck,"state", current.pi_state); // cast int as char ?
    pack_double (mp_pck,"starttime_secs", (double) current.pi_start_time /  (double) sysconf(_SC_CLK_TCK));


/*
    pack_long(mp_pck, "threads", current.pi_num_threads);
    pack_double (mp_pck,"minorfault", COUNTDELTA(pi_minflt) / elapsed);
    pack_double (mp_pck,"majorfault", COUNTDELTA(pi_majflt) / elapsed);
    pack_long(mp_pck,"it_real_value", current.pi_it_real_value);
    pack_long(mp_pck,"last_cpu", current.pi_last_cpu);
    pack_long(mp_pck,"realtime_priority", current.pi_realtime_priority);
    pack_long(mp_pck,"sched_policy", current.pi_sched_policy);
    pack_double (mp_pck,"delayacct_blkio_secs", (double) current.pi_delayacct_blkio_ticks /  (double) sysconf(_SC_CLK_TCK));
    pack_long(mp_pck,"uid", current.uid);
    pack_str(mp_pck,"username", current.username);

*/
}



/* Callback to gather CPU usage between now and previous snapshot */
static int cb_cpux_collect(struct flb_input_instance *ins,
                          struct flb_config *config, void *in_context) {
    struct flb_cpu *ctx = in_context;
    ctx->passes++;
    if (ctx->passes>10){
        exit(1);
    }
    double elapsed;

    double current_time;
    struct timeval tv;

    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;
    struct flb_time tm;

    /* Get current time and add it to our msgPack */
    flb_time_get(&tm);

    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);


    //msgpack_pack_array(&mp_pck, 2+(interfaces)?(interfaces->count):0);
    msgpack_pack_array(&mp_pck, 2);
    flb_time_append_to_msgpack(&tm, &mp_pck, 0);

    gettimeofday(&tv, 0);
    current_time = (double) tv.tv_sec + (double) tv.tv_usec * 1.0e-6;
    elapsed = current_time - ctx->previous_time ;
    ctx->previous_time = current_time;

    DYNLIST * list = dynlist_create(&pid_compare);
    DYNLIST * prevlist = ctx->procstats;

    get_procs(ctx, list ,2048);
    // compare and get deltas for each proc
    //TODO Get PAGE_SIZE
    int PAGE_SIZE=4096;

    msgpack_pack_map(&mp_pck, list->count);

    void * item;
    for (int idx = 0 ; idx < list->count ; idx ++ ){
        item =list->entries[idx];
        struct procsinfo * current = (struct procsinfo *)item;
        struct procsinfo * prev = dynlist_find(prevlist,current);
        if (prev){
            //calculate diff values
            msgpack_pack_map(&mp_pck, 1);
            pack_long(&mp_pck, "pid", current->pi_pid);
            write_procsinfo(*prev,*current,elapsed,PAGE_SIZE,&mp_pck);

           /* double cputime = (current->pi_utime-prev->pi_utime) + (current->pi_stime-prev->pi_stime);
            if (prev->pi_pid==2823) { //if (cputime>0) {
                printf("%s cputime=%f timeadj=%f ticks=%f pct=%f \n", prev->pi_comm, cputime, cputime / elapsed,
                       ctx->cpu_ticks / elapsed, 100);
            }
            */
        }else {
            msgpack_pack_map(&mp_pck, 1);
            pack_long(&mp_pck, "pid", current->pi_pid);

            msgpack_pack_map(&mp_pck, 1);
            pack_str(&mp_pck, "cmd", current->pi_comm);
            //write meta data
        }
    }
    dynlist_free_all(prevlist);
    ctx->procstats=list;


    printf("Writing data bytes=%zu\n", mp_sbuf.size);
    flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
    msgpack_sbuffer_destroy(&mp_sbuf);

    return 0;
}



/* Init CPU input */
static int cb_cpux_init(struct flb_input_instance *in,
                       struct flb_config *config, void *data) {
    int ret;
    struct flb_cpu *ctx;
    (void) data;
    const char *pval = NULL;

    /* Allocate space for the configuration */
    ctx = flb_calloc(1, sizeof(struct flb_cpu));
    if (!ctx) {
        flb_errno();
        return -1;
    }
    ctx->ins = in;
    ctx->passes=0;

    /* Gather number of processors and CPU ticks */
    ctx->n_processors = sysconf(_SC_NPROCESSORS_ONLN);
    ctx->cpu_ticks = sysconf(_SC_CLK_TCK);

    /* Process ID */
    pval = flb_input_get_property("pid", in);
    if (pval) {
        ctx->pid = atoi(pval);
    } else {
        ctx->pid = -1;
    }

    struct timeval tv;
    gettimeofday(&tv, 0);
    ctx->previous_time =  (double) tv.tv_sec + (double) tv.tv_usec * 1.0e-6;

    /* Collection time setting */
    pval = flb_input_get_property("interval_sec", in);
    if (pval != NULL && atoi(pval) >= 0) {
        ctx->interval_sec = atoi(pval);
    } else {
        ctx->interval_sec = DEFAULT_INTERVAL_SEC;
    }

    pval = flb_input_get_property("interval_nsec", in);
    if (pval != NULL && atoi(pval) >= 0) {
        ctx->interval_nsec = atoi(pval);
    } else {
        ctx->interval_nsec = DEFAULT_INTERVAL_NSEC;
    }

    if (ctx->interval_sec <= 0 && ctx->interval_nsec <= 0) {
        /* Illegal settings. Override them. */
        ctx->interval_sec = DEFAULT_INTERVAL_SEC;
        ctx->interval_nsec = DEFAULT_INTERVAL_NSEC;
    }

    /*get proc list */
    DYNLIST * list = dynlist_create(&pid_compare);

    get_procs(ctx, list ,2048);
    if (!list) {
        flb_error("[cpu] Could not obtain /proc/{pid}/stats data");
        flb_free(ctx);
        return -1;
    }
    ctx->procstats = list;

    /* Set the context */
    flb_input_set_context(in, ctx);

    /* Set our collector based on time, CPU usage every 1 second */
    ret = flb_input_set_collector_time(in,
                                       cb_cpux_collect,
                                       ctx->interval_sec,
                                       ctx->interval_nsec,
                                       config);
    if (ret == -1) {
        flb_plg_error(ctx->ins, "could not set collector for CPU input plugin");
        return -1;
    }
    ctx->coll_fd = ret;

    return 0;
}

static void cb_cpux_pause(void *data, struct flb_config *config) {
    struct flb_cpu *ctx = data;
    flb_input_collector_pause(ctx->coll_fd, ctx->ins);
}

static void cb_cpux_resume(void *data, struct flb_config *config) {
    struct flb_cpu *ctx = data;
    flb_input_collector_resume(ctx->coll_fd, ctx->ins);
}

static int cb_cpux_exit(void *data, struct flb_config *config) {
    (void) *config;
    struct flb_cpu *ctx = data;
    struct cpu_stats *cs;

    /* Release snapshots */
    dynlist_free_all(ctx->procstats);

    /* done */
    flb_free(ctx);

    return 0;
}

/* Plugin reference */
struct flb_input_plugin in_cpux_plugin = {
        .name         = "cpux",
        .description  = "CPU Usage (Extended Stats)",
        .cb_init      = cb_cpux_init,
        .cb_pre_run   = NULL,
        .cb_collect   = cb_cpux_collect,
        .cb_flush_buf = NULL,
        .cb_pause     = cb_cpux_pause,
        .cb_resume    = cb_cpux_resume,
        .cb_exit      = cb_cpux_exit
};
