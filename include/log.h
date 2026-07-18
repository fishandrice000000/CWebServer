#ifndef LOG_H
#define LOG_H

/* ---- 系统日志 (四级) ---- */
int  log_init(const char *path);
void log_debug(const char *message);
void log_info(const char *message);
void log_warning(const char *message);
void log_error(const char *message);
void log_close(void);

/* ---- 访问日志 ---- */
int  access_log_init(const char *path);
void access_log_write(const char *client_ip,
                      const char *method,
                      const char *url,
                      const char *version,
                      int         status_code,
                      int         response_bytes,
                      const char *user_agent);
void access_log_close(void);

#endif
