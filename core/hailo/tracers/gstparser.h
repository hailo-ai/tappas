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
#pragma once

#include <glib.h>
#include <glib/gstdio.h>

G_BEGIN_DECLS 

typedef struct _GstCTFParser GstCTFParser;

typedef void (*parser_handler_function) (gchar * location);

typedef struct {
    const char* location;
    parser_handler_function  parser_handler;
} parser_handler_desc;


GstCTFParser * parser_init(void);

void parser_register_callbacks(
    GstCTFParser * parser,
    const parser_handler_desc * parser_handler_desc_list,
    guint list_len,
    parser_handler_function no_match_handler);

void parser_line(GstCTFParser * parser, gchar * option);

void parser_finalize(GstCTFParser * parser);

G_END_DECLS