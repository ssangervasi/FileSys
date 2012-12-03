/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

#include "s3fs.h"
#include "libs3_wrapper.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#define GET_PRIVATE_DATA ((s3context_t *) fuse_get_context()->private_data)

/*
 * For each function below, if you need to return an error,
 * read the appropriate man page for the call and see what
 * error codes make sense for the type of failure you want
 * to convey.  For example, many of the calls below return
 * -EIO (an I/O error), since there are no S3 calls yet
 * implemented.  (Note that you need to return the negative
 * value for an error code.)
 */
int fs_opendir(const char *path, struct fuse_file_info *fi);

/* 
 * Get file attributes.  Similar to the stat() call
 * (and uses the same structure).  The st_dev, st_blksize,
 * and st_ino fields are ignored in the struct (and 
 * do not need to be filled in).
 */

int fs_getattr(const char *path, struct stat *statbuf) {
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	if(stat == NULL){ stat = malloc(sizeof(struct stat);}
	
	uint8_t** dirbuf = NULL;
	ssize_t read = s3fs_get_object(BUCK, (const char*)dirname((char*)path), dirbuf, 0, sizeof(s3dirent_t));
	if (read < 0 || dirbuf == NULL){
	    return -ENOENT;
	}
	int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)(*dirbuf);
	//loop to look for basename()
	stat->st_mode = dirarray[0]->mode;
	stat->st_size = (off_t)dirarray[0]->size;
	stat->st_ctime = mktime(&(dirarray[0]->createTime));
	stat->st_mtime = mktime(&(dirarray[0]->modTime));
	free(dirarray);
	free(dirbuf);
	return 0;
}


/* 
 * Create a file "node".  When a new file is created, this
 * function will get called.  
 * This is called for creation of all non-directory, non-symlink
 * nodes.  You *only* need to handle creation of regular
 * files here.  (See the man page for mknod (2).)
 */
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/* 
 * Create a new directory.
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits (for setting in the metadata)
 * use mode|S_IFDIR.
 */
int fs_mkdir(const char *path, mode_t mode) {
    fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR;
	
	uint8_t** dirbuf = NULL;
	ssize_t read = s3fs_get_object(BUCK, (const char*)dirname((char*)path), dirbuf, 0, 0);
	if (read < 0 || dirbuf == NULL){
	    return -ENOENT;
	}
	int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)(*dirbuf);
	s3dirent_t* newdirarray = malloc(read + sizeof(s3dirent_t));
	
	s3dirent_t* newdir = dir_init((char*)basename((char*)path));

	memcpy((void*)newdirarray, (const void*)dirarray, read);
	memcpy((void*)(newdirarray+nument), (const void*)newdir, sizeof(s3dirent_t));
	newdirarray[0].size += sizeof(s3dirent_t);	
	if(read + sizeof(s3dirent_t) == s3fs_put_object(BUCK, (const char*)dirname((char*)path), (const uint8_t*)newdirarray,read + sizeof(s3dirent_t))){
		fprintf(stderr, "new dir put in parent.\n");
	}	
	free(dirarray);
	free(newdirarray);
	free(newdir);
	free(dirbuf);
	newdir = dir_init(NULL);
	newdir -> mode = mode;
	if(sizeof(s3dirent_t) == s3fs_put_object(BUCK,path, (const uint8_t*)newdirarray,sizeof(s3dirent_t))){
		fprintf(stderr, "new dir put in parent.\n");
	}
	free(newdir);
    return 0;
}

/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Remove a directory. 
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
	s3context_t *ctx = GET_PRIVATE_DATA;

	uint8_t **dirbuf = NULL;
	ssize_t read = s3fs_get_object(BUCK, path, dirbuf, 0, 0);
	if (read == -1){
	    return -EIO;
	}

	if( read > sizeof(s3dirent_t){	
		return -1;
	}
	s3fs_remove_object(BUCK, path);
	free(dirbuf);
	read = s3fs_get_object(BUCK, (const char*)dirname((char*) path), dirbuf, 0,0);
	if (read == -1){
	    return -EIO;
	}

	s3dirent_t* dirarray = (s3dirent_t*) *dirbuf;
	s3dirent_t* newarray = malloc(sizeof(read) - sizeof(s3dirent_t));
	
	int i = 0;
	int j = 0;
	for(; i <= read/sizeof(s3dirent_t) ; i++){
		if(strcmp((const char*)dirarray[i]->name, (const char*) basename((char*) path)) != 0){
			memcpy((void*) (newarray+j), (const void*) (dirarray + i), sizeof(s3dirent_t));
			j++;
		}
	}
	newarray[0].size -= sizeof(s3dirent_t);
	if(read-sizeof(s3dirent_t) == s3fs_put_object(BUCK,(const char*)dirname((char*) path), (const uint8_t*)newarray,read-sizeof(s3dirent_t))){
		fprintf(stderr, "parent dir updated.\n");
	}	
	
	free(dirarray);
	free(newarray);
	free(dirbuf);
    return 0;
}

/*
 * Rename a file.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the permission bits of a file.
 */
int fs_chmod(const char *path, mode_t mode) {
    fprintf(stderr, "fs_chmod(fpath=\"%s\", mode=0%03o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the owner and group of a file.
 */
int fs_chown(const char *path, uid_t uid, gid_t gid) {
    fprintf(stderr, "fs_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the access and/or modification times of a file. 
 */
int fs_utime(const char *path, struct utimbuf *ubuf) {
    fprintf(stderr, "fs_utime(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * File open operation
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  
 * 
 * Optionally open may also return an arbitrary filehandle in the 
 * fuse_file_info structure (fi->fh).
 * which will be passed to all file operations.
 * (In stages 1 and 2, you are advised to keep this function very,
 * very simple.)
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
	        path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
	        path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * Possibly flush cached data for one file.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 */
int fs_flush(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_flush(path=\"%s\", fi=%p)\n", path, fi);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.  
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int fs_release(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_release(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Synchronize file contents; any cached data should be written back to 
 * stable storage.
 */
int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_fsync(path=\"%s\")\n", path);
	return 0;
}

/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
int fs_opendir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	
	uint8_t **testByte = NULL; 
	/*	We don't care about getting the whole thing, we just want to know it's there
	  	so we ask it to give us back at most 1 byte from the object.
		Note that we now have BUCK representing a handy macro in s3fs.h
	*/
	int result = (int) s3fs_get_object(BUCK, path, testByte, 0, 1);
	free(*testByte);
	free(testByte); //Don't need it
	if(result == -1){
		return -EIO;
	}
	//At this point result will be 0 if it was an empty object, or 1 if it had at least one byte.
	return result;
}

/*
 * Read directory.  See the project description for how to use the filler
 * function for filling in directory items.
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    fprintf(stderr, "fs_readdir(path=\"%s\", buf=%p, offset=%d)\n",
	        path, buf, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;

	uint8_t **dirbuf = NULL;
	ssize_t read = s3fs_get_object(BUCK, path, dirbuf, 0, 0);
	if (read == -1){
	    return -EIO;
	}
	int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)(*dirbuf);
	int i = 0;
	for(; i < nument; i++){
		if( filler(buf, dirarray[i].name, NULL, 0) != 0) {
			return -ENOMEM;
		}
	}
	free(dirarray);
	free(dirbuf);
	return nument;
}

/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	
    return 0;
}

/*
 * Synchronize directory contents; cached data should be saved to 
 * stable storage.
 */
int fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_fsyncdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Initialize the file system.  This is called once upon
 * file system startup.
 */
void *fs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;
	if(0!=s3fs_clear_bucket((const char*) &(ctx->s3bucket))){
		return NULL;
	}
	s3dirent_t* root = dir_init(NULL);

	if(sizeof(s3dirent_t) == s3fs_put_object((const char*)&(ctx)->s3bucket, (const char*)"/", (const uint8_t*)root, sizeof(s3dirent_t))){
		fprintf(stderr, "fs_init --- file sysetem initalized.\n");
	}else{
		return NULL;
	}
    return ctx;
}

//Helper function for initializing any kind of directory entry
//Note: we'll want this to do initial time stamp eventually
s3dirent_t* dir_init(char* dirname){
	s3dirent_t* newentry = malloc(sizeof(s3dirent_t));
	newentry->type = 'd';
	if(dirname !=NULL){
		strcpy(newentry->name,(const char*)dirname);
		return newentry;
	}
	strcpy(newentry->name,(const char*)".");
	newentry-> mode = (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR);
	//Initial timestamp for "."
	newentry->createTime = *tmStamp();
	newentry->size = sizeof(s3dirent_t);
	return newentry;
}

s3dirent_t* file_init(const char* name, const char type){
	s3dirent_t* newentry = malloc(sizeof(s3dirent_t));
	char* realname = (char*)name;
	if(name == NULL){
		realname = "new";
	}
	strcpy(newentry->name,(const char*) realname);
	newentry->type = 'f';
	newentry->createTime = *tmStamp();
	return newentry;
}
//Actual helper function for getting a time thing
struct tm* tmStamp(){
	time_t start;
	struct tm actual;
	time(&start);
	actual = *localtime(&start);
	return &actual;
}
//Helper function just for gettin the current time:
//This is a char* so we don't use it...
/*
char* timestamp(){
	time_t start;
	struct tm actual;
	char hold[80];
	time(&start);
	actual = *localtime(&start);
	strftime(hold, sizeof(hold), "%a %Y-%m-%d %H:%M:%S %Z", &actual);
	return hold;
}
*/

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
    free(userdata);
	
}

/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
}

/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * The struct that contains pointers to all our callback
 * functions.  Those that are currently NULL aren't 
 * intended to be implemented in this project.
 */
struct fuse_operations s3fs_ops = {
  .getattr     = fs_getattr,    // get file attributes
  .readlink    = NULL,          // read a symbolic link
  .getdir      = NULL,          // deprecated function
  .mknod       = fs_mknod,      // create a file
  .mkdir       = fs_mkdir,      // create a directory
  .unlink      = fs_unlink,     // remove/unlink a file
  .rmdir       = fs_rmdir,      // remove a directory
  .symlink     = NULL,          // create a symbolic link
  .rename      = fs_rename,     // rename a file
  .link        = NULL,          // we don't support hard links
  .chmod       = fs_chmod,      // change mode bits
  .chown       = fs_chown,      // change ownership
  .truncate    = fs_truncate,   // truncate a file's size
  .utime       = fs_utime,      // update stat times for a file
  .open        = fs_open,       // open a file
  .read        = fs_read,       // read contents from an open file
  .write       = fs_write,      // write contents to an open file
  .statfs      = NULL,          // file sys stat: not implemented
  .flush       = fs_flush,      // flush file to stable storage
  .release     = fs_release,    // release/close file
  .fsync       = fs_fsync,      // sync file to disk
  .setxattr    = NULL,          // not implemented
  .getxattr    = NULL,          // not implemented
  .listxattr   = NULL,          // not implemented
  .removexattr = NULL,          // not implemented
  .opendir     = fs_opendir,    // open directory entry
  .readdir     = fs_readdir,    // read directory entry
  .releasedir  = fs_releasedir, // release/close directory
  .fsyncdir    = fs_fsyncdir,   // sync dirent to disk
  .init        = fs_init,       // initialize filesystem
  .destroy     = fs_destroy,    // cleanup/destroy filesystem
  .access      = fs_access,     // check access permissions for a file
  .create      = NULL,          // not implemented
  .ftruncate   = fs_ftruncate,  // truncate the file
  .fgetattr    = NULL           // not implemented
};



/* 
 * You shouldn't need to change anything here.  If you need to
 * add more items to the filesystem context object (which currently
 * only has the S3 bucket name), you might want to initialize that
 * here (but you could also reasonably do that in fs_init).
 */
int main(int argc, char *argv[]) {
    // don't allow anything to continue if we're running as root.  bad stuff.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Don't run this as root.\n");
    	return -1;
    }
    s3context_t *stateinfo = malloc(sizeof(s3context_t));
    memset(stateinfo, 0, sizeof(s3context_t));

    char *s3key = getenv(S3ACCESSKEY);
    if (!s3key) {
        fprintf(stderr, "%s environment variable must be defined\n", S3ACCESSKEY);
    }
    char *s3secret = getenv(S3SECRETKEY);
    if (!s3secret) {
        fprintf(stderr, "%s environment variable must be defined\n", S3SECRETKEY);
    }
    char *s3bucket = getenv(S3BUCKET);
    if (!s3bucket) {
        fprintf(stderr, "%s environment variable must be defined\n", S3BUCKET);
    }
    strncpy((*stateinfo).s3bucket, s3bucket, BUFFERSIZE);

    fprintf(stderr, "Initializing s3 credentials\n");
    s3fs_init_credentials(s3key, s3secret);

    fprintf(stderr, "Totally clearing s3 bucket\n");
    s3fs_clear_bucket(s3bucket);

    fprintf(stderr, "Starting up FUSE file system.\n");
    int fuse_stat = fuse_main(argc, argv, &s3fs_ops, stateinfo);
    fprintf(stderr, "Startup function (fuse_main) returned %d\n", fuse_stat);
    
    return fuse_stat;
}
