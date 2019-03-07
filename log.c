#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
/*主函数全局变量*/
u_int ifMTU, snaplen;//u_int 无符号非负数整数变量
u_int16_t file_id;
bool is_shutdown, shutdown_requested, do_decode_tunnels;
time_t start_time;
/*事件等级声明*/
#define TRACE_LEVEL_ERROR     0
#define TRACE_LEVEL_WARNING   1
#define TRACE_LEVEL_NORMAL    2
#define TRACE_LEVEL_INFO      3
#define TRACE_LEVEL_DEBUG     6
#define TRACE_LEVEL_TRACE     9

#define TRACE_ERROR     TRACE_LEVEL_ERROR, __FILE__, __LINE__
#define TRACE_WARNING   TRACE_LEVEL_WARNING, __FILE__, __LINE__
#define TRACE_NORMAL    TRACE_LEVEL_NORMAL, __FILE__, __LINE__
#define TRACE_INFO      TRACE_LEVEL_INFO, __FILE__, __LINE__
#define TRACE_DEBUG     TRACE_LEVEL_DEBUG, __FILE__, __LINE__
#define TRACE_TRACE     TRACE_LEVEL_TRACE, __FILE__, __LINE__

#define MAX_TRACE_LEVEL 9
#define TRACE_DEBUGGING MAX_TRACE_LEVEL
/*文件变量声明*/
char *logFile;
FILE *logFd;
/*最大路径尺寸声明*/
#define MAX_PATH                  256
/**/
volatile u_int8_t traceLevel;
/*事件日志最高行数*/
#define TRACES_PER_LOG_FILE_HIGH_WATERMARK 10000
/*本demo函数声明*/
void rotate_logs(bool forceRotation);
static size_t strftime(char *dst, size_t dst_size, const char *fmt,const struct tm *tm);
void traceEvent(int eventTraceLevel, const char* _file,const int line, const char * format, ...);
void open_log();
void Trace();



void rotate_logs(bool forceRotation) {
  char buf1[MAX_PATH], buf2[MAX_PATH];
  const int max_num_lines = TRACES_PER_LOG_FILE_HIGH_WATERMARK;

  if(!logFd) return;
  else if((!forceRotation) && (numLogLines < max_num_lines)) return;

  fclose(logFd);
  logFd = NULL;
/*c++用法，暂时不清楚
  for(int i = MAX_NUM_NTOPNG_LOG_FILES - 1; i >= 1; i--) {
    snprintf(buf1, sizeof(buf1), "%s.%u", logFile, i);
    snprintf(buf2, sizeof(buf2), "%s.%u", logFile, i + 1);

    if(Utils::file_exists(buf1))
      rename(buf1, buf2);
  }

  if(Utils::file_exists(logFile)) {
    snprintf(buf1, sizeof(buf1), "%s.1", logFile);
    rename(logFile, buf1);
  }
*/
  open_log();
}
static size_t strftime(char *dst, size_t dst_size, const char *fmt,
                       const struct tm *tm) {
  (void) snprintf(dst, dst_size, "implement strftime() for WinCE");
  return 0;
}

void traceEvent(int eventTraceLevel, const char* _file,
                       const int line, const char * format, ...) {
  va_list va_ap;

  if((eventTraceLevel <= traceLevel) && (traceLevel > 0)) {
    char buf[8192], out_buf[8192];
    char theDate[32], *file = (char*)_file;
    const char *extra_msg = "";
    time_t theTime = time(NULL);

    char filebuf[MAX_PATH];
    const char *backslash = strrchr(_file,'/');//库函数在string，定义字符串最后一个位置

    if(backslash != NULL) {
      snprintf(filebuf, sizeof(filebuf), "%s", &backslash[1]);
      file = (char*)filebuf;
    }

    va_start (va_ap, format);//读取堆栈

    /* We have two paths - one if we're logging, one if we aren't
     *   Note that the no-log case is those systems which don't support it (WIN32),
     *                                those without the headers !defined(USE_SYSLOG)
     *                                those where it's parametrically off...
     */

    memset(buf, 0, sizeof(buf));
    strftime(theDate, 32, "%d/%b/%Y %H:%M:%S", localtime_r(&theTime, &result));//时间写入字符串

    vsnprintf(buf, sizeof(buf)-1, format, va_ap);//打印堆栈信息

    if(eventTraceLevel == 0 /* TRACE_ERROR */)
      extra_msg = "ERROR: ";
    else if(eventTraceLevel == 1 /* TRACE_WARNING */)
      extra_msg = "WARNING: ";

    while(buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';

    snprintf(out_buf, sizeof(out_buf), "%s [%s:%d] %s%s", theDate, file, line, extra_msg, buf);//转换字符串拷贝

    if(logFd) {
//      rotate_mutex.lock(__FILE__, __LINE__);//互斥锁
      numLogLines++;
      fprintf(logFd, "%s\n", out_buf);
      fflush(logFd);
      rotate_logs(false);
//     rotate_mutex.unlock(__FILE__, __LINE__);
    } else {
      syslogMsg = &out_buf[strlen(theDate)+1];
      if(eventTraceLevel == 0 /* TRACE_ERROR */)
        syslog(LOG_ERR, "%s", syslogMsg);
      else if(eventTraceLevel == 1 /* TRACE_WARNING */)
        syslog(LOG_WARNING, "%s", syslogMsg);
    }

    printf("%s\n", out_buf);
    fflush(stdout);
/*重锁释放
    if(traceRedis && traceRedis->isOperational() && ntop->getRedis()->isOperational())
        traceRedis->lpush(NTOPNG_TRACE, out_buf, MAX_NUM_NTOPNG_TRACES,false);
*/
    va_end(va_ap);
  }
}

void open_log() {
  if(logFile) {
    logFd = fopen(logFile, "a");

    if(!logFd)
      traceEvent(TRACE_ERROR, "Unable to create log %s", logFile);
    else
      chmod(logFile, CONST_DEFAULT_FILE_MODE);

    numLogLines = 0;
  }
}

void Trace()
{
    traceLevel = TRACE_LEVEL_NORMAL;
    logFile = NULL;
    logFd = NULL;
    traceRedis = NULL;

    open_log();
}

void main()
{
      start_time = time(NULL);
      ifMTU = snaplen = 1514;
      file_id = 0;
      Trace();
      is_shutdown = shutdown_requested = false, do_decode_tunnels = true;
}
