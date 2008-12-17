#ifndef APR_GETOPT_XX
#define APR_GETOPT_XX
/** @see apr_getopt_t */
typedef struct apr_getopt_t apr_getopt_t;

/**
 * Structure to store command line argument information.
 */ 
struct apr_getopt_t {
    /** context for processing */
    apr_pool_t *cont;
    /** function to print error message (NULL == no messages) */
//    apr_getopt_err_fn_t *errfn;
    /** user defined first arg to pass to error message  */
    void *errarg;
    /** index into parent argv vector */
    int ind;
    /** character checked for validity */
    int opt;
    /** reset getopt */
    int reset;
    /** count of arguments */
    int argc;
    /** array of pointers to arguments */
    const char **argv;
    /** argument associated with option */
    char const* place;
    /** set to nonzero to support interleaving options with regular args */
    int interleave;
    /** start of non-option arguments skipped for interleaving */
    int skip_start;
    /** end of non-option arguments skipped for interleaving */
    int skip_end;
};

struct apr_finfo_t {
    /** Allocates memory and closes lingering handles in the specified pool */
    apr_pool_t *pool;
    /** The bitmask describing valid fields of this apr_finfo_t structure 
     *  including all available 'wanted' fields and potentially more */
    apr_int32_t valid;
    /** The access permissions of the file.  Mimics Unix access rights. */
   
    apr_off_t size;
    /** The storage size consumed by the file */
    apr_off_t csize;
    /** The time the file was last accessed */
    apr_time_t atime;
    /** The time the file was last modified */
    apr_time_t mtime;
    /** The time the file was created, or the inode was last changed */
    apr_time_t ctime;
    /** The pathname of the file (possibly unrooted) */
    const char *fname;
    /** The file's name (no path) in filesystem case */
    const char *name;

};

typedef struct apr_finfo_t apr_finfo_t;
#endif