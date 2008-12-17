#ifndef APR_POOLS_XX
#define APR_POOLS_XX

typedef struct
{
	unsigned char *data;
	int size;
	int start;

}apr_pool_t;

typedef int (*apr_abortfunc_t)(int retcode);

#ifdef __CPLUSPLUS_
extern "C"
{
#endif
void * apr_palloc(apr_pool_t *p, apr_size_t size);

int apr_pool_create_ex(apr_pool_t **newpool,
                                             apr_pool_t *parent,
                                             apr_abortfunc_t abort_fn,
                                             void *allocator);
void apr_pool_destroy(apr_pool_t *p);

#ifdef __CPLUSPLUS_
extern "C"
}
#endif

#endif
