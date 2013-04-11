#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <dirent.h>
#include <fuse.h>

#define PROGNAME "ogurelfs"

#define DEBUGGING 1

#define LOG(priority, format, args...)              \
    do { syslog(priority, format, args); } while(0)

#define DEBUG(format, args...)                                      \
    do { if(DEBUGGING) { LOG(LOG_DEBUG, format, args); } } while(0)
#define LOGINFO(format, args...) LOG(LOG_INFO, format, args)
#define LOGERR(format, args...) LOG(LOG_ERR, format, args)

#define OGUREL_CONTEXT ((struct ogurelfs *) fuse_get_context()->private_data)

struct ogurelfs {
    char *dbpath;
};


static char *ogurelfs_dbpath(char *absolute, const char *relative)
{
    strncpy(absolute, OGUREL_CONTEXT->dbpath, PATH_MAX);
    strncat(absolute, relative, PATH_MAX);

    return absolute;
}


static int ogurel_read(const char *pathname, char *buf, size_t size,
                off_t offset, struct fuse_file_info *fi)
{
    DEBUG("read(): %s\n", pathname);
    
    return 0;
}

static int ogurel_open(const char *pathname, struct fuse_file_info *fi)
{
    DEBUG("open(): %s%s\n",
            ((struct ogurelfs *) fuse_get_context()->private_data)->dbpath,
            pathname);
    
    return 0;
}

static int ogurel_mkdir(const char *pathname, mode_t mode)
{
    char dbpath[PATH_MAX];

    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("mkdir(): %s\n", dbpath);
    mkdir(dbpath, mode);
    
    return 0;
}

static int ogurel_opendir(const char *pathname, struct fuse_file_info *fi)
{
    int res = 0;
    DIR *dirp;
    char dbpath[PATH_MAX];

    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("opendir(): %s\n", dbpath);
    
    if(!(dirp = opendir(dbpath))) {
        LOGERR("opendir(): %s: %s\n", dbpath, strerror(errno));
        res = -errno;
    }

    fi->fh = (intptr_t) dirp;

    return res;
}

static int ogurel_readdir(const char *pathname, void *buf,
                   fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    DIR *dirp;
    struct dirent *entry;
    char dbpath[PATH_MAX];

    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("readdir(): %s\n", dbpath);
    
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
    
    return 0;
}

static int ogurel_getattr(const char *pathname, struct stat *stbuf)
{
    int res = 0;
    char dbpath[PATH_MAX];

    ogurelfs_dbpath(dbpath, pathname);
    
    DEBUG("getattr(): %s\n", dbpath);
    
    memset(stbuf, 0, sizeof(struct stat));
    
    res = stat(dbpath, stbuf);
    if(res < 0) {
        LOGERR("getattr(): %s: %s\n", dbpath, strerror(errno));
        res = -errno;
    }
    
    return res;
}

static void *ogurel_init(struct fuse_conn_info *ci)
{
    LOGINFO("%s initialized\n", PROGNAME);
    openlog(PROGNAME, LOG_PID, LOG_USER);
    
    return fuse_get_context()->private_data;
}

static void ogurel_destroy(void *data)
{
    LOGINFO("%s destroyed\n", PROGNAME);
    closelog();

    return;
}

struct fuse_operations ogurel_oper = {
    .getattr = ogurel_getattr,
    .readdir = ogurel_readdir,
    .opendir = ogurel_opendir,
    .mkdir = ogurel_mkdir,
    .open = ogurel_open,
    .read = ogurel_read,
    .init = ogurel_init,
    .destroy = ogurel_destroy
};

int main(int argc, char *argv[])
{
    static struct ogurelfs *ogurelfs;
    
    if(argc < 2)
        return 1;
    
    ogurelfs = malloc(sizeof(struct ogurelfs));
    ogurelfs->dbpath = realpath("./ogureldb", NULL);
    if(!ogurelfs->dbpath) {
        return 1;
    }


    printf("dbpath: %s\n", ogurelfs->dbpath);

    fuse_main(argc, argv, &ogurel_oper, ogurelfs);

    free(ogurelfs->dbpath);
    free(ogurelfs);
    
    return 0;
}
