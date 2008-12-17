#ifndef ARP_FILE_IO_XX
#define ARP_FILE_IO_XX
typedef FILE apr_file_t;
#define APR_SET SEEK_SET
#define APR_READ 0
#define APR_OS_DEFAULT 0
#define APR_FINFO_SIZE 0

struct apr_getopt_option_t {
    /** long option name, or NULL if option has no long name */
    const char *name;
    /** option letter, or a value greater than 255 if option has no letter */
    int optch;
    /** nonzero if option takes an argument */
    int has_arg;
    /** a description of the option */
    const char *description;
};

typedef struct apr_getopt_option_t apr_getopt_option_t;
#endif