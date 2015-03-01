/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in filter.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include "sip.h"
#include "ui_call_list.h"
#include "filter.h"

//! Storage of filter information
filter_t filters[FILTER_COUNT];

int
filter_set(int type, const char *expr)
{
    // If previous value
    if (filters[type].expr) {
        free(filters[type].expr);
        filters[type].expr = NULL;
        regfree(&filters[type].regex);
    }

    if (expr) {
        // Set the filter text
        filters[type].expr = strdup(expr);
        // Compile regexp for this filter
        regcomp(&filters[type].regex, expr, REG_EXTENDED | REG_ICASE);
    } else {
        filters[type].expr = NULL;
    }

    return 0;
}

const char *
filter_get(int type)
{
    return filters[type].expr;
}

void
filter_stats(int *total, int *displayed)
{
    sip_call_t *call = NULL;

    // Initialize stats
    *total = 0;
    *displayed = 0;

    while ((call = call_get_next(call))) {
        (*total)++;
        if (filter_check_call(call) == 0)
            (*displayed)++;
    }
}

int
filter_check_call(sip_call_t *call)
{
    int i;
    const char *data;
    char linetext[256];

    // Filter for this call has already be processed
    if (call->filtered != -1)
        return call->filtered;

    // By default, call matches all filters
    call->filtered = 0;

    // Check all filter types
    for (i=0; i < FILTER_COUNT; i++) {
        // If filter is not enabled, go to the next
        if (!filters[i].expr)
            continue;

        // Get filtered field
        switch(i) {
            case FILTER_SIPFROM:
                data = call_get_attribute(call, SIP_ATTR_SIPFROM);
                break;
            case FILTER_SIPTO:
                data = call_get_attribute(call, SIP_ATTR_SIPTO);
                break;
            case FILTER_SOURCE:
                data = call_get_attribute(call, SIP_ATTR_SRC);
                break;
            case FILTER_DESTINATION:
                data = call_get_attribute(call, SIP_ATTR_DST);
                break;
            case FILTER_METHOD:
                data = call_get_attribute(call, SIP_ATTR_METHOD);
                break;
            case FILTER_CALL_LIST:
                // FIXME Maybe call should know hot to calculate this line
                memset(linetext, 0, sizeof(linetext));
                data = call_list_line_text(ui_get_panel(ui_find_by_type(PANEL_CALL_LIST)), call, linetext);
                break;
            default:
                // Unknown filter id
                return 0;
        }

        // Call doesn't match this filter
        if (regexec(&filters[i].regex, data, 0, NULL, 0)) {
            // Mak as filtered
            call->filtered = 1;
            break;
        }
    }

    // Return the final filter status
    return call->filtered;
}

void
filter_reset_calls()
{
    sip_call_t *call = NULL;

    // Force filter evaluation
    while ((call = call_get_next(call)))
        call->filtered = -1;
}
