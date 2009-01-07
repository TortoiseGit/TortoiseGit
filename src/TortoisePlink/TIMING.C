/*
 * timing.c
 * 
 * This module tracks any timers set up by schedule_timer(). It
 * keeps all the currently active timers in a list; it informs the
 * front end of when the next timer is due to go off if that
 * changes; and, very importantly, it tracks the context pointers
 * passed to schedule_timer(), so that if a context is freed all
 * the timers associated with it can be immediately annulled.
 */

#include <assert.h>
#include <stdio.h>

#include "putty.h"
#include "tree234.h"

struct timer {
    timer_fn_t fn;
    void *ctx;
    long now;
};

static tree234 *timers = NULL;
static tree234 *timer_contexts = NULL;
static long now = 0L;

static int compare_timers(void *av, void *bv)
{
    struct timer *a = (struct timer *)av;
    struct timer *b = (struct timer *)bv;
    long at = a->now - now;
    long bt = b->now - now;

    if (at < bt)
	return -1;
    else if (at > bt)
	return +1;

    /*
     * Failing that, compare on the other two fields, just so that
     * we don't get unwanted equality.
     */
#ifdef __LCC__
    /* lcc won't let us compare function pointers. Legal, but annoying. */
    {
	int c = memcmp(&a->fn, &b->fn, sizeof(a->fn));
	if (c < 0)
	    return -1;
	else if (c > 0)
	    return +1;
    }
#else    
    if (a->fn < b->fn)
	return -1;
    else if (a->fn > b->fn)
	return +1;
#endif

    if (a->ctx < b->ctx)
	return -1;
    else if (a->ctx > b->ctx)
	return +1;

    /*
     * Failing _that_, the two entries genuinely are equal, and we
     * never have a need to store them separately in the tree.
     */
    return 0;
}

static int compare_timer_contexts(void *av, void *bv)
{
    char *a = (char *)av;
    char *b = (char *)bv;
    if (a < b)
	return -1;
    else if (a > b)
	return +1;
    return 0;
}

static void init_timers(void)
{
    if (!timers) {
	timers = newtree234(compare_timers);
	timer_contexts = newtree234(compare_timer_contexts);
	now = GETTICKCOUNT();
    }
}

long schedule_timer(int ticks, timer_fn_t fn, void *ctx)
{
    long when;
    struct timer *t, *first;

    init_timers();

    when = ticks + GETTICKCOUNT();

    /*
     * Just in case our various defences against timing skew fail
     * us: if we try to schedule a timer that's already in the
     * past, we instead schedule it for the immediate future.
     */
    if (when - now <= 0)
	when = now + 1;

    t = snew(struct timer);
    t->fn = fn;
    t->ctx = ctx;
    t->now = when;

    if (t != add234(timers, t)) {
	sfree(t);		       /* identical timer already exists */
    } else {
	add234(timer_contexts, t->ctx);/* don't care if this fails */
    }

    first = (struct timer *)index234(timers, 0);
    if (first == t) {
	/*
	 * This timer is the very first on the list, so we must
	 * notify the front end.
	 */
	timer_change_notify(first->now);
    }

    return when;
}

/*
 * Call to run any timers whose time has reached the present.
 * Returns the time (in ticks) expected until the next timer after
 * that triggers.
 */
int run_timers(long anow, long *next)
{
    struct timer *first;

    init_timers();

#ifdef TIMING_SYNC
    /*
     * In this ifdef I put some code which deals with the
     * possibility that `anow' disagrees with GETTICKCOUNT by a
     * significant margin. Our strategy for dealing with it differs
     * depending on platform, because on some platforms
     * GETTICKCOUNT is more likely to be right whereas on others
     * `anow' is a better gold standard.
     */
    {
	long tnow = GETTICKCOUNT();

	if (tnow + TICKSPERSEC/50 - anow < 0 ||
	    anow + TICKSPERSEC/50 - tnow < 0
	    ) {
#if defined TIMING_SYNC_ANOW
	    /*
	     * If anow is accurate and the tick count is wrong,
	     * this is likely to be because the tick count is
	     * derived from the system clock which has changed (as
	     * can occur on Unix). Therefore, we resolve this by
	     * inventing an offset which is used to adjust all
	     * future output from GETTICKCOUNT.
	     * 
	     * A platform which defines TIMING_SYNC_ANOW is
	     * expected to have also defined this offset variable
	     * in (its platform-specific adjunct to) putty.h.
	     * Therefore we can simply reference it here and assume
	     * that it will exist.
	     */
	    tickcount_offset += anow - tnow;
#elif defined TIMING_SYNC_TICKCOUNT
	    /*
	     * If the tick count is more likely to be accurate, we
	     * simply use that as our time value, which may mean we
	     * run no timers in this call (because we got called
	     * early), or alternatively it may mean we run lots of
	     * timers in a hurry because we were called late.
	     */
	    anow = tnow;
#else
/*
 * Any platform which defines TIMING_SYNC must also define one of the two
 * auxiliary symbols TIMING_SYNC_ANOW and TIMING_SYNC_TICKCOUNT, to
 * indicate which measurement to trust when the two disagree.
 */
#error TIMING_SYNC definition incomplete
#endif
	}
    }
#endif

    now = anow;

    while (1) {
	first = (struct timer *)index234(timers, 0);

	if (!first)
	    return FALSE;	       /* no timers remaining */

	if (find234(timer_contexts, first->ctx, NULL) == NULL) {
	    /*
	     * This timer belongs to a context that has been
	     * expired. Delete it without running.
	     */
	    delpos234(timers, 0);
	    sfree(first);
	} else if (first->now - now <= 0) {
	    /*
	     * This timer is active and has reached its running
	     * time. Run it.
	     */
	    delpos234(timers, 0);
	    first->fn(first->ctx, first->now);
	    sfree(first);
	} else {
	    /*
	     * This is the first still-active timer that is in the
	     * future. Return how long it has yet to go.
	     */
	    *next = first->now;
	    return TRUE;
	}
    }
}

/*
 * Call to expire all timers associated with a given context.
 */
void expire_timer_context(void *ctx)
{
    init_timers();

    /*
     * We don't bother to check the return value; if the context
     * already wasn't in the tree (presumably because no timers
     * ever actually got scheduled for it) then that's fine and we
     * simply don't need to do anything.
     */
    del234(timer_contexts, ctx);
}
