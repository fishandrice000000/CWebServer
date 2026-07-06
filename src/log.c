#include "log.h"
#include <stdio.h>

static FILE *log_file = NULL;

int log_init(const char *path)
{
    log_file = fopen(path, "a");
    if (log_file == NULL) {
        fprintf(stderr, "failed to open log file: %s\n", path);
        return -1;
    }
    return 0;
}

void log_info(const char *message)
{
    if (log_file != NULL) {
        fprintf(log_file, "[INFO] %s\n", message);
        fflush(log_file);
    }
}

void log_error(const char *message)
{
    if (log_file != NULL) {
        fprintf(log_file, "[ERROR] %s\n", message);
        fflush(log_file);
    }
}

void log_close(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}
