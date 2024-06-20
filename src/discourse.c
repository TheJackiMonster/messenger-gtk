/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author Tobias Frisch
 * @file discourse.c
 */

#include "discourse.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gstreamer-1.0/gst/gst.h>

static void
_setup_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  // TODO: Create stream_pipeline per subscription in discourse!

  info->stream_pipeline = gst_parse_launch(
    "appsrc name=src ! rtpL16depay ! audioconvert",
    NULL
  );

  info->stream_src = gst_bin_get_by_name(
    GST_BIN(info->stream_pipeline), "src"
  );

  // TODO: Connect stream_pipeline to audiomixer element of mix_pipeline!
  // https://gstreamer.freedesktop.org/documentation/audiomixer/audiomixer.html?gi-language=c
}

static GstFlowReturn
_new_audio_sample(GstElement *sink, gpointer data)
{
  g_assert((sink) && (data));

  MESSENGER_DiscourseInfo *info = (MESSENGER_DiscourseInfo*) data;

  GstSample *sample;
  g_signal_emit_by_name(sink, "pull-sample", &sample);

  if (!sample)
    return GST_FLOW_ERROR;

  GstBuffer *buffer = gst_sample_get_buffer(sample);

  if (!buffer)
    goto cleanup_sample;

  gsize size = gst_buffer_get_size(buffer);
  if (!size)
    goto cleanup_sample;

  GstMapInfo mapping;
  if (gst_buffer_map(buffer, &mapping, GST_MAP_READ))
  {
    GNUNET_CHAT_discourse_write(info->discourse, (const char*) mapping.data, mapping.size);
    gst_buffer_unmap(buffer, &mapping);
  }

cleanup_sample:
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

/* This function is called when an error message is posted on the bus */
static void
error_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);
}

static void
_setup_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

  // TODO: Use record_pipeline to record and send data via discourse!
  // https://gstreamer.freedesktop.org/documentation/tutorials/basic/short-cutting-the-pipeline.html?gi-language=c
  // https://gstreamer.freedesktop.org/documentation/app/appsink.html?gi-language=c

  info->record_pipeline = gst_parse_launch(
    "autoaudiosrc ! audioconvert ! rtpL16pay ! appsink name=sink",
    NULL
  );

  info->record_sink = gst_bin_get_by_name(
    GST_BIN(info->record_pipeline), "sink"
  );

  {
    GstBus *bus = gst_element_get_bus(info->record_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "payload", G_TYPE_INT, 96,
      "clock-rate", G_TYPE_INT, 44100,
      "encoding-name", G_TYPE_STRING, "L16",
      "channels", G_TYPE_INT, 1,
      NULL
    );

    g_object_set(info->record_sink, "emit-signals", TRUE, "caps", caps, NULL);
    g_signal_connect(info->record_sink, "new-sample", G_CALLBACK(_new_audio_sample), info);
    
    gst_caps_unref(caps);

    gst_element_set_state(info->record_pipeline, GST_STATE_PLAYING);
  }

  // TODO: Have mix_pipeline in background while being subscribed to discourse!

  info->mix_pipeline = gst_parse_launch(
    "audiomixer name=mixer ! autoaudiosink",
    NULL
  );

  info->mix_element = gst_bin_get_by_name(
    GST_BIN(info->mix_pipeline), "mixer"
  );
}

enum GNUNET_GenericReturnValue
discourse_create_info(struct GNUNET_CHAT_Discourse *discourse)
{
  if ((!discourse) || (GNUNET_CHAT_discourse_get_user_pointer(discourse)))
    return GNUNET_NO;

  MESSENGER_DiscourseInfo* info = g_malloc(sizeof(MESSENGER_DiscourseInfo));

  info->discourse = discourse;

  info->record_pipeline = NULL;
  info->record_sink = NULL;

  info->mix_pipeline = NULL;
  info->mix_element = NULL;

  _setup_gst_pipelines(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, info);
  return GNUNET_YES;
}

void
discourse_destroy_info(struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert(discourse);

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  if (info->record_pipeline)
  {
    gst_element_set_state(info->record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->record_pipeline));
  }

  if (info->mix_pipeline)
  {
    gst_element_set_state(info->mix_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->mix_pipeline));
  }

  g_free(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, NULL);
}
