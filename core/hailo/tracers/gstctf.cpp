/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016 RidgeRun Engineering <manuel.leiva@ridgerun.com>
 *                                         <sebastian.fatjo@ridgerun.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <string.h>


#include "gstctf.hpp"
#include "gstparser.h"

#define MAX_DIRNAME_LEN (30)

/* Default port */
#define SOCKET_PORT     (1000)
#define SOCKET_PROTOCOL G_SOCKET_PROTOCOL_TCP
#define CTF_MEM_SIZE      (1048576)     //1M = 1024*1024
#define CTF_UUID_SIZE     (16)

typedef guint8 tcp_header_id;
typedef guint32 tcp_header_length;

#define TCP_HEADER_SIZE (sizeof(tcp_header_id) + sizeof(tcp_header_length))
#define CTF_HEADER_SIZE (sizeof(guint16) + sizeof(guint32))
#define CTF_AVAILABLE_MEM_SIZE (CTF_MEM_SIZE - TCP_HEADER_SIZE)

typedef guint16 ctf_header_id;
typedef guint32 ctf_header_timestamp;

/* TCP Section ID */
#define TCP_METADATA_ID    (0x01)
#define TCP_DATASTREAM_ID  (0x02)

#define TCP_EVENT_HEADER_WRITE(id,size,mem) \
  G_STMT_START {                            \
    *(tcp_header_id*)mem = id;              \
    mem += sizeof(tcp_header_id);           \
    *(tcp_header_length*)mem = size;        \
  } G_STMT_END

/* Write string */
#define CTF_EVENT_WRITE_STRING(str,mem)         \
  G_STMT_START {                                \
    mem = (guint8 *)g_stpcpy ((gchar*)mem,str); \
    *(gchar*)mem = '\0';                        \
    ++mem;                                      \
  } G_STMT_END


/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

#ifdef WORDS_BIGENDIAN
#  define CTF_EVENT_WRITE(w,mem,data)      \
  G_STMT_START {                           \
    GST_WRITE_UINT ## w ## _BE (mem,data); \
    mem += sizeof(guint ## w );            \
  } G_STMT_END
#else
#  define CTF_EVENT_WRITE(w,mem,data)      \
  G_STMT_START {                           \
    GST_WRITE_UINT ## w ## _LE (mem,data); \
    mem += sizeof(guint ## w );            \
  } G_STMT_END
#endif

#define CTF_EVENT_WRITE_INT16(int16,mem) \
  CTF_EVENT_WRITE(16,mem,int16)

#define CTF_EVENT_WRITE_INT32(int32,mem) \
  CTF_EVENT_WRITE(32,mem,int32)

#define CTF_EVENT_WRITE_INT64(int64,mem) \
  CTF_EVENT_WRITE(64,mem,int64)

#define CTF_EVENT_WRITE_FLOAT(float_val,mem) \
  G_STMT_START {                             \
    *(gfloat*)mem = float_val;               \
    mem += sizeof(gfloat);                   \
  } G_STMT_END

/* *INDENT-OFF* */
#define CTF_EVENT_WRITE_HEADER(id,mem) \
  G_STMT_START {                       \
    /* Write event ID */               \
    CTF_EVENT_WRITE_INT16(id,mem);     \
    /* Write timestamp */              \
    CTF_EVENT_WRITE_INT32(             \
      GST_CLOCK_DIFF (                 \
          ctf_descriptor->start_time,  \
          gst_util_get_timestamp ()    \
      )/1000,                          \
    mem);                              \
  } G_STMT_END
/* *INDENT-ON* */

static void file_parser_handler (gchar * line);
static void tcp_parser_handler (gchar * line);
static inline gboolean event_exceeds_mem_size (gsize size);

typedef enum
{
  BYTE_ORDER_BE,
  BYTE_ORDER_LE,
} byte_order;


struct _GstCtfDescriptor
{
  guint8 mem[CTF_MEM_SIZE];
  GstClockTime start_time;
  GMutex mutex;
  guint8 uuid[CTF_UUID_SIZE];
  /* This memory space would be used as auxiliar memory to build the stream
   * that will be written in the FILE or in the socket.
   */
  /* File variables */
  FILE *metadata;
  FILE *datastream;
  gchar *dir_name;
  gchar *env_dir_name;
  gboolean file_output_disable;
  gsize file_buf_size;
  gboolean change_file_buf_size;

  /* TCP connection variables */
  gchar *host_name;
  gint port_number;
  GSocketClient *socket_client;
  GSocketConnection *socket_connection;
  GOutputStream *output_stream;
  gboolean tcp_output_disable;
};

static GstCtfDescriptor *ctf_descriptor = NULL;

static const parser_handler_desc parser_handler_desc_list[] = {
  {"file://", file_parser_handler},
  {"tcp://", tcp_parser_handler},
};

/* Metadata format string */
static const gchar metadata_fmt[] = "\
/* CTF 1.8 */\n\
typealias integer { size = 8; align = 8; signed = false; } := uint8_t;\n\
typealias integer { size = 16; align = 8; signed = false; } := uint16_t;\n\
typealias integer { size = 32; align = 8; signed = false; } := uint32_t;\n\
typealias integer { size = 64; align = 8; signed = false; } := uint64_t;\n\
\n\
trace {\n\
    major = %u;\n\
    minor = %u;\n\
    uuid = \"%s\";\n\
    byte_order = %s;\n\
    packet.header := struct {\n\
        uint32_t magic;\n\
        uint8_t  uuid[16];\n\
        uint32_t stream_id;\n\
    };\n\
};\n\
\n\
clock { \n\
    name = monotonic; \n\
    uuid = \"84db105b-b3f4-4821-b662-efc51455106a\"; \n\
    description = \"Monotonic Clock\"; \n\
    freq = 1000000; /* Frequency, in Hz */ \n\
    /* clock value offset from Epoch is: offset * (1/freq) */ \n\
    offset_s = 21600; \n\
};\n\
\n\
typealias integer {\n\
    size = 32; align = 8; signed = false;\n\
    map = clock.monotonic.value;\n\
} := uint32_clock_monotonic_t;\n\
\n\
typealias integer {\n\
    size = 64; align = 8; signed = false;\n\
    map = clock.monotonic.value;\n\
} := uint64_clock_monotonic_t;\n\
\n\
struct packet_context {\n\
    uint64_clock_monotonic_t timestamp_begin;\n\
    uint64_clock_monotonic_t timestamp_end;\n\
};\n\
\n\
struct event_header {\n\
    enum : uint16_t { compact = 0 ... 65534, extended = 65535 } id;\n\
    variant <id> {\n\
        struct {\n\
            uint32_t timestamp;\n\
        } compact;\n\
        struct {\n\
            uint32_t id;\n\
            uint64_t timestamp;\n\
        } extended;\n\
    } v;\n\
} align(8);\n\
\n\
stream {\n\
    id = 0;\n\
    event.header := struct event_header;\n\
    packet.context := struct packet_context;\n\
};\n\
\n\
event {\n\
    name = init;\n\
    id = 0;\n\
    stream_id = 0;\n\
};\n\
";


static gboolean
event_exceeds_mem_size (const gsize size)
{
  gboolean ret;

  /* Protect against overflows */
  if (size > CTF_AVAILABLE_MEM_SIZE) {
    GST_ERROR ("Metadata event exceeds available memory and will not be added");
    ret = TRUE;
  } else {
    ret = FALSE;
  }

  return ret;
}

static GstCtfDescriptor *
ctf_create_struct (void)
{
  gchar UUID[] =
      { static_cast<gchar>(0xd1), static_cast<gchar>(0x8e), static_cast<gchar>(0x63), static_cast<gchar>(0x74), static_cast<gchar>(0x35), static_cast<gchar>(0xa1), static_cast<gchar>(0xcd), static_cast<gchar>(0x42), static_cast<gchar>(0x8e), static_cast<gchar>(0x70), static_cast<gchar>(0xa9), static_cast<gchar>(0xcf),
    static_cast<gchar>(0xfa), static_cast<gchar>(0x71), static_cast<gchar>(0x27), static_cast<gchar>(0x93)
  };
  GstCtfDescriptor *ctf;

  ctf = (GstCtfDescriptor*)g_malloc (sizeof (GstCtfDescriptor));
  if (NULL == ctf) {
    GST_ERROR ("CTF descriptor could not be created.");
    return NULL;
  }

  /* File variables */
  ctf->dir_name = NULL;
  ctf->env_dir_name = NULL;
  ctf->file_buf_size = 0;
  ctf->change_file_buf_size = FALSE;

  /* Default state Enable */
  ctf->file_output_disable = FALSE;

  ctf->metadata = NULL;
  ctf->datastream = NULL;

  /* TCP connection variables */
  ctf->host_name = NULL;
  ctf->port_number = SOCKET_PORT;

  ctf->socket_client = NULL;
  ctf->socket_connection = NULL;
  ctf->output_stream = NULL;

  /* Default TCP connection state Enable */
  ctf->tcp_output_disable = FALSE;

  /* Currently a constant UUID value is used */
  memcpy (ctf->uuid, UUID, CTF_UUID_SIZE);

  return ctf;
}

static void
generate_datastream_header (void)
{
  guint64 time_stamp_begin;
  guint64 time_stamp_end;
  guint32 magic = 0xC1FC1FC1;
  guint32 padding;
  gint32 stream_id;
  guint8 *mem;
  guint event_size;
  guint8 *event_mem;
  GError *error;

  stream_id = 0;

  event_size = CTF_UUID_SIZE + 4 + 8 + 8 + 4 + 4;
  /* Create Stream */
  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  g_mutex_lock (&ctf_descriptor->mutex);
  /* The begin of the data stream header is compound by the Magic Number,
     the trace UUID and the Stream ID. These are all required fields. */
  /* Magic Number */
  CTF_EVENT_WRITE_INT32 (magic, event_mem);
  /* Trace UUID */
  memcpy (event_mem, ctf_descriptor->uuid, CTF_UUID_SIZE);
  event_mem += CTF_UUID_SIZE;
  /* Stream ID */
  CTF_EVENT_WRITE_INT32 (stream_id, event_mem);
  /* Time Stamp begin */
  time_stamp_begin = 0;
  CTF_EVENT_WRITE_INT64 (time_stamp_begin, event_mem);
  /* Time Stamp end */
  time_stamp_end = 0;
  CTF_EVENT_WRITE_INT64 (time_stamp_end, event_mem);
  /* Padding needed */
  padding = 0x0000FFFF;
  CTF_EVENT_WRITE_INT32 (padding, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

static void
uuid_to_uuidstring (gchar * uuid_string, guint8 * uuid)
{
  gchar *uuid_string_idx;
  gint32 byte;
  gint uuid_idx;

  uuid_string_idx = uuid_string;
  uuid_idx = 0;
  for (uuid_idx = 0; uuid_idx < 4; ++uuid_idx) {
    byte = 0xFF & uuid[uuid_idx];

    g_sprintf (uuid_string_idx, "%x", byte);
    uuid_string_idx += 2;
  }
  *(uuid_string_idx++) = '-';

  for (; uuid_idx < 6; ++uuid_idx) {
    byte = 0xFF & uuid[uuid_idx];
    g_sprintf (uuid_string_idx, "%x", byte);
    uuid_string_idx += 2;
  }
  *(uuid_string_idx++) = '-';

  for (; uuid_idx < 8; ++uuid_idx) {
    byte = 0xFF & uuid[uuid_idx];

    g_sprintf (uuid_string_idx, "%x", byte);
    uuid_string_idx += 2;
  }
  *(uuid_string_idx++) = '-';

  for (; uuid_idx < 10; ++uuid_idx) {
    byte = 0xFF & uuid[uuid_idx];
    g_sprintf (uuid_string_idx, "%x", byte);
    uuid_string_idx += 2;
  }
  *(uuid_string_idx++) = '-';

  for (; uuid_idx < 16; ++uuid_idx) {
    byte = 0xFF & uuid[uuid_idx];
    g_sprintf (uuid_string_idx, "%x", byte);
    uuid_string_idx += 2;
  }

  *++uuid_string_idx = 0;
}

static void
generate_metadata (gint major, gint minor, gint byte_order)
{
  gint str_len;
  GError *error;
  guint8 *event_mem;
  guint8 *mem;
  gchar uuid_string[] = "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX0";

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;
  /* Writing the first sections of the metadata file with the structures
     and the definitions that will be needed in the future. */

  uuid_to_uuidstring (uuid_string, ctf_descriptor->uuid);

  g_mutex_lock (&ctf_descriptor->mutex);

  str_len =
      g_snprintf ((gchar *) event_mem, CTF_MEM_SIZE, metadata_fmt, major, minor,
      uuid_string, byte_order ? "le" : "be");
  if (CTF_MEM_SIZE == str_len) {
    GST_ERROR ("Insufficient memory to create metadata");
    return;
  }

  if (FALSE == ctf_descriptor->file_output_disable) {
    fwrite (event_mem, sizeof (gchar), str_len, ctf_descriptor->metadata);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_METADATA_ID, str_len, mem);
    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, str_len + TCP_HEADER_SIZE, NULL, &error);
  }
  g_mutex_unlock (&ctf_descriptor->mutex);
}

gchar *
get_ctf_path_name (void)
{
  return ctf_descriptor->dir_name;
}

static void
tcp_parser_handler (gchar * line)
{
  gchar *line_end;
  gchar *host_name;
  gchar *port_name;
  gchar *port_name_end;
  gsize str_len;

  host_name = line;
  line_end = line;
  while (('\0' != *line_end) && (':' != *line_end)) {
    ++line_end;
  }

  if (*line_end == '\0') {
    str_len = strlen (host_name);

    ctf_descriptor->host_name = (gchar*)g_malloc (str_len + 1);

    strcpy (ctf_descriptor->host_name, host_name);
    /* End of the line, finish parser process */
    return;
  }
  if (*line_end == ':') {
    /* Get the port value */
    *line_end = '\0';

    str_len = strlen (host_name);

    ctf_descriptor->host_name = (gchar*)g_malloc (str_len + 1);

    strcpy (ctf_descriptor->host_name, host_name);

    ++line_end;
    port_name = line_end;

    ctf_descriptor->port_number = g_ascii_strtoull (port_name,
        &port_name_end, 10);

    /* Verify if the convertion of the string works */
    if ('\0' != *port_name_end || '-' == port_name[0]) {
      ctf_descriptor->port_number = SOCKET_PORT;
      GST_ERROR ("Invalid port number \"%s\", using the default value: %d",
          port_name, ctf_descriptor->port_number);
    }

    return;
  }
}

static void
file_parser_handler (gchar * line)
{
  gsize str_len;

  str_len = strlen (line);
  ctf_descriptor->env_dir_name = (gchar*)g_malloc (str_len + 1);
  strcpy (ctf_descriptor->env_dir_name, line);
}

static void
ctf_process_env_var (void)
{
  const gchar *env_loc_value;
  const gchar *env_file_buf_value;
  gchar *env_file_buf_value_end;
  gchar dir_name[MAX_DIRNAME_LEN];
  gchar *env_dir_name;
  gchar *env_line;
  gint size_env_path = 0;
  gint str_len;
  GstCTFParser *parser;
  time_t now = time (NULL);

  env_loc_value = g_getenv ("HAILO_PROFILE_LOCATION");

  if (NULL != env_loc_value) {

    parser = parser_init ();

    parser_register_callbacks (parser,
        parser_handler_desc_list,
        sizeof (parser_handler_desc_list) / sizeof (parser_handler_desc),
        file_parser_handler);

    str_len = strlen (env_loc_value);

    env_line = (gchar*)g_malloc (str_len + 1);

    strcpy (env_line, env_loc_value);

    parser_line (parser, env_line);

    parser_finalize (parser);

    g_free (env_line);
  }

  env_file_buf_value = g_getenv ("GST_SHARK_FILE_BUFFERING");

  if (NULL != env_file_buf_value) {
    ctf_descriptor->file_buf_size =
        g_ascii_strtoull (env_file_buf_value, &env_file_buf_value_end, 10);
    if ('\0' == *env_file_buf_value_end && '-' != env_file_buf_value[0]) {
      ctf_descriptor->change_file_buf_size = TRUE;
    } else {
      GST_ERROR ("Invalid buffer size \"%s\", using default system value",
          env_file_buf_value);
    }
  }

  if (G_UNLIKELY (g_getenv ("GST_SHARK_CTF_DISABLE") != NULL)) {
    env_dir_name = (gchar *) g_getenv ("PWD");
    ctf_descriptor->file_output_disable = TRUE;
  } else {
    env_dir_name = ctf_descriptor->env_dir_name;
  }

  if (G_LIKELY (env_dir_name == NULL)) {
    /* Creating the output folder for the CTF output files. */
    strftime (dir_name, MAX_DIRNAME_LEN, "gstshark_%F_%T", localtime (&now));
    ctf_descriptor->dir_name = (gchar*)g_malloc (MAX_DIRNAME_LEN + 1);
    g_stpcpy (ctf_descriptor->dir_name, dir_name);
  } else {
    size_env_path = strlen (env_dir_name);
    ctf_descriptor->dir_name = (gchar*)g_malloc (size_env_path + 1);
    g_stpcpy (ctf_descriptor->dir_name, env_dir_name);
  }
}

static int
create_ctf_path (gchar * dir_name)
{
  int ret = 0;

  g_return_val_if_fail (dir_name, -1);

  if (!g_file_test (dir_name, G_FILE_TEST_EXISTS)) {
    ret = g_mkdir (dir_name, 0755);
    if (ret == 0) {
      GST_INFO ("Directory %s did not exist and was created sucessfully.",
          dir_name);
    } else {
      GST_ERROR ("Directory %s could not be created.", dir_name);
    }
  } else {
    GST_INFO ("Directory %s already exists in the current path.", dir_name);
  }

  return ret;
}

static void
ctf_file_init (void)
{
  gchar *metadata_file = NULL;
  gchar *datastream_file = NULL;

  g_mutex_init (&ctf_descriptor->mutex);

  if (TRUE != ctf_descriptor->file_output_disable) {
    /* Creating the output folder for the CTF output files. */
    if (create_ctf_path (ctf_descriptor->dir_name) == 0) {
      datastream_file =
          g_strjoin (G_DIR_SEPARATOR_S, ctf_descriptor->dir_name, "datastream",
          NULL);
      metadata_file =
          g_strjoin (G_DIR_SEPARATOR_S, ctf_descriptor->dir_name, "metadata",
          NULL);

      ctf_descriptor->datastream = g_fopen (datastream_file, "w");
      if (ctf_descriptor->datastream == NULL) {
        GST_ERROR ("Could not open datastream file, path does not exist.");
        goto error;
      }

      ctf_descriptor->metadata = g_fopen (metadata_file, "w");
      if (ctf_descriptor->metadata == NULL) {
        GST_ERROR ("Could not open metadata file, path does not exist.");
        goto error;
      }

      if (ctf_descriptor->change_file_buf_size) {
        if (ctf_descriptor->file_buf_size == 0) {
          setvbuf (ctf_descriptor->metadata, NULL, _IONBF, 0);
          setvbuf (ctf_descriptor->datastream, NULL, _IONBF, 0);
        } else {
          setvbuf (ctf_descriptor->metadata, NULL, _IOFBF,
              ctf_descriptor->file_buf_size);
          setvbuf (ctf_descriptor->datastream, NULL, _IOFBF,
              ctf_descriptor->file_buf_size);
        }
      }

      ctf_descriptor->start_time = gst_util_get_timestamp ();
      ctf_descriptor->file_output_disable = FALSE;

      g_free (datastream_file);
      g_free (metadata_file);
      return;
    } else {
      GST_ERROR ("Could not create CTF output files, ignoring them");
      ctf_descriptor->file_output_disable = TRUE;
      return;
    }
  }

  return;

error:
  ctf_descriptor->file_output_disable = TRUE;
  g_free (datastream_file);
  g_free (metadata_file);
}


static void
ctf_tcp_init (void)
{
  GSocketClient *socket_client;
  GOutputStream *output_stream;
  GSocketConnection *socket_connection;
  GError *error;

  /* Verify if the host name was given */
  if (NULL == ctf_descriptor->host_name) {
    ctf_descriptor->tcp_output_disable = TRUE;
    return;
  }

  /* Creates a new GSocketClient with the default options. */
  socket_client = g_socket_client_new ();

  g_socket_client_set_protocol (socket_client, SOCKET_PROTOCOL);

  /* Attempts to create a TCP connection to the named host. */
  error = NULL;
  socket_connection = g_socket_client_connect_to_host (socket_client,
      ctf_descriptor->host_name, ctf_descriptor->port_number, NULL, &error);

  /* Verify connection */
  if (NULL == socket_connection) {
    g_object_unref (socket_client);
    socket_client = NULL;
    ctf_descriptor->tcp_output_disable = TRUE;
    return;
  }

  output_stream =
      g_io_stream_get_output_stream (G_IO_STREAM (socket_connection));
  /* Store connection variables */
  ctf_descriptor->socket_client = socket_client;
  ctf_descriptor->socket_connection = socket_connection;
  ctf_descriptor->output_stream = output_stream;
}

gboolean
gst_ctf_init (void)
{
  if (ctf_descriptor) {
    GST_ERROR ("CTF Descriptor already exists.");
    return FALSE;
  }

  /* Since the descriptors structure does not exist it is needed to
     create and initialize a new one. */
  ctf_descriptor = ctf_create_struct ();
  /* Load and proccess enviroment variables */
  ctf_process_env_var ();

  ctf_file_init ();
  ctf_tcp_init ();

  generate_metadata (1, 3, BYTE_ORDER_LE);
  generate_datastream_header ();
  do_print_ctf_init (INIT_EVENT_ID);


  return TRUE;
}

void
add_metadata_event_struct (const gchar * metadata_event)
{
  GError *error;
  gchar *event_mem;
  guint event_size;
  guint8 *mem;

  event_size = strlen (metadata_event);

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = (gchar *) mem + TCP_HEADER_SIZE;

  /* This function only writes the event structure to the metadata file, it
     depends entirely of what is passed as an argument. */
  g_mutex_lock (&ctf_descriptor->mutex);

  memcpy (event_mem, metadata_event, event_size);

  if (FALSE == ctf_descriptor->file_output_disable) {
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->metadata);
  }
  if (FALSE == ctf_descriptor->tcp_output_disable) {
    TCP_EVENT_HEADER_WRITE (TCP_METADATA_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }
  g_mutex_unlock (&ctf_descriptor->mutex);
}


void
do_print_cpuusage_event (event_id id, guint32 cpu_num, gfloat * cpuload)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;
  guint cpu_idx;

  event_size = cpu_num * sizeof (gfloat) + CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem, datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Write CPU load for each CPU */
  for (cpu_idx = 0; cpu_idx < cpu_num; ++cpu_idx) {
    /* Write CPU load */
    CTF_EVENT_WRITE_FLOAT (cpuload[cpu_idx], event_mem);
  }

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_proctime_event (event_id id, gchar * elementname, guint64 time)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size = strlen (elementname) + 1 + sizeof (time) + CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Write element name */
  CTF_EVENT_WRITE_STRING (elementname, event_mem);
  /* Write time */
  CTF_EVENT_WRITE_INT64 (time, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem -= event_size;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_framerate_event (event_id id, gchar * elementname, guint64 fps)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size = strlen (elementname) + 1 + sizeof (guint64) + CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Write element name */
  CTF_EVENT_WRITE_STRING (elementname, event_mem);
  /* Write fps */
  CTF_EVENT_WRITE_INT64 (fps, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_interlatency_event (event_id id,
    gchar * originpad, gchar * destinationpad, guint64 time)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size =
      strlen (originpad) + 1 + strlen (destinationpad) + 1 + sizeof (guint64) +
      CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Add event payload */
  /* Write origin pad name */
  CTF_EVENT_WRITE_STRING (originpad, event_mem);
  /* Write destination pad name */
  CTF_EVENT_WRITE_STRING (destinationpad, event_mem);
  /* Write time */
  CTF_EVENT_WRITE_INT64 (time, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }
  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_scheduling_event (event_id id, gchar * elementname, guint64 time)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size = strlen (elementname) + 1 + sizeof (guint64) + CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Add event payload */
  /* Write element name */
  CTF_EVENT_WRITE_STRING (elementname, event_mem);
  /* Write time */
  CTF_EVENT_WRITE_INT64 (time, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_queue_level_event (event_id id, const gchar * elementname,
    guint32 bytes, guint32 max_bytes, guint32 buffers, guint32 max_buffers,
    guint64 time, guint64 max_time)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size =
      strlen (elementname) + 1 + 4 * sizeof (guint32) + 2 * sizeof (guint64) +
      CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Add event payload */
  /* Write element name */
  CTF_EVENT_WRITE_STRING (elementname, event_mem);

  /* Write bytes */
  CTF_EVENT_WRITE_INT32 (bytes, event_mem);

  /* Write bytes */
  CTF_EVENT_WRITE_INT32 (max_bytes, event_mem);

  /* Write buffers */
  CTF_EVENT_WRITE_INT32 (buffers, event_mem);

  /* Write buffers */
  CTF_EVENT_WRITE_INT32 (max_buffers, event_mem);

  /* Write time */
  CTF_EVENT_WRITE_INT64 (time, event_mem);

  /* Write time */
  CTF_EVENT_WRITE_INT64 (max_time, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_bitrate_event (event_id id, gchar * elementname, guint64 bps)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size = strlen (elementname) + 1 + sizeof (bps) + CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Write element name */
  CTF_EVENT_WRITE_STRING (elementname, event_mem);
  /* Write bitrate */
  CTF_EVENT_WRITE_INT64 (bps, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_buffer_event (event_id id, const gchar * pad, GstClockTime pts,
    GstClockTime dts, GstClockTime duration, guint64 offset,
    guint64 offset_end, guint64 size, GstBufferFlags flags, guint32 refcount)
{
  GError *error;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  event_size =
      strlen (pad) + 1 + 6 * sizeof (guint64) + 2 * sizeof (guint32) +
      CTF_HEADER_SIZE;

  if (event_exceeds_mem_size (event_size)) {
    return;
  }

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);

  /* Write event specific fields */
  CTF_EVENT_WRITE_STRING (pad, event_mem);
  CTF_EVENT_WRITE_INT64 (pts, event_mem);
  CTF_EVENT_WRITE_INT64 (dts, event_mem);
  CTF_EVENT_WRITE_INT64 (duration, event_mem);
  CTF_EVENT_WRITE_INT64 (offset, event_mem);
  CTF_EVENT_WRITE_INT64 (offset_end, event_mem);
  CTF_EVENT_WRITE_INT64 (size, event_mem);
  CTF_EVENT_WRITE_INT32 (flags, event_mem);
  CTF_EVENT_WRITE_INT32 (refcount, event_mem);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem -= event_size;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
do_print_ctf_init (event_id id)
{
  GError *error;
  guint32 unknown = 0;
  guint8 *mem;
  guint8 *event_mem;
  gsize event_size;

  mem = ctf_descriptor->mem;
  event_mem = mem + TCP_HEADER_SIZE;

  /* Lock mem and datastream and output_stream resources */
  g_mutex_lock (&ctf_descriptor->mutex);
  /* Add CTF header */
  CTF_EVENT_WRITE_HEADER (id, event_mem);
  /* Write padding */
  CTF_EVENT_WRITE_INT32 (unknown, event_mem);

  /* Computer event size */
  event_size = event_mem - (mem + TCP_HEADER_SIZE);

  if (FALSE == ctf_descriptor->file_output_disable) {
    event_mem = mem + TCP_HEADER_SIZE;
    fwrite (event_mem, sizeof (gchar), event_size, ctf_descriptor->datastream);
  }

  if (FALSE == ctf_descriptor->tcp_output_disable) {
    /* Write the TCP header */
    TCP_EVENT_HEADER_WRITE (TCP_DATASTREAM_ID, event_size, mem);

    g_output_stream_write (ctf_descriptor->output_stream,
        ctf_descriptor->mem, event_size + TCP_HEADER_SIZE, NULL, &error);
  }

  g_mutex_unlock (&ctf_descriptor->mutex);
}

void
gst_ctf_close (void)
{
  GError *error;
  gboolean res;
  fclose (ctf_descriptor->metadata);
  fclose (ctf_descriptor->datastream);
  g_mutex_clear (&ctf_descriptor->mutex);

  if (NULL != ctf_descriptor->dir_name) {
    g_free (ctf_descriptor->dir_name);
  }
  if (NULL != ctf_descriptor->host_name) {
    g_free (ctf_descriptor->host_name);
  }
  /* Closes the stream, releasing resources related to it. */
  if (NULL != ctf_descriptor->output_stream) {
    res = g_output_stream_close (ctf_descriptor->output_stream, NULL, &error);
    if (FALSE == res) {
      GST_ERROR ("Failed to close output stream");
    }
  }

  if (NULL != ctf_descriptor->socket_client) {
    g_object_unref (ctf_descriptor->socket_client);
  }

  g_free (ctf_descriptor);
}
