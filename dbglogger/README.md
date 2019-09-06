# dbglogger library

A simple PS3 /PSL1GHT library that provides basic debug logging over the network (TCP/UDP) or to a local file.
The library also includes additional helper functions to save screenshots in PNG and encode files in Base64.

By default, the logger will send debug messages to UDP multicast address `239.255.0.100:30000`. 

To receive them you can use [socat](http://www.dest-unreach.org/socat/) on your PC:

```
socat udp4-recv:30000,ip-add-membership=239.255.0.100:0.0.0.0 -
```



## Build/Install

Build the library with: `make`

Install the library to your PSL1GHT setup with: `make install`


## Initialize methods

You need to initialize the library before you can send logs to the network or file.

### Default initialization

The default init will send log messages to UDP multicast address `239.255.0.100:30000`.

```C    
int dbglogger_init(void);
```

### Initialize with a string

You can initialize the library with a custom string, to override the default init.

```C
int dbglogger_init_str(const char* ini_str);
```

Example:
`dbglogger_init_str("tcp:192.168.1.123:18194");`

More example init strings:
```
"udp:239.255.0.100:30000"
"file:/dev_hdd0/tmp/dbglogger.log"
```

### Init by parameters

You can also initialize the library with custom parameters, to override the default init.

```C
typedef enum {
	NO_LOGGER,
	UDP_LOGGER,
	TCP_LOGGER,
	FILE_LOGGER	
} LOGGER_MODES;
```

```C
int dbglogger_init_mode(const unsigned int log_mode, const char* dest, const u_short port);
```

Example:
`dbglogger_init_mode(TCP_LOGGER, "192.168.1.123", 18999);`


### initialize with a text file

You can initialize the library with a custom string read from a text file, to override the default init.

```C
int dbglogger_init_file(const char* ini_file);
```

Example: `dbglogger_init_file("/dev_hdd0/tmp/mylogger.ini");`


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
