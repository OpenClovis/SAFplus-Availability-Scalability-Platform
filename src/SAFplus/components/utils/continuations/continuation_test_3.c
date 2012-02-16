/*
 * Continuation repeater example. which just repeats 2 continuations one after the other
 * and adds a third on demand.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "continuation_player.h"

static int g_cont;
static int g_countdown;
struct scope_repeater
{
    int cur_index;
    int len;
    char *arr;
};

static void bday_alarm(int sig)
{
    if(--g_countdown <= 0)
    {
        static struct itimerval stop_timer;
        setitimer(ITIMER_REAL, &stop_timer, NULL);
        close_continuation(g_cont);
    }
}

static void setup_alarm(void)
{
    struct tm curtime = {0};
    time_t t, exptime;
    static struct itimerval itimer = { .it_interval = { .tv_sec = 1, .tv_usec = 0 },
                                       .it_value = {.tv_sec = 1, .tv_usec = 0 },
    };
    time(&t);
    localtime_r(&t, &curtime);
    curtime.tm_mday = 5;
    curtime.tm_mon = 7;
    curtime.tm_sec = 0;
    curtime.tm_min = 0;
    curtime.tm_hour = 0;
    exptime = mktime(&curtime);
    g_countdown = abs(exptime - t);
    signal(SIGALRM, bday_alarm);
    setitimer(ITIMER_REAL, &itimer, NULL);
}

static void continuation_repeater_release(void *arg, int last_cont)
{
    struct scope_repeater *scope = arg;
    if(scope)
    {
        if(scope->arr) free(scope->arr);
        free(scope);
    }
}

static void do_expiry(void *arg)
{
    static char expbuf[40];
    snprintf(expbuf, sizeof(expbuf), " [%d] secs to expiry\n", g_countdown);
    fputs(expbuf, stdout);
    usleep(500000);
    remove_continuation(g_cont);
}

static void do_bday(void *arg)
{
    struct scope_repeater *scope = arg;
    int index = scope->cur_index;
    if(!scope->len)
    {
        scope->arr = strdup("a" "p" " " "b" "r" "h" "a" " " "d" "v" "r" "j");
        scope->len = strlen(scope->arr);
    }
    scope->cur_index = (scope->cur_index + 1) % scope->len;
    putc(scope->arr[index], stdout);
    if(!scope->cur_index)
    {
        static continuation_block_t expiry_continuation = { .block = do_expiry, .arg = NULL };
        add_continuation(g_cont, &expiry_continuation, 1);
    }
    fflush(stdout);
}

static void do_happy(void *arg)
{
    struct scope_repeater *scope = arg;
    int index = scope->cur_index;
    if(!scope->len)
    {
        scope->arr = strdup("h" "p" "y" " " "i" "t" "d" "y" " " "e" "a" "a");
        scope->len = strlen(scope->arr);
    }
    scope->cur_index = (scope->cur_index + 1) % scope->len;
    putc(scope->arr[index], stdout);
    fflush(stdout);
}

static __inline__ struct scope_repeater *make_scope(void)
{
   return calloc(1, sizeof(struct scope_repeater));
}

static void continuation_repeater(void)
{
    continuation_block_t continuations[2] = {
        {.block = do_happy, .arg = make_scope()},
        {.block = do_bday, .arg = make_scope()},
    };

    int c = open_continuation(continuations, sizeof(continuations)/sizeof(continuations[0]), 
                              continuation_repeater_release, CONT_REPEATER);
    g_cont = c;
    mark_continuation(c);
    setup_alarm();
    /*
     * Just play the marked continuations.
     */
    continuation_player();
}

int main(int argc, char **argv)
{
    continuation_repeater();
    return 0;
}
