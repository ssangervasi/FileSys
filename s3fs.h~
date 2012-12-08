#ifndef __USERSPACEFS_H__
#define __USERSPACEFS_H__

#include <sys/stat.h>
#include <stdint.h>   // for uint32_t, etc.
#include <sys/time.h> // for struct timeval
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> //?


/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

/* The callback functions of this code were written by Sebastian Sangervasi
	and Nicole VanMeter for Colgate University COSC 301 course.
	Submission date: December 7, 2012 CE
	Macros are cool.
*/

/* Declare to the FUSE API which version we're willing to speak */

#define FUSE_USE_VERSION 26

#define S3ACCESSKEY "S3_ACCESS_KEY_ID"
#define S3SECRETKEY "S3_SECRET_ACCESS_KEY"
#define S3BUCKET "S3_BUCKET"
#define sebastian "NERDY"
#define BUFFERSIZE 1024

#define BNAME char* btemp=strdup((const char*)path);char*bname=basename(btemp);

#define DNAME char* dtemp=strdup((const char*)path);char*dname=dirname(dtemp);

#define BUCK (const char*)(ctx->s3bucket)

#define TMSTAMP time_t start; struct tm actual; time(&start); actual = *localtime(&start);

// store filesystem state information in this struct
typedef struct {
    char s3bucket[BUFFERSIZE];
} s3context_t;

/*
 * Other data type definitions (e.g., a directory entry
 * type) should go here.
 */

//THE STRUCT THAT WE NEED.


typedef struct{
	unsigned char type; //'f' or 'd' or unused?
	char name[256];
	//metadata
	long unsigned int size; // number of bytes
	//going to need time stamps and stuff
	struct tm createTime;
	struct tm modTime;
	mode_t mode;
	
	
} s3dirent_t;

//Helper functions:

int remove(const char *path);
s3dirent_t* entry_init(const char* name, const char type);
s3dirent_t* dir_init(const char* givename);
s3dirent_t* file_init(const char* name);
struct tm* tmStamp();

#endif // __USERSPACEFS_H__
