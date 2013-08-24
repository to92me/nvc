//
//  Copyright (C) 2013  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "util.h"
#include "rt.h"
#include "tree.h"
#include "common.h"
#include "lxt_write.h"

#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define MAX_VALS 256

typedef struct lxt_data lxt_data_t;

typedef void (*lxt_fmt_fn_t)(tree_t, lxt_data_t *);

typedef struct lxt_data {
   struct lt_symbol *sym;
   lxt_fmt_fn_t      fmt;
   range_kind_t      dir;
   const char       *map;
} lxt_data_t;

static struct lt_trace *trace = NULL;
static tree_t           lxt_top;
static ident_t          lxt_data_i;
static lxttime_t        last_time;

static const char std_logic_map[] = "UX01ZWLH-";
static const char bit_map[]       = "01";

static void lxt_close_trace(void)
{
   if (trace != NULL) {
      lt_close(trace);
      trace = NULL;
   }
}

static void lxt_fmt_int(tree_t decl, lxt_data_t *data)
{
   uint64_t val;
   rt_signal_value(decl, &val, 1, false);

   lt_emit_value_int(trace, data->sym, 0, val);
}

static void lxt_fmt_chars(tree_t decl, lxt_data_t *data)
{
   uint64_t vals[MAX_VALS];
   const int nvals = rt_signal_value(decl, vals, MAX_VALS, false);

   char bits[MAX_VALS + 1];
   bits[nvals] = '\0';
   if (data->map != NULL) {
      for (int i = 0; i < nvals; i++)
         bits[i] = data->map[vals[(data->dir == RANGE_TO) ? i : nvals - i - 1]];
      lt_emit_value_bit_string(trace, data->sym, 0, bits);
   }
   else {
      for (int i = 0; i < nvals; i++)
         bits[i] = vals[(data->dir == RANGE_TO) ? i : nvals - i - 1];
      lt_emit_value_string(trace, data->sym, 0, bits);
   }
}

static void lxt_event_cb(uint64_t now, tree_t decl)
{
   if (now != last_time) {
      lt_set_time64(trace, now);
      last_time = now;
   }

   lxt_data_t *data = tree_attr_ptr(decl, lxt_data_i);
   (*data->fmt)(decl, data);
}

static char *lxt_fmt_name(tree_t decl)
{
   char *s = strdup(istr(tree_ident(decl)) + 1);
   for (char *p = s; *p != '\0'; p++) {
      if (*p == ':')
         *p = '.';
   }
   return s;
}

void lxt_restart(void)
{
   if (trace == NULL)
      return;

   lt_set_timescale(trace, -15);

   const int ndecls = tree_decls(lxt_top);
   for (int i = 0; i < ndecls; i++) {
      tree_t d = tree_decl(lxt_top, i);
      if (tree_kind(d) != T_SIGNAL_DECL)
         continue;

      type_t type = tree_type(d);

      int rows, msb;
      if (type_is_array(type)) {
         rows = type_dims(type) - 1;
         if ((rows > 0) || type_is_array(type_elem(type))) {
            warn_at(tree_loc(d), "cannot emit arrays of greater than one "
                    "dimension or arrays of arrays in LXT yet");
            continue;
         }

         int64_t low, high;
         range_bounds(type_dim(type, 0), &low, &high);
         msb = high - low;
      }
      else
         msb = rows = 0;

      lxt_data_t *data = xmalloc(sizeof(lxt_data_t));
      memset(data, '\0', sizeof(lxt_data_t));

      int flags = 0;

      if (type_is_array(type)) {
         // Only arrays of CHARACTER, BIT, STD_ULOGIC are supported
         type_t elem = type_base_recur(type_elem(type));
         switch (type_kind(elem)) {
         case T_ENUM:
            {
               ident_t name = type_ident(elem);
               if (icmp(name, "IEEE.STD_LOGIC_1164.STD_ULOGIC")) {
                  data->fmt = lxt_fmt_chars;
                  data->map = std_logic_map;
                  flags = LT_SYM_F_BITS;
                  break;
               }
               else if (icmp(name, "STD.STANDARD.BIT")) {
                  data->fmt = lxt_fmt_chars;
                  data->map = bit_map;
                  flags = LT_SYM_F_BITS;
               }
               else if (icmp(name, "STD.STANDARD.CHARACTER")) {
                  data->fmt = lxt_fmt_chars;
                  data->map = NULL;
                  flags = LT_SYM_F_STRING;
               }
            }
            // Fall-through

         default:
            warn_at(tree_loc(d), "cannot represent arrays of type %s "
                    "in LXT format", type_pp(elem));
            free(data);
            continue;
         }

         data->dir = type_dim(type, 0).kind;
      }
      else {
         switch (type_kind(type)) {
         case T_INTEGER:
            data->fmt = lxt_fmt_int;
            flags = LT_SYM_F_INTEGER;
            break;

         default:
            warn_at(tree_loc(d), "cannot represent type %s in LXT format",
                    type_pp(type));
            free(data);
            continue;
         }
      }

      char *name = lxt_fmt_name(d);
      data->sym = lt_symbol_add(trace, name, rows, msb, 0, flags);
      free(name);

      tree_add_attr_ptr(d, lxt_data_i, data);

      rt_set_event_cb(d, lxt_event_cb);

      (*data->fmt)(d, data);
   }

   last_time = (lxttime_t)-1;
}

void lxt_finish(uint64_t now)
{
   lt_set_time64(trace, now);
   lxt_close_trace();
}

void lxt_init(const char *filename, tree_t top)
{
   lxt_data_i = ident_new("lxt_data");

   if ((trace = lt_init(filename)) == NULL)
      fatal("lt_init failed");

   atexit(lxt_close_trace);

   lxt_top = top;
}