/*
 * GNOME CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Authors : Carlos Garc�a Campos <carlosgc@gnome.org>
 */

#include <glib.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <cpufreq.h>

#include "cpufreq-monitor-libcpufreq.h"

static void     cpufreq_monitor_libcpufreq_class_init                (CPUFreqMonitorLibcpufreqClass *klass);

static gboolean cpufreq_monitor_libcpufreq_run                       (CPUFreqMonitor *monitor);
static GList   *cpufreq_monitor_libcpufreq_get_available_frequencies (CPUFreqMonitor *monitor);
static GList   *cpufreq_monitor_libcpufreq_get_available_governors   (CPUFreqMonitor *monitor);

G_DEFINE_TYPE (CPUFreqMonitorLibcpufreq, cpufreq_monitor_libcpufreq, CPUFREQ_TYPE_MONITOR)

typedef struct cpufreq_policy                CPUFreqPolicy;
typedef struct cpufreq_available_frequencies CPUFreqFrequencyList;
typedef struct cpufreq_available_governors   CPUFreqGovernorList;

static void
cpufreq_monitor_libcpufreq_init (CPUFreqMonitorLibcpufreq *monitor)
{
}

static void
cpufreq_monitor_libcpufreq_class_init (CPUFreqMonitorLibcpufreqClass *klass)
{
        CPUFreqMonitorClass *monitor_class = CPUFREQ_MONITOR_CLASS (klass);

        monitor_class->run = cpufreq_monitor_libcpufreq_run;
        monitor_class->get_available_frequencies = cpufreq_monitor_libcpufreq_get_available_frequencies;
        monitor_class->get_available_governors = cpufreq_monitor_libcpufreq_get_available_governors;
}

CPUFreqMonitor *
cpufreq_monitor_libcpufreq_new (guint cpu)
{
        CPUFreqMonitorLibcpufreq *monitor;

        monitor = g_object_new (CPUFREQ_TYPE_MONITOR_LIBCPUFREQ,
                                "cpu", cpu, NULL);

        return CPUFREQ_MONITOR (monitor);
}

static gboolean
cpufreq_monitor_libcpufreq_run (CPUFreqMonitor *monitor)
{
        guint          cpu;
	CPUFreqPolicy *policy;

        g_object_get (G_OBJECT (monitor), "cpu", &cpu, NULL);

	policy = cpufreq_get_policy (cpu);
	if (!policy)
		return FALSE;

        g_object_set (G_OBJECT (monitor),
                      "governor", policy->governor,
                      "frequency", cpufreq_get_freq_kernel (cpu),
                      "max-frequency", policy->max,
                      NULL);

	cpufreq_put_policy (policy);
        
        return TRUE;
}

static gint
compare (gconstpointer a, gconstpointer b)
{
        gint aa, bb;

        aa = atoi ((gchar *) a);
        bb = atoi ((gchar *) b);

        if (aa == bb)
                return 0;
        else if (aa > bb)
                return -1;
        else
                return 1;
}

static GList *
cpufreq_monitor_libcpufreq_get_available_frequencies (CPUFreqMonitor *monitor)
{
	GList                *list = NULL;
	guint                 cpu;
	CPUFreqFrequencyList *freqs, *freq;

        g_object_get (G_OBJECT (monitor),
                      "cpu", &cpu, NULL);

	freqs = cpufreq_get_available_frequencies (cpu);
	if (!freqs)
		return NULL;

	for (freq = freqs; freq; freq = freq->next) {
		gchar *frequency;

		frequency = g_strdup_printf ("%lu", freq->frequency);
		
		if (!g_list_find_custom (list, frequency, compare))
			list = g_list_prepend (list, frequency);
		else
			g_free (frequency);
	}

	cpufreq_put_available_frequencies (freqs);

        return g_list_sort (list, compare);
}

static GList *
cpufreq_monitor_libcpufreq_get_available_governors (CPUFreqMonitor *monitor)
{
	guint                cpu;
        GList               *list = NULL;
	CPUFreqGovernorList *govs, *gov;

        g_object_get (G_OBJECT (monitor),
                      "cpu", &cpu, NULL);
        
	govs = cpufreq_get_available_governors (cpu);
	if (!govs)
		return NULL;

	for (gov = govs; gov; gov = gov->next) {
		list = g_list_prepend (list, g_strdup (gov->governor));
	}

	cpufreq_put_available_governors (govs);

        return list;
}