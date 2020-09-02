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

#ifndef FLB_IN_CPU_H
#define FLB_IN_CPU_H

#include <fluent-bit/flb_config.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_utils.h>
#include <sys/time.h>
#include <dynlist.h>
#include <fluent_kvp.h>

/* Default collection time: every 1 second (0 nanoseconds) */
#define DEFAULT_INTERVAL_SEC    1
#define DEFAULT_INTERVAL_NSEC   0
#define IN_CPU_KEY_LEN       16

struct cpu_key {
    uint8_t length;
    char name[IN_CPU_KEY_LEN];
};


typedef struct procsinfo {
    /* Process owner */
    uid_t uid;
    char username[64];
    /* Process details */
    int pi_pid;
    char pi_comm[64];
    char pi_state;
    int pi_ppid;
    int pi_pgrp;
    int pi_session;
    int pi_tty_nr;
    int pi_tty_pgrp;
    unsigned long pi_flags;
    unsigned long pi_minflt;
    unsigned long pi_child_min_flt;
    unsigned long pi_majflt;
    unsigned long pi_child_maj_flt;
    unsigned long pi_utime;
    unsigned long pi_stime;
    long pi_child_utime;
    long pi_child_stime;
    long pi_priority;
    long pi_nice;
    long pi_num_threads;
    long pi_it_real_value;
    unsigned long pi_start_time;
    unsigned long pi_vsize;
    long pi_rss;
    unsigned long pi_rsslimit;
    unsigned long pi_start_code;
    unsigned long pi_end_code;
    unsigned long pi_start_stack;
    unsigned long pi_esp;
    unsigned long pi_eip;
    /* The signal information here is obsolete. See "man proc" */
    unsigned long pi_signal_pending;
    unsigned long pi_signal_blocked;
    unsigned long pi_signal_ignore;
    unsigned long pi_signal_catch;
    unsigned long pi_wchan;
    unsigned long pi_swap_pages;
    unsigned long pi_child_swap_pages;
    int pi_signal_exit;
    int pi_last_cpu;
    unsigned long pi_realtime_priority;
    unsigned long pi_sched_policy;
    unsigned long long pi_delayacct_blkio_ticks;
    /* Process stats for memory */
    unsigned long statm_size;    /* total program size */
    unsigned long statm_resident;    /* resident set size */
    unsigned long statm_share;    /* shared pages */
    unsigned long statm_trs;    /* text (code) */
    unsigned long statm_drs;    /* data/stack */
    unsigned long statm_lrs;    /* library */
    unsigned long statm_dt;    /* dirty pages */

    /* Process stats for disks */
    unsigned long long read_io;    /* storage read bytes */
    unsigned long long write_io;    /* storage write bytes */
} procsinfo;

struct cpu_snapshot {
    /* data snapshots */
    char          v_cpuid[8];
    unsigned long v_user;
    unsigned long v_nice;
    unsigned long v_system;
    unsigned long v_idle;
    unsigned long v_iowait;

    /* percent values */
    double p_cpu;           /* Overall CPU usage        */
    double p_user;          /* user space (user + nice) */
    double p_system;        /* kernel space percent     */

    /* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
};



/* CPU Input configuration & context */
struct flb_cpu {
    /* setup */
    pid_t pid;          /* optional PID */
    int n_processors;   /* number of core processors  */
    int cpu_ticks;      /* CPU ticks (Kernel setting) */
    int coll_fd;        /* collector id/fd            */
    int interval_sec;   /* interval collection time (Second) */
    int interval_nsec;  /* interval collection time (Nanosecond) */
    int passes;
    DYNLIST * procstats;
    double previous_time;
    struct flb_input_instance *ins;
};


#define DELTA(member) (current->member - prev->member)

/*
 * This routine calculate the average CPU utilization of the system, it
 * takes in consideration the number CPU cores, so it return a value
 * between 0 and 100 based on 'capacity'.
 */
static inline double CPU_METRIC_SYS_AVERAGE(unsigned long pre,
                                            unsigned long now,
                                            struct flb_cpu *ctx)
{
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    //diff = ULL_ABS(now, pre);
    total = (((999 / ctx->cpu_ticks) * 100) / ctx->n_processors) / (ctx->interval_sec + 1e-9*ctx->interval_nsec);

    return total;
}




/* Returns the CPU % utilization of a given CPU core */
static inline double CPU_METRIC_PID_USAGE(unsigned long pre, unsigned long now,
                                          struct flb_cpu *ctx)
{
    double diff;
    double total = 0;

    if (pre == now) {
        return 0.0;
    }

    diff = ULL_ABS(now, pre);
    total = 100.0 * ((diff / ctx->cpu_ticks) / (ctx->interval_sec + 1e-9*ctx->interval_nsec));

    return total;
}

#endif
