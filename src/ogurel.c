#define FUSE_USE_VERSION 30

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuseops.h"
#include "ogurel.h"

struct ogurelfs_resolver {
    char *module_name;
    char *class_name;
    PyObject *instance;
};

struct ogurelfs_resolvers {
    struct ogurelfs_resolver *rs[16];
    int count;
};


char *ogurelfs_dbpath(char *absolute, const char *relative)
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

static struct ogurelfs *ogurelfs_init(const char *mountpoint)
{
    struct ogurelfs *ogurelfs = malloc(sizeof(struct ogurelfs));
    
    ogurelfs->dbpath = realpath(mountpoint, NULL);
    if(!ogurelfs->dbpath) {
        return NULL;
    }
    
    //python_init(ogurelfs);
    
    return ogurelfs;
}

static void ogurelfs_destroy(struct ogurelfs *ogurelfs)
{
    python_destroy(ogurelfs);
    
    free(ogurelfs->dbpath);
    free(ogurelfs);
}


struct fuse_operations ogurel_oper = {
    .getattr = op_getattr,
    //.readdir = op_readdir,
    //.opendir = op_opendir,
    //.mkdir   = op_mkdir,
    //.open    = op_open,
    //.read    = op_read,
    .init    = op_init,
    .destroy = op_destroy
};

int main(int argc, char *argv[])
{
    static struct ogurelfs *ogurelfs;

    if(argc < 2)
        return 1;

    if(!(ogurelfs = ogurelfs_init(argv[1]))) {
        return 1;
    } 
    
    fuse_main(argc, argv, &ogurel_oper, ogurelfs);

    ogurelfs_destroy(ogurelfs);
    
    return 0;
}
