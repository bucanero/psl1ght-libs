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

#include <time.h>

#include "dbglogger.h"

static int loggerMode = NO_LOGGER;
static int socketFD;
static char logFile[256];

#define DEBUG_PORT			18194
#define B64_SRC_BUF_SIZE	45  // This *MUST* be a multiple of 3
#define B64_DST_BUF_SIZE    4 * ((B64_SRC_BUF_SIZE + 2) / 3)

static const char encode_table[2][65] = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
	"`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"};

/*
 *  Copyright (C) 2000 by Glenn McGrath
 *
 *  based on the function base64_encode from http.c in wget v1.6
 *  Copyright (C) 1995, 1996, 1997, 1998, 2000 Free Software Foundation, Inc.
 *
 * Encode the string S of length LENGTH to base64 format and place it
 * to STORE.  STORE will be 0-terminated, and must point to a writable
 * buffer of at least 1+BASE64_LENGTH(length) bytes.
 * where BASE64_LENGTH(len) = (4 * ((LENGTH + 2) / 3))
 */
static void uuencode(const unsigned char *s, const char *store, const int length, const unsigned int encode)
{
	int i;
	const char *tbl = encode_table[encode];
	unsigned char *p = (unsigned char *)store;

	/* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
	for (i = 0; i < length; i += 3) {
		*p++ = tbl[s[0] >> 2];
		*p++ = tbl[((s[0] & 0x03) << 4) + (s[1] >> 4)];
		*p++ = tbl[((s[1] & 0x0f) << 2) + (s[2] >> 6)];
		*p++ = tbl[s[2] & 0x3f];
		s += 3;
	}
	/* Pad the result if necessary...  */
	if (i == length + 1) {
		*(p - 1) = tbl[64];
	}
	else if (i == length + 2) {
		*(p - 1) = *(p - 2) = tbl[64];
	}
	/* ...and zero-terminate it.  */
	*p = '\0';
}

int dbglogger_uuencode(const char *filename, const unsigned short encode)
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
    dst_buf = malloc(B64_DST_BUF_SIZE + 1);

	dbglogger_printf("begin%s %o %s", encode == ENCODE_UUENCODE ? "" : "-base64", 0644, strrchr(filename, '/')+1);
	while ((size = fread(src_buf, 1, B64_SRC_BUF_SIZE, src_stream)) > 0) {
		if (size != B64_SRC_BUF_SIZE) {
			/* pad with 0s so we can just encode extra bits */
			memset(&src_buf[size], 0, B64_SRC_BUF_SIZE - size);
		}
		/* Encode the buffer we just read in */
		uuencode(src_buf, dst_buf, size, encode);

        switch (encode) {
            case ENCODE_BASE64:
                dbglogger_printf("\n%s", dst_buf);
                break;
            case ENCODE_UUENCODE:
		        dbglogger_printf("\n%c%s", encode_table[encode][size], dst_buf);
		        break;
		}
	}
	dbglogger_printf(encode == ENCODE_UUENCODE ? "\n`\nend\n" : "\n====\n");

    free(src_buf);
    free(dst_buf);
	fclose(src_stream);
	return(1);
}


void networkInit(const char* dbglog_ip, const u_short dbglog_port) {
    struct sockaddr_in stSockAddr;
    
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(dbglog_port);
    inet_pton(AF_INET, dbglog_ip, &stSockAddr.sin_addr);
    
    netConnect(socketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
}


int logFileInit(const char* file_path) {
    strncpy(logFile, file_path, 255);
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

int dbglogger_init_mode(const unsigned int log_mode, const char* dest, const u_short port) {
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
        	loggerMode = logFileInit(dest);
            if (loggerMode)
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
