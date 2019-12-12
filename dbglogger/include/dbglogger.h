/* 
*
*   DBGLOGGER - debug logger library / (c) 2019 El Bucanero  <www.bucanero.com.ar>
*
*/

#ifndef LIBDEBUGLOG_H
#define LIBDEBUGLOG_H

//#include <tiny3d.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
* By default the logger will send debug messages to UDP multicast address 239.255.0.100:30000. 
* To receive them you can use socat on your PC:
* $ socat udp4-recv:30000,ip-add-membership=239.255.0.100:0.0.0.0 -
*/
#define DEFAULT_LOG_INIT  "udp:239.255.0.100:30000"

#define UDP_INI_STR		  "udp"
#define TCP_INI_STR		  "tcp"
#define FILE_INI_STR 	  "file"


typedef enum {
	NO_LOGGER,
	UDP_LOGGER,
	TCP_LOGGER,
	FILE_LOGGER	
} LOGGER_MODES;

typedef enum {
	ENCODE_BASE64,
	ENCODE_UUENCODE
} B64ENC_MODES;

    
int dbglogger_init(void);
int dbglogger_init_str(const char* ini_str);
int dbglogger_init_mode(const unsigned char log_mode, const char* dest, const unsigned short port);
int dbglogger_init_file(const char* ini_file);

int dbglogger_stop(void);

// function to print with format string similar to printf
void dbglogger_printf(const char* fmt, ...);

// function that prints "[timestamp] log \n" similar to printf
void dbglogger_log(const char* fmt, ...);

// screenshot method
int dbglogger_screenshot(const char* filename, const unsigned char alpha);

// screenshot will be placed in /dev_hdd0/tmp/screenshot_YYYY_MM_DD_HH_MM_SS.bmp 
int dbglogger_screenshot_tmp(const unsigned char alpha);

// base64/uuencoding method
int dbglogger_uuencode(const char* filename, const unsigned char table);


#ifdef __cplusplus
}
#endif

#endif
