/* 
 *
 *   DBGLOGGER - debug logger library / (c) 2019 El Bucanero  <www.bucanero.com.ar>
 *

 Small library for network and local file debug logging in PSL1GHT.
 This is for fw 3.55 where ethdebug is not available.
 
 Usage:
 1. Set the correct IP/port to your computer (default port 18194)
 2. Execute a local tool to listen to the incoming messages (e.g. netcat, socat)
 
 Try any of these commands in your terminal:

UDP
 nc -l -u 18194
 socat udp-recv:18194 stdout

TCP
 nc -l -k 18194
 socat tcp-listen:18194 stdout

 3. Start the app on your PS3.
 
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <net/net.h>
#include <netinet/in.h>
#include <http/util.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <lv2/process.h>

#include <time.h>

#include "dbglogger.h"

static int loggerMode = NO_LOGGER;
static int socketFD;
static char logFile[256];

#define DEBUG_PORT			18194
#define B64_SRC_BUF_SIZE	45  // This *MUST* be a multiple of 3
#define B64_DST_BUF_SIZE    4 * ((B64_SRC_BUF_SIZE + 2) / 3)

int dbglogger_b64encode(const char *filename)
{
	FILE *src_stream;
	size_t size;
	unsigned char *src_buf;
	char *dst_buf;

    src_stream = fopen(filename, "rb");
    if (!src_stream) {
        return(0);
    }
    src_buf = malloc(B64_SRC_BUF_SIZE + 1);
    dst_buf = calloc(1, B64_DST_BUF_SIZE + 1);

	dbglogger_printf("begin%s %o %s", "-base64", 0644, strrchr(filename, '/')+1);
	while ((size = fread(src_buf, 1, B64_SRC_BUF_SIZE, src_stream)) > 0) {
		if (size != B64_SRC_BUF_SIZE) {
			/* pad with 0s so we can just encode extra bits */
			memset(&src_buf[size], 0, B64_SRC_BUF_SIZE - size);
			memset(dst_buf, 0, B64_DST_BUF_SIZE + 1);
		}
		/* Encode the buffer we just read in */
		httpUtilBase64Encoder(dst_buf, src_buf, size);

        dbglogger_printf("\n%s", dst_buf);
	}
	dbglogger_printf("\n====\n");

    free(src_buf);
    free(dst_buf);
	fclose(src_stream);
	return(1);
}

// check if we receive a connection and kill the process
void debug_netkill_thread(void *port)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(strtoul(port, NULL, 0));
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	
	int list_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if ((bind(list_s, (struct sockaddr *)&sa, sizeof(sa)) == -1) || (listen(list_s, 4) == -1))
	{
		return;
	}
	
	while(accept(list_s, NULL, NULL) <= 0)
	{
        usleep(1000*1000);
	}
	
	shutdown(list_s, SHUT_RDWR);
	dbglogger_stop();
    sysProcessExit(1);
}

// check if the file exists and kill the process
void debug_kill_thread(void* path)
{
    struct stat sb;

    while ((stat((char*) path, &sb) != 0) || !S_ISREG(sb.st_mode))
    {
        usleep(1000*1000);
    }

    chmod((char*) path, 0777);
    sysLv2FsUnlink((char*) path);
	dbglogger_stop();
    sysProcessExit(1);
}

int dbglogger_failsafe(const char* fpath)
{
    sys_ppu_thread_t tid;

    return sysThreadCreate(&tid, (fpath[0] == '/' ? debug_kill_thread : debug_netkill_thread), (void*) fpath, 1000, 16*1024, THREAD_JOINABLE, "debug_wait");
}

void networkInit(const char* dbglog_ip, const unsigned short dbglog_port) {
    struct sockaddr_in stSockAddr;
    
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(dbglog_port);
    inet_pton(AF_INET, dbglog_ip, &stSockAddr.sin_addr);
    
    netConnect(socketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
}


int logFileInit(const char* file_path) {
    snprintf(logFile, sizeof(logFile), "%s", file_path);
    FILE *fp = fopen(logFile, "a");
    
    if (fp) {
        fclose(fp);
    } else {
        loggerMode = NO_LOGGER;
    }
    return(loggerMode);
}


void fileLog(const char* str) {
    FILE *fp = fopen(logFile, "a");
    
    if (fp) {
        fputs(str, fp);
        fclose(fp);
    }
}

void dbglogger_printf(const char* fmt, ...) {
	if (loggerMode) {
    	char buffer[0x800];
    	va_list arg;
    	va_start(arg, fmt);
    	vsnprintf(buffer, sizeof(buffer), fmt, arg);
    	va_end(arg);
    
    	switch (loggerMode) {
        	case UDP_LOGGER:
        	case TCP_LOGGER:
            	netSend(socketFD, buffer, strlen(buffer), 0);
            	break;
        	case FILE_LOGGER:
            	fileLog(buffer);
            	break;
    	}
	}
}

void dbglogger_log(const char* fmt, ...) {
	if (loggerMode) {
    	char buffer[0x800];

    	va_list arg;
    	va_start(arg, fmt);
    	vsnprintf(buffer, sizeof(buffer), fmt, arg);
    	va_end(arg);
    
    	struct tm t = *gmtime(&(time_t){time(NULL)});
    
    	dbglogger_printf("[%d-%02d-%02d %02d:%02d:%02d] %s\n", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, buffer);
	}
}

int dbglogger_init_mode(const unsigned char log_mode, const char* dest, const unsigned short port) {
    loggerMode = log_mode;
    switch (log_mode) {
        case UDP_LOGGER:
            netInitialize();
            socketFD = netSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            networkInit(dest, port);
            dbglogger_log("------ UDP (%s:%d) network debug logger initialized -----", dest, port);
            break;
        case TCP_LOGGER:
            netInitialize();
            socketFD = netSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            networkInit(dest, port);
            dbglogger_log("------ TCP (%s:%d) network debug logger initialized -----", dest, port);
            break;
        case FILE_LOGGER:
            if (logFileInit(dest))
            	dbglogger_log("----- File (%s) debug logger initialized -----", dest) ;
            break;
        default:
        	loggerMode = NO_LOGGER;
            // Logging disabled
            break;
    }
    
    return(loggerMode);
}

int dbglogger_init_str(const char* ini_str) {
	char str[128];	
	strcpy(str, ini_str);

	char *mode = strtok(str, ":");
	char *data = strtok(NULL, ":");
	char *tmp  = strtok(NULL, ":");
	u_short port = DEBUG_PORT;
		
	if (tmp)
		port = strtoul(tmp, NULL, 0);
	
	if (strcmp(mode, UDP_INI_STR) == 0) {
		return dbglogger_init_mode(UDP_LOGGER, data, port);
	} else 
	if (strcmp(mode, TCP_INI_STR) == 0) {
		return dbglogger_init_mode(TCP_LOGGER, data, port);
	} else 
	if (strcmp(mode, FILE_INI_STR) == 0) {
		return dbglogger_init_mode(FILE_LOGGER, data, 0);
	}
	
	return(NO_LOGGER);
}

int dbglogger_init_file(const char* ini_file) {
	char str[128];
    FILE *fp = fopen(ini_file, "r");
    
    if (fp) {
        fgets(str, sizeof(str), fp);
        fclose(fp);
		return(dbglogger_init_str(str));
    }
	return(NO_LOGGER);
}

int dbglogger_init(void) {
	return(dbglogger_init_str(DEFAULT_LOG_INIT));
}

int dbglogger_stop(void) {
    switch (loggerMode) {
        case UDP_LOGGER:
        case TCP_LOGGER:
            dbglogger_log("------ network debug logger terminated -----");
            netClose(socketFD);
            netDeinitialize();
            break;
        case FILE_LOGGER:
        	dbglogger_log("------ file debug logger terminated -----");
            break;
        default:
            // Logging disabled
            break;
    }    
    loggerMode = NO_LOGGER;
    return(loggerMode);
}
