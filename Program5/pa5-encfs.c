#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include "aes-crypt.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define USAGE "Usage: ./pa5-encfs <KEY PHRASE> <MIRROR DIRECTORY> <MOUNT POINT>\n"

char *password;

typedef struct{
	char *rootdir;
}en_state;

#define EN_DATA ((en_state *) fuse_get_context()->private_data)

/*
* Gets the path for the "real" directory
*/
static void enter_path(char full_path[PATH_MAX], const char *path)
{
    strcpy(full_path, EN_DATA->rootdir);
    strncat(full_path, path, PATH_MAX);
}

////////////////////////////////////////////////////////////////////////////////
// Default xmp_ functions from fusexmp.c
// START DEFAULT XMP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/*
* Gets file attributes
*/
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = lstat(full_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Gets access to a certain file
*/
static int xmp_access(const char *path, int mask)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = access(full_path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Reads a symbolic link and puts the actual path in buf
*/
static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = readlink(full_path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

/*
* Reads the contects of a directory
*/
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{

	DIR *dp;
	struct dirent *de;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	(void) offset;
	(void) fi;

	dp = opendir(full_path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

/*
* Used to make special device files, not used in this program
*/
static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char full_path[PATH_MAX];
	enter_path(full_path, path);

	if (S_ISREG(mode)) {
		res = open(full_path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(full_path, mode);
	else
		res = mknod(full_path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Makes a new directory
*/
static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = mkdir(full_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Unlink a symbolic link
*/
static int xmp_unlink(const char *path)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = unlink(full_path);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Remove a directory
*/
static int xmp_rmdir(const char *path)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = rmdir(full_path);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Create a symlink
*/
static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Rename a file
*/
static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Link two files
*/
static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Change a file's permissions
*/
static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = chmod(full_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Change an objects owner
*/
static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = lchown(full_path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Truncate a given file
*/
static int xmp_truncate(const char *path, off_t size)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = truncate(full_path, size);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Get the time in microseconds
*/
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(full_path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

/*
* Open a file
*/
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	//fprintf(stderr, "full path %s\n", full_path);

	res = open(full_path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

/*
* Get file stats
*/
static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	res = statvfs(full_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	int res = lsetxattr(full_path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	int res = lgetxattr(full_path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	int res = llistxattr(full_path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char full_path[PATH_MAX];
	enter_path(full_path, path);

	int res = lremovexattr(full_path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

////////////////////////////////////////////////////////////////////////////////
// END DEFAULT XMP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/*
* Read function that reads and encrypted file
*/
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{

	FILE* fh;
	FILE* tmp;
	int res;
	char full_path[PATH_MAX];
	enter_path(full_path, path);
	char encrypted[5]; // Encypted flag 'false' or 'true'

	(void) fi;
	(void) offset;

	//open both the file and the temporary file
	fh = fopen(full_path, "r");
	tmp = tmpfile();

	getxattr(full_path, "user.pa5_encfs.encrypted", encrypted, (sizeof(char)*5));

	if(strcmp(encrypted, "true") == 0){
		if (do_crypt(fh, tmp, 0, password) == 0){
			return -errno;
		}
	} else {
    char* encrypted = "false";
    setxattr(full_path, "user.pa5_encfs.encrypted", encrypted, (sizeof(char)*5), 0);
		if (do_crypt(fh, tmp, -1, password) == 0){
			return -errno;
		}
	}

	rewind(tmp); // Go to beginning

	// Read the file
	if (fread(buf, 1, size, tmp) == -1){
      res = -errno;
  }

  // Make sure to close both files
	fclose(fh);
	fclose(tmp);

	return res;
}

/*
* Write function that writes and encrypted file
*/
static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{

	FILE* fh;
	FILE* tmp;
	int fd;
	int res;
	char full_path[PATH_MAX];
	char encrypted[5]; //this will be an integer indicating if it is encrypted

	(void) fi;

	enter_path(full_path, path);

	fh = fopen(full_path, "r+");
	tmp = tmpfile();

	getxattr(full_path, "user.pa5_encfs.encrypted", encrypted, (sizeof(char)*5));

  // Decrypt the file
	if (xmp_access(full_path, R_OK) == 0 && size > 0) {
		if(strcmp(encrypted, "true") == 0){
			if (do_crypt(fh, tmp, 0, password) == 0){
				return --errno;
			}
		}else {
			if (do_crypt(fh, tmp, -1, password) == 0){
				return -errno;
			}
		}

	}

	rewind(fh);
	rewind(tmp);

	fd = fileno(tmp); // Makes the temp file pointer a file descriptor
	res = pwrite(fd, buf, size, offset);
	if (res == -1){
    res = -errno;
  }

	// Reencrypt the file
	if(strcmp(encrypted, "true") == 0){
		if (do_crypt(tmp, fh, 1, password) == 0){ //
			return -errno;
		}
	} else {
		if (do_crypt(tmp, fh, -1, password) == 0){
			return -errno;
		}
	}

  // Make sure to close the files at the end
	fclose(fh);
	fclose(tmp);

	return res;
}

/*
* Create an encrypted file
*/
static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
    char full_path[PATH_MAX];
    int res;
    char* encrypted = "true";

    enter_path(full_path, path);

    res = creat(full_path, mode);
    if(res == -1){
	    return -errno;
    }

    // Set the encrypted attribute
    int err = setxattr(full_path, "user.pa5_encfs.encrypted", encrypted, (sizeof(char)*5), 0);
    if (err == -1){
		  return -errno;
	  }

    close(res);

    return 0;
}

/*
* Main method to be called on program start
*/
int main(int argc, char *argv[])
{
  // Give permissions to new files
	umask(0);
	en_state *en_data;

  // Make sure we are not running as root
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Please run as user\n");
		return 1;
  }

  // Check for valid args
	if (argc < 4) {
        fprintf(stderr, "Usage: ./pa5-encfs <Key> <Mirror Directory> <Mount Point>");
        return EXIT_FAILURE;
  }

  en_data = malloc(sizeof (en_state));

	// Hacky way to pass new args to fuse_main
	// Gets the real path of the mirror directory
	en_data -> rootdir = realpath(argv[2], NULL);
	password = argv[1];
	argv[1] = argv[3]; // Make Mount point the only arg
	argv[3] = NULL; // Mount Point
	argv[2] = NULL; // Root Directory
	argc = argc - 2;

	printf("Password: %s \n", password);
	printf("Root Directory: %s \n", en_data -> rootdir);

  // Mounts the new filesystem
	return fuse_main(argc, argv, &xmp_oper, en_data);
}
