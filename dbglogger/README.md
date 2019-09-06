# dbglogger library

By default the logger will send debug messages to UDP multicast address 239.255.0.100:30000. 

To receive them you can use socat on your PC:

```
socat udp4-recv:30000,ip-add-membership=239.255.0.100:0.0.0.0 -
```







## Initialize methods

```C
typedef enum {
	NO_LOGGER,
	UDP_LOGGER,
	TCP_LOGGER,
	FILE_LOGGER	
} LOGGER_MODES;
```

```C    
int dbglogger_init(void);
```

```C
int dbglogger_init_str(const char* ini_str);
```


`"udp:239.255.0.100:30000"`
`"tcp:192.168.1.123:18194"`
`"file:/dev_hdd0/tmp/dbglogger.log"`

```C
int dbglogger_init_mode(const unsigned int log_mode, const char* dest, const u_short port);
```

```C
int dbglogger_init_file(const char* ini_file);
```

## Shutdown methods

```C
int dbglogger_stop(void);
```

## Logging methods

// function to print with format string similar to printf
```C
void dbglogger_printf(const char* fmt, ...);
```

// function that prints "[timestamp] log \n" similar to printf
```C
void dbglogger_log(const char* fmt, ...);
```

## Screenshot methods

// screenshot method
```C
int dbglogger_screenshot(const char* filename, const unsigned short alpha);
```

// screenshot will be placed in /dev_hdd0/tmp/screenshot_YYYY_MM_DD_HH_MM_SS.bmp 
```C
int dbglogger_screenshot_tmp(const unsigned short alpha);
```

## Base64 methods

```C
typedef enum {
	ENCODE_BASE64,
	ENCODE_UUENCODE
} B64ENC_MODES;
```

// base64/uuencoding method
```C
int dbglogger_uuencode(const char* filename, const unsigned short table);
```
