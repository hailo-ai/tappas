/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016 RidgeRun Engineering <manuel.leiva@ridgerun.com>
 *
 * This file is part of GstShark.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>
#include <stdlib.h>

#include "gstparser.h"


struct _GstCTFParser
{
  const parser_handler_desc *parser_desc_list;
  gint parser_desc_list_len;
  parser_handler_function no_match_handler;
};

static gboolean
parse_strcmp (const gchar * ref, gchar ** cmp_string)
{
  gchar *string;

  string = *cmp_string;

  while (*ref == *string && '\0' != *ref) {
    ref++;
    string++;
  }
  /* Verify if the loop reaches the null character */
  if ('\0' == *ref) {
    *cmp_string = string;
    return TRUE;
  }
  return FALSE;
}

GstCTFParser *
parser_init (void)
{
  return g_malloc (sizeof (GstCTFParser));
}


void
parser_finalize (GstCTFParser * parser)
{
  g_return_if_fail (parser);
  g_free (parser);
}


/* Register callback function list matched with a resource name.
 * Also and optional handler callback can be added to process the 
 * resources not identified.
 */
void
parser_register_callbacks (GstCTFParser * parser,
    const parser_handler_desc * parser_handler_desc_list,
    guint list_len, parser_handler_function no_match_handler)
{
  g_return_if_fail (parser);
  g_return_if_fail (parser_handler_desc_list);
  g_return_if_fail (no_match_handler);

  parser->parser_desc_list = parser_handler_desc_list;
  parser->parser_desc_list_len = list_len;
  parser->no_match_handler = no_match_handler;
}


/* Parse the line based in the list of the resources names registered */
void
parser_line (GstCTFParser * parser, gchar * line)
{
  gboolean cmp_res;
  gchar *line_end;
  gchar *next_location;
  guint str_len;
  guint list_idx;
  gboolean match;

  g_return_if_fail (parser);
  g_return_if_fail (line);

  /* Compute the end of the line */
  str_len = strlen (line);

  line_end = line + str_len;

  /* Search next location */
  next_location = line;
  while ((next_location != line_end) && (';' != *next_location)) {
    ++next_location;
  }

  if (';' == *next_location) {
    *next_location = '\0';
    next_location++;
  } else {
    next_location = NULL;
  }

  do {
    match = FALSE;
    for (list_idx = 0; list_idx < parser->parser_desc_list_len; ++list_idx) {
      cmp_res =
          parse_strcmp (parser->parser_desc_list[list_idx].location, &line);
      if (TRUE == cmp_res) {
        parser->parser_desc_list[list_idx].parser_handler (line);
        match = TRUE;

        line = next_location;
        /* Search next location */
        if (next_location == NULL) {
          break;
        }
        while ((next_location != line_end) && (';' != *next_location)) {
          ++next_location;
        }
        if (';' == *next_location) {
          *next_location = '\0';
          next_location++;
        } else {
          next_location = NULL;
        }
      }
    }
    /* if location is not defined */
    if (NULL != parser->no_match_handler && FALSE == match) {
      parser->no_match_handler (line);

      line = next_location;
      /* Search next location */
      if (next_location == NULL) {
        break;
      }
      while ((next_location != line_end) && (';' != *next_location)) {
        ++next_location;
      }
      if (';' == *next_location) {
        *next_location = '\0';
        next_location++;
      } else {
        next_location = NULL;
      }
    }
  } while (line != NULL);
}
