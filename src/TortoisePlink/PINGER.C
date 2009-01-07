/*
 * pinger.c: centralised module that deals with sending TS_PING
 * keepalives, to avoid replicating this code in multiple backends.
 */

#include "putty.h"

struct pinger_tag {
    int interval;
    int pending;
    long next;
    Backend *back;
    void *backhandle;
};

static void pinger_schedule(Pinger pinger);

static void pinger_timer(void *ctx, long now)
{
    Pinger pinger = (Pinger)ctx;

    if (pinger->pending && now - pinger->next >= 0) {
	pinger->back->special(pinger->backhandle, TS_PING);
	pinger->pending = FALSE;
	pinger_schedule(pinger);
    }
}

static void pinger_schedule(Pinger pinger)
{
    int next;

    if (!pinger->interval) {
	pinger->pending = FALSE;       /* cancel any pending ping */
	return;
    }

    next = schedule_timer(pinger->interval * TICKSPERSEC,
			  pinger_timer, pinger);
    if (!pinger->pending || next < pinger->next) {
	pinger->next = next;
	pinger->pending = TRUE;
    }
}

Pinger pinger_new(Config *cfg, Backend *back, void *backhandle)
{
    Pinger pinger = snew(struct pinger_tag);

    pinger->interval = cfg->ping_interval;
    pinger->pending = FALSE;
    pinger->back = back;
    pinger->backhandle = backhandle;
    pinger_schedule(pinger);

    return pinger;
}

void pinger_reconfig(Pinger pinger, Config *oldcfg, Config *newcfg)
{
    if (oldcfg->ping_interval != newcfg->ping_interval) {
	pinger->interval = newcfg->ping_interval;
	pinger_schedule(pinger);
    }
}

void pinger_free(Pinger pinger)
{
    expire_timer_context(pinger);
    sfree(pinger);
}
