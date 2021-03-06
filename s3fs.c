/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

/* The callback functions of this code were written by Sebastian Sangervasi
	and Nicole VanMeter for Colgate University COSC 301 course.
	Submission date: December 7, 2012 CE
	
*/
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
#define _GNU_SOURCE
/*
 * For each function below, if you need to printf("\n"); return an error,
 * read the appropriate man page for the call and see what
 * error codes make sense for the type of failure you want
 * to convey.  For example, many of the calls below return
 * -EIO (an I/O error), since there are no S3 calls yet
 * implemented.  (Note that you need to printf("\n"); return the negative
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
	
	
	DNAME; //creates (mallocs) a const char* called dtemp
	uint8_t* dirbuf;
	fprintf(stderr, "begin alright__ dname: %s\n", dname);
	ssize_t read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	free(dtemp);
	if (read <= 0 || dirbuf == NULL){
		fprintf(stderr, "not alright\n");
	    printf("\n");
		free(dirbuf); 
		return -ENOENT;
	}
	fprintf(stderr, "alright\n");
	int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	if(strcmp(path, (const char*)"/") != 0){
		BNAME; //similar to DNAME
		for(; entdex < nument; entdex++){			
			if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
				break;
			}			 
		}
		free(btemp);
	}
	
	if(nument == entdex){ 
		printf("entdex ==nument\n");
		free(dirbuf);
		return -ENOENT;
	}
	fprintf(stderr, "still alright\n");
	if(dirarray[entdex].type == 'd'){ //If we're statting a dir, we need to navigate to its '.' instead.		
		free(dirbuf);
		read = s3fs_get_object(BUCK, path, &dirbuf, 0, sizeof(s3dirent_t));
		if (read < 0 || dirbuf == NULL){
	   		printf("this case\n"); 
			free(dirbuf);
			return -ENOENT;
		}
		dirarray = (s3dirent_t*)dirbuf;
		entdex = 0;
	}
	fprintf(stderr, "still 2 alright\n");
	if(statbuf == NULL){ statbuf = malloc(sizeof(struct stat));}
	
	statbuf->st_mode = dirarray[entdex].mode;
	statbuf->st_size = (off_t)dirarray[entdex].size;
	statbuf->st_nlink = 0;
	statbuf->st_uid = getuid();
	statbuf->st_gid = getgid();
	statbuf->st_blksize = 0;
	statbuf->st_blocks =1+( dirarray[entdex].size/512);
	statbuf->st_atime = mktime(&(dirarray[entdex].modTime));
	statbuf->st_ctime = mktime(&(dirarray[entdex].createTime));
	statbuf->st_mtime = mktime(&(dirarray[entdex].modTime));
	
	free(dirbuf);
	printf("\n"); 
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

	ssize_t wrote = s3fs_put_object(BUCK, path, NULL, 0);
	if(wrote!=0){
		return -1;
	}
    uint8_t* dirbuf;
	DNAME;
	ssize_t read = s3fs_get_object(BUCK,(const char*)dname, &dirbuf, 0, 0);
	if (read < 0 || dirbuf == NULL){
	    free(dtemp); 
		return -ENOENT;
	}
	
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	s3dirent_t* newdirarray = malloc(read + sizeof(s3dirent_t));
	BNAME;
	s3dirent_t* newfile = file_init((const char*)bname);
	newfile->mode = mode;
	fprintf(stderr, "bname: _%s_\n", bname);
	memcpy((void*)newdirarray, (const void*)dirarray, read);
	memcpy((void*)(newdirarray) + read/sizeof(void), (const void*)newfile, sizeof(s3dirent_t));
	newdirarray[0].size += sizeof(s3dirent_t);
	TMSTAMP;
	newdirarray[0].modTime = actual;	
	if((read + sizeof(s3dirent_t)) == s3fs_put_object(BUCK,(const char*)dname, (const uint8_t*)newdirarray,read + sizeof(s3dirent_t))){
		fprintf(stderr, "directory entry put in parent. key of parent: _%s_\n", dname);
	}
	free(newdirarray);
	free(newfile);
	free(dirbuf);
	return 0;	
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
    fprintf(stderr, "->fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR;
	
	uint8_t* dirbuf;
	DNAME;
	ssize_t read = s3fs_get_object(BUCK,(const char*)dname, &dirbuf, 0, 0);
	
	if (read < 0 || dirbuf == NULL){
	    free(dtemp); 
		return -ENOENT;
	}
	//int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	s3dirent_t* newdirarray = malloc(read + sizeof(s3dirent_t));
	BNAME;
	s3dirent_t* newdir = dir_init((const char*)bname);
	fprintf(stderr, "bname: _%s_\n", bname);
	memcpy((void*)newdirarray, (const void*)dirarray, read);
	memcpy((void*)(newdirarray) + read/sizeof(void), (const void*)newdir, sizeof(s3dirent_t));
	newdirarray[0].size += sizeof(s3dirent_t);
	TMSTAMP;
	newdirarray[0].modTime = actual;	
	if((read + sizeof(s3dirent_t)) == s3fs_put_object(BUCK,(const char*)dname, (const uint8_t*)newdirarray,read + sizeof(s3dirent_t))){
		fprintf(stderr, "directory entry put in parent. key of parent: _%s_\n", dname);
	}
	
	free(newdirarray);
	free(newdir);
	free(dirbuf);
	newdir = dir_init(NULL);
	newdir -> mode = mode;
	if(sizeof(s3dirent_t) == s3fs_put_object(BUCK,path, (const uint8_t*)newdir,sizeof(s3dirent_t))){
		fprintf(stderr, "new entry put onto file system. key: _%s_\n", path);
	}
	free(dtemp);
	free(btemp);
	free(newdir); 
	return 0;
}

/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    return remove(path);
}

/*
 * Remove a directory. 
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
	return remove(path);	

}

//Magical removal:
int remove(const char *path) {
	s3context_t *ctx = GET_PRIVATE_DATA;

	uint8_t* dirbuf;
	ssize_t read = s3fs_get_object(BUCK, path, &dirbuf, 0, 0);
	if (read < 0){
	    printf("\n"); 
		return -EIO;
	}

	if( read > sizeof(s3dirent_t)){	
		printf("\n"); return -1;
	}
	s3fs_remove_object(BUCK, path);
	free(dirbuf);
	DNAME;
	read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	if (read < 0){
		free(dtemp); 
		return -EIO;
	}

	s3dirent_t* dirarray = (s3dirent_t*) dirbuf;
	s3dirent_t* newarray = malloc(read - sizeof(s3dirent_t));
	
	int i = 0;
	int j = 0;
	BNAME;
	for(; i < read/sizeof(s3dirent_t) ; i++){
		if(strcmp((const char*)dirarray[i].name, (const char*)bname) != 0){
			memcpy((void*) (newarray+j), (const void*) (dirarray + i), sizeof(s3dirent_t));
			j++;
		}
	}
	newarray[0].size -= sizeof(s3dirent_t);
	TMSTAMP;
	newarray[0].modTime =actual;
		
	if(read-sizeof(s3dirent_t) == s3fs_put_object(BUCK,(const char*)dname, (const uint8_t*)newarray,read-sizeof(s3dirent_t))){
		fprintf(stderr, "parent dir updated.\n");
	}
	  
	free(dtemp);
	free(btemp);
	free(newarray);
	free(dirbuf);
    printf("\n"); 
	return 0;
}


/*
 * Rename a file.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;
		
	if(strcmp(path, (const char*)"/") == 0){
		return -EISDIR;
	}

	uint8_t* buf;
	int read = s3fs_get_object(BUCK, path, &buf, 0,0);
	if(read <0){
		return -1;
	}
	
	s3fs_put_object(BUCK, newpath, buf, read);
	free(buf);

	uint8_t* dirbuf;
	DNAME;
	read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	if (read < 0 || dirbuf == NULL){
	    printf("read<0 or dirbuf == null\n");
		free(dtemp);
		free(dirbuf); 
		return -ENOENT;
	}
	int nument = read/sizeof(s3dirent_t);
	
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	BNAME;
	for(; entdex < nument; entdex++){
		if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
			strcpy(dirarray[entdex].name, basename((char*)newpath));
			TMSTAMP;
			dirarray[entdex].modTime = actual;
			break;
		}
	}
	if(read == s3fs_put_object(BUCK,(const char*)dname, (const uint8_t*)dirarray,read)){
		fprintf(stderr, "parent dir updated.\n");
	}
	free(dirbuf);
	free(dtemp);
	free(btemp);
    printf("\n"); 
	return 0;
}

/*
 * Change the permission bits of a file.
 */
int fs_chmod(const char *path, mode_t mode) {
    fprintf(stderr, "fs_chmod(fpath=\"%s\", mode=0%03o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    printf("\n"); 
	return -EIO;
}

/*
 * Change the owner and group of a file.
 */
int fs_chown(const char *path, uid_t uid, gid_t gid) {
    fprintf(stderr, "fs_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);
    s3context_t *ctx = GET_PRIVATE_DATA;
    printf("\n"); return -EIO;
}

/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;
    
	if(strcmp(path, (const char*)"/") == 0){
		return -EISDIR;
	}

	uint8_t* dirbuf;
	DNAME;
	int read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	if (read < 0 || dirbuf == NULL){
	    printf("read<0 or dirbuf == null\n");
		free(dtemp);
		free(dirbuf); 
		return -ENOENT;
	}
	int nument = read/sizeof(s3dirent_t);
	
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	BNAME;
	for(; entdex < nument; entdex++){
		if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
			if(dirarray[entdex].type == 'd'){
				free(dirbuf); free(dtemp); free(btemp);
				return -EISDIR;
			}
			s3fs_remove_object(BUCK, path);
			dirarray[entdex].size = 0;
			TMSTAMP;
			dirarray[entdex].modTime = actual;					
			s3fs_put_object(BUCK, path, NULL, 0);			
			break;
		}
	}
	if(read == s3fs_put_object(BUCK,(const char*)dname, (const uint8_t*)dirarray,read)){
		fprintf(stderr, "parent dir updated.\n");
	}
	free(dirbuf);
	free(dtemp);
	free(btemp);
    printf("\n"); 
	return 0;
}

/*
 * Change the access and/or modification times of a file. 
 */
int fs_utime(const char *path, struct utimbuf *ubuf) {
    fprintf(stderr, "fs_utime(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
}


/* 
 * File open operation
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  
 * 
 * Optionally open may also printf("\n"); return an arbitrary filehandle in the 
 * fuse_file_info structure (fi->fh).
 * which will be passed to all file operations.
 * (In stages 1 and 2, you are advised to keep this function very,
 * very simple.)
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
   
	if(strcmp(path, (const char*)"/") == 0){
		return -EISDIR;
	}
	uint8_t* dirbuf;
	DNAME;
	ssize_t read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	if (read < 0 || dirbuf == NULL){
	    printf("read<0 or dirbuf == null\n");
		free(dtemp);
		free(dirbuf); 
		return -ENOENT;
	}
	int nument = read/sizeof(s3dirent_t);
	
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	BNAME;
	for(; entdex < nument; entdex++){
		if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
			break;
		}
	}
	free(dtemp);
	free(btemp);
	if(entdex >=nument){
		free(dirbuf);
		return -ENOENT;
	}
	if(dirarray[entdex].type == 'f'){		
		fprintf(stderr, "~~Good Is File\n\n");
		free(dirbuf); 	
		return 0;
	}  
	free(dirbuf);
	
	fprintf(stderr, "__Not File Error__ path: _%s_\n", path);
	printf("\n"); 
	return -EISDIR;
}


/* 
 * Read data from an open file
 *
 * Read should printf("\n"); return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
	        path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;

	
	uint8_t* dirbuf;
	ssize_t read = s3fs_get_object(BUCK, path, &dirbuf, 0, 0);
	if (read  <0){ 
		free(dirbuf);
		return -EIO;
	}
	if(offset+size > read){
		return -EOF;
	} 
	buf = malloc(size);
	memcpy((void*)buf,(const void*)(dirbuf+offset), size);  
	
	free(dirbuf);
    printf("\n"); 
	return (int)size;
}

/*
 * Write data to an open file
 *
 * Write should printf("\n"); return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
	        path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    
	if(buf == NULL){
		return 0;
	}	
	uint8_t* dirbuf;
	ssize_t read = s3fs_get_object(BUCK, path, &dirbuf, 0, 0);
	if (read  <0){ 
		free(dirbuf);
		return -EIO;
	}
	uint8_t* newbuf;
	ssize_t newsize = read;
	if(offset+size > read){
		newbuf = malloc(offset+size);
		memcpy(newbuf, (const void*)dirbuf, read);
		newsize = offset + size;
		free(dirbuf);
	} else{ 
		newbuf = dirbuf;
	}
	memcpy((newbuf+offset),(void*)buf, size);  
	if(newsize != s3fs_put_object(BUCK, path, (const uint8_t*)newbuf, newsize)){
		return -1;
	}	
	free(newbuf);

	DNAME;
	read = s3fs_get_object(BUCK, dname, &dirbuf, 0, 0);
	int nument = read/sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*) dirbuf;	
	BNAME;
	int entdex = 1;
	for(; entdex < nument; entdex++){
		if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
			dirarray[entdex].size = newsize;
			TMSTAMP;
			dirarray[entdex].modTime = actual;
			break;
		}
	}
	if(read != s3fs_put_object(BUCK, dname, (const uint8_t*)dirbuf, read)){
		free(btemp);
		free(dtemp);
		free(dirbuf);
		return -1;
	}
	free(btemp);
	free(dtemp);
	free(dirbuf);
    printf("\n"); 
	return (int)size;
}


/* 
 * Possibly flush cached data for one file.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to printf("\n"); return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and printf("\n"); return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 */
int fs_flush(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_flush(path=\"%s\", fi=%p)\n", path, fi);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    printf("\n");
	 return 0;
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
 * file.  The printf("\n"); return value of release is ignored.
 */
int fs_release(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_release(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    printf("\n"); 
	return 0;
}

/*
 * Synchronize file contents; any cached data should be written back to 
 * stable storage.
 */
int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_fsync(path=\"%s\")\n", path);
	printf("\n"); return 0;
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
	
	if(strcmp(path, (const char*)"/") == 0){
		return 0;
	}
	uint8_t* dirbuf;
	DNAME;
	ssize_t read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	if (read < 0 || dirbuf == NULL){
	    printf("read< or dirbuf == null\n");
		free(dtemp);
		free(dirbuf); 
		return -ENOENT;
	}
	int nument = read/sizeof(s3dirent_t);
	
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	BNAME;
	for(; entdex < nument; entdex++){
		if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
			break;
		}
	}
	free(dtemp);
	free(btemp);
	if(entdex >=nument){
		free(dirbuf);
		return -ENOENT;
	}
	if(dirarray[entdex].type == 'd'){		
		fprintf(stderr, "__Good Is Dir\n\n");
		free(dirbuf);
		printf("\n"); return 0;
	}  
	free(dirbuf);
	
	fprintf(stderr, "__Not Dir Error__ path: _%s_\n", path);
	printf("\n"); 
	return -ENOTDIR;
 
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

	uint8_t* dirbuf;
	ssize_t read = s3fs_get_object(BUCK, path, &dirbuf, 0, 0);
	if (read  <0){ 
		free(dirbuf);
		return -EIO;
	}
	unsigned int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	unsigned int i = offset/sizeof(s3dirent_t);
	for(; i < nument; i++){
		fprintf(stderr, "-----Dir[%d] name is _%s_\n", i, dirarray[i].name);
		if( filler(buf, dirarray[i].name, NULL, 0) != 0) {
			free(dirbuf); 
			return -ENOMEM;
		}
		
	}
	free(dirbuf);
	printf("\n"); 
	return 0;
}

/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
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
	if(0!=s3fs_clear_bucket(BUCK)){
		printf("\n"); return NULL;
	}
	s3dirent_t* root = dir_init(NULL);

	if(sizeof(s3dirent_t) == s3fs_put_object(BUCK, (const char*)"/", (const uint8_t*)root, sizeof(s3dirent_t))){
		fprintf(stderr, "fs_init --- file sysetem initalized.\n");
	}else{
		free(root); 
		return NULL;
	}
	free(root);
    printf("\n"); return ctx;
}

//Helper function for initializing any kind of directory entry
//Note: we'll want this to do initial time stamp eventually
s3dirent_t* dir_init(const char* givename){
	s3dirent_t* newentry = malloc(sizeof(s3dirent_t));
	newentry->type = 'd';
	if(givename !=NULL){
		strcpy(newentry->name,(const char*)givename);
		printf("\n");
		return newentry;
	}
	strcpy(newentry->name,(const char*)".");
	newentry-> mode = (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR);
	//Initial timestamp for "."
	TMSTAMP;
	newentry->createTime = actual;
	newentry->modTime = actual;
	newentry->size = sizeof(s3dirent_t);
	printf("\n"); 
	return newentry;
}

s3dirent_t* file_init(const char* name){
	s3dirent_t* newentry = malloc(sizeof(s3dirent_t));
	char* realname = (char*)name;
	if(name == NULL){
		realname = "new";
	}
	strcpy(newentry->name,(const char*) realname);
	newentry->type = 'f';
	TMSTAMP;
	newentry->createTime = actual;
	newentry->modTime = actual;
	newentry->size = 0;
	newentry->mode = (S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR);
	return newentry;
}

//Helper function just for gettin the current time:
// Actually we just macro'd that sh$t. [Written by Sebastian Sangervasi]

//This is a char* so we don't use it...
/*
char* timestamp(){
	time_t start;
	struct tm actual;
	char hold[80];
	time(&start);
	actual = *localtime(&start);
	strftime(hold, sizeof(hold), "%a %Y-%m-%d %H:%M:%S %Z", &actual);
	printf("\n"); return hold;
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
 * Check file access permissions.  For now, just printf("\n"); return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;
	
	DNAME; //creates (mallocs) a const char* called dtemp
	uint8_t* dirbuf;
	ssize_t read = s3fs_get_object(BUCK, (const char*)dname, &dirbuf, 0,0);
	free(dtemp);
	if (read <= 0 || dirbuf == NULL){
		fprintf(stderr, "not alright\n");
	    printf("\n");
		free(dirbuf); 
		return -ENOENT;
	}

	int nument = (int)read / sizeof(s3dirent_t);
	s3dirent_t* dirarray = (s3dirent_t*)dirbuf;
	//loop to look for basename()
	int entdex =0;
	if(strcmp(path, (const char*)"/") != 0){
		BNAME; //similar to DNAME
		for(; entdex < nument; entdex++){			
			if(strcmp((const char*)dirarray[entdex].name, (const char*)bname) == 0){
				break;
			}			 
		}
		free(btemp);
	}else{
		free(dirbuf);
		return 0;
	}

	
	if(nument == entdex){ 
		printf("entdex ==nument\n");
		free(dirbuf);
		return -ENOENT;
	}

	
	if(dirarray[entdex].type == 'd'){ //If we're statting a dir, we need to navigate to its '.' instead.		
			
		free(dirbuf);
		read = s3fs_get_object(BUCK, path, &dirbuf, 0, sizeof(s3dirent_t));
		if (read < 0 || dirbuf == NULL){
	   		printf("this case\n"); 
			free(dirbuf);
			return -ENOENT;
		}
		dirarray = (s3dirent_t*)dirbuf;
		entdex = 0;
	}

	free(dirbuf);
	if((dirarray[entdex].mode | mask) != dirarray[entdex].mode){
		return -EACCES;
	} 
	return 0;
}

/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    return fs_truncate(path, 0);
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
    	printf("\n"); return -1;
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
    
    printf("\n"); return fuse_stat;
}
