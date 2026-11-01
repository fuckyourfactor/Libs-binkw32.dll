#ifndef BK_TRACE_H
#define BK_TRACE_H
#ifdef BINK_ENABLE_TRACE
void bk_trace(const char *fmt, ...);
#else
#define bk_trace(...) ((void)0)
#endif
#endif
