#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
#endif

#if 0
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#endif
#include "dvbfuse.h"
#include <fuse/fuse.h>

static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
/*-------------------------------------------------------------------------*/

static int dvbfuse_getattr (const char *path, struct stat *stbuf)
{
	int res = 0;
	int cnum, n;
	char buf[512];

	memset (stbuf, 0, sizeof (struct stat));
	if (strcmp (path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		cnum = get_channel_num ();
		for (n = 0; n < cnum; n++) {
			get_channel_name (n, buf, 512);
			if (!strcmp (buf, path + 1)) {
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_nlink = 1;
				stbuf->st_size = 1024 * 1024 * 1024;
				return 0;
			}
		}
		return -ENOENT;
	}

	return res;
}

/*-------------------------------------------------------------------------*/

static int dvbfuse_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
//      (void) offset;
//      (void) fi;
	int channel_num, n;
//      int uid=getuid();

	if (strcmp (path, "/") != 0)
		return -ENOENT;

	filler (buf, ".", NULL, 0);
	filler (buf, "..", NULL, 0);

	channel_num = get_channel_num ();

	for (n = 0; n < channel_num; n++) {
		char name[256] = { 0 };
		get_channel_name (n, name, 256);
		filler (buf, name, NULL, 0);
	}

	return 0;
}

/*-------------------------------------------------------------------------*/

static int dvbfuse_open (const char *path, struct fuse_file_info *fi)
{
	void **fh;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	
	pthread_mutex_lock(&lock);
	// fh is not allowed to be overwritten later
	// So reserve own memory

	fh = (void**) malloc (sizeof(void*));
	*fh = 0;
	fi->fh=(uint64_t)fh;

	printf ("open %s %lu %p\n", path, (unsigned int long)fi->fh, *(void **)fi->fh);
	pthread_mutex_unlock(&lock);

//	fi->nonseekable=1;
	return 0;
}

/*-------------------------------------------------------------------------*/
static int dvbfuse_release (const char *path, struct fuse_file_info *fi)
{
	printf ("close %s %lu\n", path, (unsigned int long)fi->fh);
	pthread_mutex_lock(&lock);
	if (fi->fh) {
		void *handle;
		handle=*(void**)fi->fh;
		if (handle) {
			mcli_stream_stop (handle);
			free ((void *) fi->fh);
			fi->fh=0;
		}
	}
	pthread_mutex_unlock(&lock);
	return 0;
}
/*-------------------------------------------------------------------------*/

static int dvbfuse_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t len = 0;
	int retries = 0;
	void *handle;

	if (!fi || !fi->fh) {
		return -EACCES;
	}
//printf("read %lu %p\n", (unsigned int long)fi->fh, *(void **)fi->fh);
	pthread_mutex_lock(&lock);

	if (*(void**)fi->fh==NULL) {
		*(void**)fi->fh = mcli_stream_setup (path);
//		printf ("mcli_stream_setup %p\n", *(void**)fi->fh);
	}

	handle=*(void**)fi->fh;

	stream_info_t *si = (stream_info_t *) handle;

	pthread_mutex_lock(&si->lock_rd);
	pthread_mutex_unlock(&lock);

	if (si->stop)  {
		pthread_mutex_unlock(&si->lock_rd);
		return -EACCES;
	}
#if 1
	while (retries < 50) {
//		printf("si->closed %d\n",si->closed);
		len += mcli_stream_read (handle, buf + len, size - len, offset);
		offset += len;
		if (len == size)
			break;
		usleep (100 * 1000);
		retries++;
	}
#else
	len += mcli_stream_read (handle, buf + len, size - len);
#endif
//      printf("read %s %i, offset %i\n",path,(int)len,(int)offset);    
	pthread_mutex_unlock(&si->lock_rd);
	return len;
}

/*-------------------------------------------------------------------------*/

static struct fuse_operations dvbfuse_oper;

/*-------------------------------------------------------------------------*/
int start_fuse (int argc, char *argv[])
{
	printf ("Starting fuse\n");
	dvbfuse_oper.getattr = dvbfuse_getattr;
	dvbfuse_oper.readdir = dvbfuse_readdir;
	dvbfuse_oper.open = dvbfuse_open;
	dvbfuse_oper.release = dvbfuse_release;
	dvbfuse_oper.read = dvbfuse_read;
	return fuse_main (argc, argv, &dvbfuse_oper, NULL);
}
