#include "log.h"
#include <stdio.h>
#include <time.h>

static FILE *log_file        = NULL;
static FILE *access_log_file = NULL;

/* ---- 系统日志 ---- */

int log_init(const char *path)
{
    log_file = fopen(path, "a");
    if (log_file == NULL) {
        fprintf(stderr, "failed to open log file: %s\n", path);
        return -1;
    }
    return 0;
}

static void log_write(const char *level, const char *message)
{
    if (log_file == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_file, "%s [%s] %s\n", ts, level, message);
    fflush(log_file);
}

void log_debug(const char *message)   { log_write("DEBUG",   message); }
void log_info(const char *message)    { log_write("INFO",    message); }
void log_warning(const char *message) { log_write("WARNING", message); }
void log_error(const char *message)   { log_write("ERROR",   message); }

void log_close(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

/* ---- 访问日志 ---- */

int access_log_init(const char *path)
{
    access_log_file = fopen(path, "a");
    if (access_log_file == NULL) {
        fprintf(stderr, "failed to open access log: %s\n", path);
        return -1;
    }
    return 0;
}

void access_log_write(const char *client_ip,
                      const char *method,
                      const char *url,
                      const char *version,
                      int         status_code,
                      int         response_bytes,
                      const char *user_agent)
{
    if (access_log_file == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);

    fprintf(access_log_file, "%s %s %s %s %s %d %d %s\n",
            ts, client_ip, method, url, version,
            status_code, response_bytes,
            (user_agent && user_agent[0]) ? user_agent : "-");
    fflush(access_log_file);
}

void access_log_close(void)
{
    if (access_log_file != NULL) {
        fclose(access_log_file);
        access_log_file = NULL;
    }
}
