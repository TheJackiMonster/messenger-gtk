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
#include <stdlib.h>

static void
error_cb(GstBus *bus,
         GstMessage *msg,
         gpointer data)
{
  GError *err;
  gchar *debug_info;

  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);
}

static void
_setup_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  info->stream_source = gst_element_factory_make("appsrc", NULL);
  info->decoder = gst_element_factory_make("rtpL16depay", NULL);
  info->converter = gst_element_factory_make("audioconvert", NULL);

  gst_bin_add_many(GST_BIN(info->discourse->mix_pipeline), info->stream_source, info->decoder, info->converter, NULL);
  gst_element_link_many(info->stream_source, info->decoder, info->converter, NULL);

  {
    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "payload", G_TYPE_INT, 96,
      "clock-rate", G_TYPE_INT, 44100,
      "encoding-name", G_TYPE_STRING, "L16",
      "channels", G_TYPE_INT, 1,
      NULL
    );

    g_object_set(info->stream_source, "format", GST_FORMAT_TIME, "caps", caps, NULL);
    gst_caps_unref(caps);
  }

  info->mix_pad = gst_element_request_pad_simple(
    info->discourse->mix_element, "sink_%u"
  );

  {
    GstPad *pad = gst_element_get_static_pad(
      info->converter, "src"
    );

    g_object_set(info->mix_pad, "mute", FALSE, "volume", 1.0, NULL);
    gst_pad_link(pad, info->mix_pad);
  }

  gst_element_sync_state_with_parent(info->stream_source);
  gst_element_sync_state_with_parent(info->decoder);
  gst_element_sync_state_with_parent(info->converter);
}

static MESSENGER_DiscourseSubscriptionInfo*
discourse_subscription_create_info(MESSENGER_DiscourseInfo *discourse,
                                   struct GNUNET_CHAT_Contact *contact)
{
  g_assert((discourse) && (contact));

  MESSENGER_DiscourseSubscriptionInfo* info = g_malloc(
    sizeof(MESSENGER_DiscourseSubscriptionInfo)
  );

  if (!info)
    return NULL;

  info->discourse = discourse;
  info->contact = contact;

  info->stream_source = NULL;
  info->decoder = NULL;
  info->converter = NULL;

  info->mix_pad = NULL;

  info->position = 0;

  _setup_gst_pipelines_of_subscription(info);
  return info;
}

static void
discourse_subscription_destroy_info(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  gst_element_set_state(info->stream_source, GST_STATE_NULL);
  gst_element_set_state(info->decoder, GST_STATE_NULL);
  gst_element_set_state(info->converter, GST_STATE_NULL);

  if (info->mix_pad)
  {
    GstPad *pad = gst_element_get_static_pad(
      info->converter, "src"
    );

    gst_pad_unlink(pad, info->mix_pad);

    gst_element_release_request_pad(info->discourse->mix_element, info->mix_pad);
    gst_object_unref(GST_OBJECT(info->mix_pad));
  }

  gst_element_unlink_many(info->stream_source, info->decoder, info->converter, NULL);
  gst_bin_remove_many(GST_BIN(info->discourse->mix_pipeline), info->stream_source, info->decoder, info->converter, NULL);

  g_free(info);
}

static void
discourse_subscription_stream_message(MESSENGER_DiscourseSubscriptionInfo *info,
                                      const struct GNUNET_CHAT_Message *message)
{
  g_assert((info) && (message));

  const uint64_t available = GNUNET_CHAT_message_available(message);

  if (!available)
    return;

  const uint64_t samples = available / 2;

  GstBuffer *buffer = gst_buffer_new_and_alloc(available);
  GstFlowReturn ret = GST_FLOW_OK;

  if (!buffer)
    return;

  GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(info->position, GST_SECOND, 44100);
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(samples, GST_SECOND, 44100);

  GstMapInfo mapping;
  if (gst_buffer_map(buffer, &mapping, GST_MAP_WRITE))
  {
    if (mapping.size)
    {
      if (GNUNET_OK != GNUNET_CHAT_message_read(message, (char*) mapping.data, mapping.size))
        memset(mapping.data, 0, mapping.size);
    }

    gst_buffer_unmap(buffer, &mapping);
  }
  else
    goto skip_buffer;

  g_signal_emit_by_name(info->stream_source, "push-buffer", buffer, &ret);
  info->position += samples;

skip_buffer:
  gst_buffer_unref(buffer);

  if (GST_FLOW_OK != ret)
    return;
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

  const gsize size = gst_buffer_get_size(buffer);
  if (!size)
    goto cleanup_sample;

  GstMapInfo mapping;
  if (gst_buffer_map(buffer, &mapping, GST_MAP_READ))
  {
    if (mapping.size)
      GNUNET_CHAT_discourse_write(info->discourse, (const char*) mapping.data, mapping.size);

    gst_buffer_unmap(buffer, &mapping);
  }

cleanup_sample:
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

static void
_setup_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

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

  info->mix_pipeline = gst_parse_launch(
    "audiomixer name=mixer ! volume name=control ! autoaudiosink",
    NULL
  );

  info->mix_element = gst_bin_get_by_name(
    GST_BIN(info->mix_pipeline), "mixer"
  );

  info->volume_element = gst_bin_get_by_name(
    GST_BIN(info->mix_pipeline), "control"
  );

  {
    GstBus *bus = gst_element_get_bus(info->mix_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    gst_element_set_state(info->mix_pipeline, GST_STATE_PLAYING);
  }
}

enum GNUNET_GenericReturnValue
discourse_create_info(struct GNUNET_CHAT_Discourse *discourse)
{
  if ((!discourse) || (GNUNET_CHAT_discourse_get_user_pointer(discourse)))
    return GNUNET_NO;

  MESSENGER_DiscourseInfo* info = g_malloc(sizeof(MESSENGER_DiscourseInfo));

  if (!info)
    return GNUNET_NO;

  info->discourse = discourse;

  info->record_pipeline = NULL;
  info->record_sink = NULL;

  info->mix_pipeline = NULL;
  info->mix_element = NULL;
  info->volume_element = NULL;

  info->subscriptions = NULL;

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

  if (info->subscriptions)
  {
    MESSENGER_DiscourseSubscriptionInfo *sub_info;
    GList *sub = info->subscriptions;
    while (sub)
    {
      sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
      discourse_subscription_destroy_info(sub_info);
      sub = g_list_next(sub);
    }

    g_list_free(info->subscriptions);
  }

  if (info->mix_pipeline)
  {
    gst_element_set_state(info->mix_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->mix_pipeline));
  }

  if (info->record_pipeline)
  {
    gst_element_set_state(info->record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->record_pipeline));
  }

  g_free(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, NULL);
}

static enum GNUNET_GenericReturnValue
_append_contact_to_subscription_list(void *cls,
                                     const struct GNUNET_CHAT_Discourse *discourse,
                                     struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (discourse) && (contact));

  GList **list = cls;

  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

void
discourse_update_subscriptions(struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert(discourse);

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  GList *list = NULL;
  GNUNET_CHAT_discourse_iterate_contacts(
    info->discourse,
    _append_contact_to_subscription_list,
    &list
  );

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info;

  GList *drop = NULL;

  while (sub)
  {
    GList *link = sub;

    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (link->data);
    sub = g_list_next(sub);

    if (g_list_find(list, sub_info->contact))
    {
      list = g_list_remove(list, sub_info->contact);
      continue;
    }

    link = g_list_remove_link(info->subscriptions, link);

    if (!drop)
      drop = link;
    else
      drop = g_list_concat(drop, link);
  }

  sub = drop;
  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    discourse_subscription_destroy_info(sub_info);
    sub = g_list_next(sub);
  }

  if (drop)
    g_list_free(drop);

  sub = list;
  while (sub)
  {
    sub_info = discourse_subscription_create_info(
      info, (struct GNUNET_CHAT_Contact*) sub->data
    );

    if (sub_info)
      info->subscriptions = g_list_append(
        info->subscriptions, sub_info
      );
    
    sub = g_list_next(sub);
  }

  if (list)
    g_list_free(list);
}

void
discourse_stream_message(struct GNUNET_CHAT_Discourse *discourse,
                         const struct GNUNET_CHAT_Message *message)
{
  g_assert((discourse) && (message));

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if (GNUNET_CHAT_message_get_sender(message) == sub_info->contact)
      break;

    sub = g_list_next(sub);
  }

  if (!sub_info)
    return;

  discourse_subscription_stream_message(sub_info, message);
}

bool
discourse_has_controls(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  if ((!(info->record_pipeline)) && (!(info->mix_pipeline)))
    return FALSE;

  return TRUE;
}

void
discourse_set_volume(struct GNUNET_CHAT_Discourse *discourse,
                     double volume)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->mix_pipeline)) || (!(info->volume_element)))
    return;

  g_object_set(info->volume_element, "volume", volume, NULL);
}

double
discourse_get_volume(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->mix_pipeline)) || (!(info->volume_element)))
    return 0.0;

  gdouble volume;
  g_object_get(info->volume_element, "volume", &volume, NULL);

  return volume;
}

void
discourse_set_mute(struct GNUNET_CHAT_Discourse *discourse,
                   bool mute)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->record_pipeline)))
    return;

  gst_element_set_state(
    info->record_pipeline,
    mute? GST_STATE_NULL : GST_STATE_PLAYING
  );
}

bool
discourse_is_mute(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->record_pipeline)))
    return TRUE;

  GstState state;
  gst_element_get_state(
    info->record_pipeline,
    &state,
    NULL,
    GST_CLOCK_TIME_NONE
  );

  return (GST_STATE_PLAYING != state);
}
