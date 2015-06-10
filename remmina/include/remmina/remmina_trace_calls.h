/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINATRACECALLS_H__
#define __REMMINATRACECALLS_H__

#ifdef  WITH_TRACE_CALLS

#include <gtk/gtk.h>

#define TRACE_CALL(text) \
{ \
	GDateTime *datetime = g_date_time_new_now_local(); \
	gchar *sfmtdate = g_date_time_format(datetime, "%x %X"); \
	g_print("%s Trace calls: %s\n", sfmtdate, text); \
	g_free(sfmtdate); \
	g_date_time_unref(datetime); \
}

#else
#define TRACE_CALL(text)
#endif  /* _WITH_TRACE_CALLS_ */

#endif  /* __REMMINATRACECALLS_H__  */
