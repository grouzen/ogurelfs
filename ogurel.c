#define FUSE_USE_VERSION 26

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
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

enum ogurelfs_dir {
    OGURELFS_ROOT = 0,
    OGURELFS_ARTIST,
    OGURELFS_ALBUMS,
    OGURELFS_TRACKS
};

struct ogurelfs_resolver {
    char *module_name;
    char *class_name;
    PyObject *instance;
};

struct ogurelfs_resolvers {
    struct ogurelfs_resolver *rs[16];
    int count;
};

struct ogurelfs {
    char *dbpath;
    struct ogurelfs_resolvers *resolvers;
};


static char *ogurelfs_dbpath(char *absolute, const char *relative)
{
    strncpy(absolute, OGUREL_CONTEXT->dbpath, PATH_MAX);
    strncat(absolute, relative, PATH_MAX);

    return absolute;
}

enum ogurelfs_dir ogurelfs_dir(const char *pathname)
{
    char *buf = NULL;
    char *buforig = NULL;
    int odir = OGURELFS_ROOT;

    if(strcmp(pathname, "/")) {
        buf = malloc(sizeof(char) * (strlen(pathname) + 1));
        buforig = buf;
        strcpy(buf, pathname);
    
        while((buf = strchr(buf, '/')) != NULL) {
            buf++;
            odir++;

            if(odir > OGURELFS_ARTIST) {
                if(!strncmp(buf, "tracks", 6)) {
                    odir = OGURELFS_TRACKS;
                }
                
                break;
            }
        }
    
        free(buforig);
    }
    
    return odir;
}

char *ogurelfs_translate(const char *pathname, char *translate)
{
    int odir = ogurelfs_dir(pathname);

    if(odir == OGURELFS_ALBUMS) {
        for(int i = 0; i < strlen(pathname); i++) {
            if(i > 0 && pathname[i] == '/') {
                translate[i] = '\0';
                break;
            }

            translate[i] = pathname[i];
        }
    }

    return translate;
}


static struct ogurelfs_resolver *python_resolver_init(char *name)
{
    PyObject *pName, *pModule, *pDict, *pClass, *pInstance;
    struct ogurelfs_resolver *resolver;
    int name_len = strlen(name) + 1;

    resolver = malloc(sizeof(struct ogurelfs_resolver));

    resolver->module_name = malloc(sizeof(char) * name_len);
    strcpy(resolver->module_name, name);
    resolver->class_name = malloc(sizeof(char) * name_len);
    strcpy(resolver->class_name, name);
    resolver->class_name[0] = toupper(resolver->class_name[0]);
    
    pName = PyString_FromString(resolver->module_name);
    pModule = PyImport_Import(pName);
    
    pDict = PyModule_GetDict(pModule);
    
    pClass = PyDict_GetItemString(pDict, resolver->class_name);
    pInstance = PyObject_CallObject(pClass, NULL);

    resolver->instance = pInstance;
    
    Py_DECREF(pName);
    Py_DECREF(pModule);
    Py_DECREF(pDict);
    Py_DECREF(pClass);

    return resolver;
}

static void python_resolver_destroy(struct ogurelfs_resolver *r)
{
    free(r->module_name);
    free(r->class_name);
    Py_DECREF(r->instance);

    free(r);

    return;
}

static void python_init(struct ogurelfs *ogurelfs)
{
    char *resolvers_names[] = { "muzebra", "lastfm" };
    struct ogurelfs_resolver *r;
        
    ogurelfs->resolvers = malloc(sizeof(struct ogurelfs_resolvers));
    ogurelfs->resolvers->count = 2;
    
    Py_Initialize();

    for(int i = 0; i < ogurelfs->resolvers->count; i++) {
        r = python_resolver_init(resolvers_names[i]);
        ogurelfs->resolvers->rs[i] = r;
    }

    return;
}

static void python_destroy(struct ogurelfs *ogurelfs)
{
    struct ogurelfs_resolvers *resolvers = ogurelfs->resolvers;

    for(int i = 0; i < resolvers->count; i++) {
        python_resolver_destroy(resolvers->rs[i]);
    }

    free(resolvers);
    
    Py_Finalize();

    return;
}

static struct ogurelfs *ogurelfs_init()
{
    struct ogurelfs *ogurelfs = malloc(sizeof(struct ogurelfs));
    
    ogurelfs->dbpath = realpath("./ogureldb", NULL);
    if(!ogurelfs->dbpath) {
        return NULL;
    }
    
    python_init(ogurelfs);
    
    return ogurelfs;
}

static void ogurelfs_destroy(struct ogurelfs *ogurelfs)
{
    python_destroy(ogurelfs);
    
    free(ogurelfs->dbpath);
    free(ogurelfs);
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

static int ogurel_readdir(const char *pathname, void *buf,
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

static int ogurel_getattr(const char *pathname, struct stat *stbuf)
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
        stbuf->st_mode = S_IFDIR | 0644;
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

static void *ogurel_init(struct fuse_conn_info *ci)
{
    openlog(PROGNAME, LOG_PID, LOG_USER);
    
    LOGINFO("%s initialized\n", PROGNAME);
    
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

    if(!(ogurelfs = ogurelfs_init())) {
        return 1;
    }
    
    fuse_main(argc, argv, &ogurel_oper, ogurelfs);

    ogurelfs_destroy(ogurelfs);
    
    return 0;
}
