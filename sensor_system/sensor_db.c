#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"

extern FILE * fp_fifo;

void storagemgr_parse_sensor_data(DBCONN * conn, sbuffer_t ** buffer)
{
    sensor_data_t sensor_data;
    while(sbuffer_remove(*buffer, &sensor_data)!= SBUFFER_NO_DATA)
    {
        insert_sensor(conn, sensor_data.id, sensor_data.value, sensor_data.ts);
    }
    
}

DBCONN * init_connection(char clear_up_flag)
{
   sqlite3 *db;
   char *err_msg = 0;
   int rc = sqlite3_open(TO_STRING(DB_NAME), &db);
   if (rc != SQLITE_OK) {
        
        fprintf(fp_fifo, "Cannot open database: %s\n", sqlite3_errmsg(db));
        fflush(fp_fifo);
        sqlite3_close(db);
        return NULL;
    }
    else
   {
       fprintf(fp_fifo, "database connected\n");
       fflush(fp_fifo);
   }
   if(clear_up_flag == '1')
   {
     char *sql = "DROP TABLE IF EXISTS "TO_STRING(TABLE_NAME);
     rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return NULL;
    }
    else {
         fprintf(fp_fifo, "old table dropped : "TO_STRING(TABLE_NAME)"\n");
     }
    sql = "CREATE TABLE "TO_STRING(TABLE_NAME)"( Id INTEGER PRIMARY KEY, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP)";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
        fflush(fp_fifo);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return NULL;
    }
    else {
        fprintf(fp_fifo, "new table created : "TO_STRING(TABLE_NAME)"\n");
        fflush(fp_fifo);
    }
     //rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
   }
   else
   {
     char *sql = "CREATE TABLE IF NOT EXISTS "TO_STRING(TABLE_NAME)"( Id INTEGER PRIMARY KEY, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP)";
     rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return NULL;
     }
     else {
         fprintf(fp_fifo, "new table created : "TO_STRING(TABLE_NAME)"\n");
         fflush(fp_fifo);
     }
   } 
   return db;
}

void disconnect(DBCONN *conn)
{
    fprintf(fp_fifo, "database disconnected\n");
    fflush(fp_fifo);
    sqlite3_close(conn);
}

int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
  sqlite3_stmt *res;
  char *sql = "INSERT INTO "TO_STRING(TABLE_NAME)"( sensor_id, sensor_value, timestamp) VALUES(?, ?, ?)";
  int rc = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
  if (rc == SQLITE_OK) {
     sqlite3_bind_int(res, 1, (int)id);
     sqlite3_bind_double(res, 2, value);
     sqlite3_bind_int64(res, 3, ts);       
     } else 
     { 
       fprintf(fp_fifo, "Failed to execute statement: %s\n", sqlite3_errmsg(conn));
         fflush(fp_fifo);
       return 1;
     }
    sqlite3_step(res);
    sqlite3_finalize(res);
  return 0;
}

int find_sensor_all(DBCONN * conn, callback_t f)
{
  char *err_msg = 0;
  char *sql = "SELECT * FROM "TO_STRING(TABLE_NAME);
  int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        return 1;
     }
  return 0;
}

int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
  char *err_msg = 0;
  char *sql = NULL;
  asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value=(%f)", value);//"SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value=?";
  int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
  free(sql);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        return 1;
     }
  return 0;
}

int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
  char *err_msg = 0;
  char *sql = NULL;
  asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value>(%f)", value);
  int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
  free(sql);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        return 1;
     }
  return 0;
}

int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
  char *err_msg = 0;
  char *sql = NULL;
  asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp=(%ld)", (long)ts);
  int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
  free(sql);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        return 1;
     }
  return 0;
}

int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
  char *err_msg = 0;
  char *sql = NULL;
  asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp>(%ld)", (long)ts);
  int rc = sqlite3_exec(conn, sql, f, 0, &err_msg);
  free(sql);
     if (rc != SQLITE_OK ) {
        fprintf(fp_fifo, "SQL error: %s\n", err_msg);
         fflush(fp_fifo);
        sqlite3_free(err_msg);        
        return 1;
     }
  return 0;
}

































































