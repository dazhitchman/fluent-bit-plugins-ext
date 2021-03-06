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

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_input_plugin.h>
#include <fluent-bit/flb_sqldb.h>

#include "tail_db.h"
#include "tail_sql.h"
#include "tail_file.h"

struct query_status {
    int id;
    int rows;
    off_t offset;
};

/* Open or create database required by tail plugin */
struct flb_sqldb *flb_tailx_db_open(const char *path,
                                   struct flb_input_instance *in,
                                   struct flb_tailx_config *ctx,
                                   struct flb_config *config)
{
    int ret;
    char tmp[64];
    struct flb_sqldb *db;

    /* Open/create the database */
    db = flb_sqldb_open(path, in->name, config);
    if (!db) {
        return NULL;
    }

    /* Create table schema if it don't exists */
    ret = flb_sqldb_query(db, SQL_CREATE_FILES, NULL, NULL);
    if (ret != FLB_OK) {
        flb_plg_error(ctx->ins, "db: could not create 'in_tailx_files' table");
        flb_sqldb_close(db);
        return NULL;
    }

    if (ctx->db_sync >= 0) {
        snprintf(tmp, sizeof(tmp) - 1, SQL_PRAGMA_SYNC,
                 ctx->db_sync);
        ret = flb_sqldb_query(db, tmp, NULL, NULL);
        if (ret != FLB_OK) {
            flb_plg_error(ctx->ins, "db could not set pragma 'sync'");
            flb_sqldb_close(db);
            return NULL;
        }
    }


    ret = flb_sqldb_query(db, SQL_PRAGMA_JOURNAL_MODE, NULL, NULL);
    if (ret != FLB_OK) {
        flb_plg_error(ctx->ins, "db: could not set pragma 'journal_mode'");
        flb_sqldb_close(db);
        return NULL;
    }

    return db;
}

int flb_tailx_db_close(struct flb_sqldb *db)
{
    flb_sqldb_close(db);
    return 0;
}


static int cb_file_check(void *data, int argc, char **argv, char **cols)
{
    struct query_status *qs = data;

    qs->id = atoi(argv[0]);      /* id column */
    qs->offset = atoll(argv[2]); /* offset column */

    qs->rows++;
    return 0;
}

int flb_tailx_db_file_set(struct flb_tailx_file *file,
                         struct flb_tailx_config *ctx)
{
    int ret;
    char query[PATH_MAX];
    struct query_status qs = {0};
    uint64_t created;

    /* Check if the file exists */
    snprintf(query, sizeof(query) - 1, SQL_GET_FILE, (uint64_t) file->inode);
    memset(&qs, '\0', sizeof(qs));
    ret = flb_sqldb_query(ctx->db,
                          query, cb_file_check, &qs);
    if (ret == FLB_ERROR) {
        flb_plg_error(ctx->ins, "cannot execute SQL: %s", query);
        return -1;
    }

    if (qs.rows == 0) {
        /* Register the file */
        created = time(NULL);
        snprintf(query, sizeof(query) - 1,
                 SQL_INSERT_FILE,
                 file->name, (uint64_t) 0, (uint64_t) file->inode, created);
        ret = flb_sqldb_query(ctx->db, query, NULL, NULL);
        if (ret == FLB_ERROR) {
            flb_plg_error(ctx->ins, "cannot execute SQL: %s", query);
            return -1;
        }

        /* Get the database ID for this file */
        file->db_id = flb_sqldb_last_id(ctx->db);
        return 0;
    }

    file->db_id  = qs.id;
    file->offset = qs.offset;
    return 0;
}

/* Update Offset v2 */
int flb_tailx_db_file_offset(struct flb_tailx_file *file,
                            struct flb_tailx_config *ctx)
{
    int ret;

    /* Bind parameters */
    sqlite3_bind_int64(ctx->stmt_offset, 1, file->offset);
    sqlite3_bind_int64(ctx->stmt_offset, 2, file->db_id);

    ret = sqlite3_step(ctx->stmt_offset);

    sqlite3_clear_bindings(ctx->stmt_offset);
    sqlite3_reset(ctx->stmt_offset);

    if (ret != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

/* Mark a file as rotated */
int flb_tailx_db_file_rotate(const char *new_name,
                            struct flb_tailx_file *file,
                            struct flb_tailx_config *ctx)
{
    int ret;
    char query[PATH_MAX];
    struct query_status qs = {0};

    snprintf(query, sizeof(query) - 1,
             SQL_ROTATE_FILE,
             new_name, file->db_id);

    ret = flb_sqldb_query(ctx->db,
                          query, cb_file_check, &qs);
    if (ret != FLB_OK) {
        return -1;
    }

    return 0;
}

/* Delete file entry from the database */
int flb_tailx_db_file_delete(struct flb_tailx_file *file,
                            struct flb_tailx_config *ctx)
{
    int ret;
    char query[PATH_MAX];

    /* Check if the file exists */
    snprintf(query, sizeof(query) - 1, SQL_DELETE_FILE, file->db_id);
    ret = flb_sqldb_query(ctx->db, query, NULL, NULL);
    if (ret != FLB_OK) {
        flb_plg_error(ctx->ins, "db: error deleting entry from database: %s",
                      file->name);
        return -1;
    }

    flb_plg_debug(ctx->ins, "db: file deleted from database: %s", file->name);
    return 0;
}
