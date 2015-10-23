#ifndef __FUSEOPS_H__
#define __FUSEOPS_H__

#include <fuse.h>
#include <sys/stat.h>

void *op_init(struct fuse_conn_info*);
void  op_destroy(void*);
int   op_getattr(const char*, struct stat*);
int   op_readdir(const char*, void*, fuse_fill_dir_t, off_t, struct fuse_file_info*);
int   op_read(const char*, char*, size_t, off_t, struct fuse_file_info*);
int   op_open(const char*, struct fuse_file_info*);
int   op_mkdir(const char*, mode_t);
int   op_opendir(const char*, struct fuse_file_info*);

#endif
