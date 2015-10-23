#ifndef __OGUREL_H__
#define __OGUREL_H__

#include <syslog.h>
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

enum ogurelfs_dir {
    OGURELFS_ROOT = 0,
    OGURELFS_ARTIST,
    OGURELFS_ALBUMS,
    OGURELFS_TRACKS
};

struct ogurelfs {
    char *dbpath;
    struct ogurelfs_resolvers *resolvers;
};

char *ogurelfs_dbpath(char*, const char*);
enum  ogurelfs_dir ogurelfs_dir(const char*);
char *ogurelfs_translate(const char*, char*);

#endif
