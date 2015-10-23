
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "ogurel.h"
#include "fuseops.h"

void *op_init(struct fuse_conn_info *ci)
{
    openlog(PROGNAME, LOG_PID, LOG_USER);
    
    LOGINFO("%s initialized\n", PROGNAME);
    
    return fuse_get_context()->private_data;
}

void op_destroy(void *data)
{
    LOGINFO("%s destroyed\n", PROGNAME);
    closelog();

    return;
}

int op_getattr(const char *pathname, struct stat *stbuf)
{
    int res = 0;
    char dbpath[PATH_MAX];
    int odir;
    
    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("getattr(): %s\n", dbpath);
    
    memset(stbuf, 0, sizeof(struct stat));
    odir = ogurelfs_dir(pathname);
    
    if(odir == OGURELFS_TRACKS ||
       odir == OGURELFS_ALBUMS) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
    } else {
        res = stat(dbpath, stbuf);
        if(res < 0) {
            LOGERR("getattr(): %s: %s\n", dbpath, strerror(errno));
            res = -errno;
        }
    }
        
    return res;
}

int op_readdir(const char *pathname, void *buf,
                      fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    DIR *dirp;
    struct dirent *entry;
    char dbpath[PATH_MAX];
    int odir;
    
    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("readdir(): %s\n", dbpath);

    odir = ogurelfs_dir(pathname);

    switch(odir) {
    case OGURELFS_ARTIST:
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "albums", NULL, 0);
        filler(buf, "tracks", NULL, 0);

        break;
    default:
        dirp = (DIR *) (uintptr_t) fi->fh;
        
        if(!(entry = readdir(dirp))) {
            LOGERR("readdir(): %s: %s\n", dbpath, strerror(errno));
            return -errno;
        }

        do {
            if(filler(buf, entry->d_name, NULL, 0) != 0) {
                LOGERR("readdir(): %s: %s\n", dbpath, strerror(ENOMEM));
                return -ENOMEM;
            }
        } while((entry = readdir(dirp)) != NULL);
    }
    
    return 0;
}
int op_read(const char *pathname, char *buf, size_t size,
                   off_t offset, struct fuse_file_info *fi)
{
    DEBUG("read(): %s\n", pathname);
    
    return 0;
}

int op_open(const char *pathname, struct fuse_file_info *fi)
{
    DEBUG("open(): %s%s\n",
            ((struct ogurelfs *) fuse_get_context()->private_data)->dbpath,
            pathname);
    
    return 0;
}

int op_mkdir(const char *pathname, mode_t mode)
{
    char dbpath[PATH_MAX];

    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("mkdir(): %s\n", dbpath);
    mkdir(dbpath, mode);
    
    return 0;
}

int op_opendir(const char *pathname, struct fuse_file_info *fi)
{
    int res = 0;
    DIR *dirp;
    char dbpath[PATH_MAX];

    if(ogurelfs_dir((char *) pathname) != OGURELFS_ARTIST) {
        char translate[PATH_MAX];

        ogurelfs_translate(pathname, translate);
        ogurelfs_dbpath(dbpath, translate);
        
        if(!(dirp = opendir(dbpath))) {
            LOGERR("opendir(): %s: %s\n", dbpath, strerror(errno));
            res = -errno;
        }
    
        fi->fh = (intptr_t) dirp;
    }

    return res;
}
