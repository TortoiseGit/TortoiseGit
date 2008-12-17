#include <apr.h>
#include <apr_pools.h>
#include <apr_general.h>

void * apr_palloc(apr_pool_t *p, apr_size_t size)
{
	if(p->start+size> p->size)
		return NULL;

	else
	{
		p->start+=size;
		return (void*)(p->data+p->start);
	}
}

void * apr_pcalloc(apr_pool_t *p, apr_size_t size)
{
	void *p1=apr_palloc(p,size);
	memset(p1,0,size);
	return p1;
}
apr_pool_t * svn_pool_create(apr_pool_t *p)
{
	return malloc(4096);
}

void svn_pool_destroy(apr_pool_t *p)
{
	free(p);
}

void svn_pool_clear(apr_pool_t *p)
{
	p->start=0;
}

void svn_error__malfunction(char * error, int x, void* p)
{
}

void svn_error_clear()
{
}