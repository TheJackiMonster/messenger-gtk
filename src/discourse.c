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

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <pthread.h>
#include <stdlib.h>

const struct GNUNET_ShortHashCode*
get_voice_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_ShortHashCode id;

  if (GNUNET_YES != init)
  {
    memset(&id, 0, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

const struct GNUNET_ShortHashCode*
get_video_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_ShortHashCode id;

  if (GNUNET_YES != init)
  {
    memset(&id, 1, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

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
_setup_audio_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  info->audio_stream_source = gst_element_factory_make("appsrc", NULL);
  info->audio_converter = gst_element_factory_make("audioconvert", NULL);

  gst_bin_add_many(
    GST_BIN(info->discourse->audio_mix_pipeline),
    info->audio_stream_source,
    info->audio_converter,
    NULL
  );

  gst_element_link_many(
    info->audio_stream_source,
    info->audio_converter,
    NULL
  );

  {
    GstCaps *caps = gst_caps_new_simple (
      "audio/x-raw",
      "format", G_TYPE_STRING, "S16BE",
      "layout", G_TYPE_STRING, "interleaved",
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 1,
      NULL
    );

    g_object_set(
      info->audio_stream_source,
      "format", GST_FORMAT_TIME,
      "caps", caps,
      "is-live", TRUE,
      NULL
    );

    gst_caps_unref(caps);
  }

  info->audio_mix_pad = gst_element_request_pad_simple(
    info->discourse->audio_mix_element, "sink_%u"
  );

  {
    GstPad *pad = gst_element_get_static_pad(
      info->audio_converter, "src"
    );

    g_object_set(info->audio_mix_pad, "mute", FALSE, "volume", 1.0, NULL);
    gst_pad_link(pad, info->audio_mix_pad);
  }

  gst_element_sync_state_with_parent(info->audio_stream_source);
  gst_element_sync_state_with_parent(info->audio_converter);
}

static void
_setup_video_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  info->video_stream_pipeline = gst_parse_launch(
    "appsrc name=source ! avdec_h264 ! videoconvert ! gtksink name=sink",
    NULL
  );

  info->video_stream_source = gst_bin_get_by_name(
    GST_BIN(info->video_stream_pipeline), "source"
  );

  info->video_stream_sink = gst_bin_get_by_name(
    GST_BIN(info->video_stream_pipeline), "sink"
  );

  {
    GstBus *bus = gst_element_get_bus(info->video_stream_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "video/x-h264",
      "stream-format", G_TYPE_STRING, "avc",
      "alignment", G_TYPE_STRING, "au",
      NULL
    );

    g_object_set(
      info->video_stream_source,
      "format", GST_FORMAT_TIME,
      "caps", caps,
      "is-live", TRUE,
      NULL
    );

    gst_caps_unref(caps);

    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);
  }
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

  info->audio_stream_source = NULL;
  info->audio_converter = NULL;

  info->video_stream_pipeline = NULL;
  info->video_stream_source = NULL;
  info->video_stream_sink = NULL;

  info->audio_mix_pad = NULL;

  info->position = 0;

  const struct GNUNET_ShortHashCode *id = GNUNET_CHAT_discourse_get_id(
    info->discourse->discourse
  );

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
    _setup_audio_gst_pipelines_of_subscription(info);
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    _setup_video_gst_pipelines_of_subscription(info);

  return info;
}

static void
discourse_subscription_destroy_info(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  if (info->audio_stream_source)
    gst_element_set_state(info->audio_stream_source, GST_STATE_NULL);

  if (info->audio_converter)
    gst_element_set_state(info->audio_converter, GST_STATE_NULL);

  if (info->video_stream_pipeline)
  {
    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->video_stream_pipeline));
  }

  if (info->audio_mix_pad)
  {
    GstPad *pad = gst_element_get_static_pad(
      info->audio_converter, "src"
    );

    gst_pad_unlink(pad, info->audio_mix_pad);

    gst_element_release_request_pad(info->discourse->audio_mix_element, info->audio_mix_pad);
    gst_object_unref(GST_OBJECT(info->audio_mix_pad));
  }

  if ((info->audio_stream_source) || (info->audio_converter))
  {
    gst_element_unlink_many(
      info->audio_stream_source,
      info->audio_converter,
      NULL
    );

    gst_bin_remove_many(
      GST_BIN(info->discourse->audio_mix_pipeline),
      info->audio_stream_source,
      info->audio_converter,
      NULL
    );
  }

  g_free(info);
}

static void
_stream_audio_message(MESSENGER_DiscourseSubscriptionInfo *info,
                      const struct GNUNET_CHAT_Message *message,
                      uint64_t available)
{
  g_assert((info) && (message) && (available));

  if (GNUNET_YES == GNUNET_CHAT_message_is_sent(message))
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

  g_signal_emit_by_name(info->audio_stream_source, "push-buffer", buffer, &ret);
  info->position += samples;

skip_buffer:
  gst_buffer_unref(buffer);

  if (GST_FLOW_OK != ret)
    return;
}

static void
_stream_video_message(MESSENGER_DiscourseSubscriptionInfo *info,
                      const struct GNUNET_CHAT_Message *message,
                      uint64_t available)
{
  g_assert((info) && (message) && (available));

  GstBuffer *buffer = gst_buffer_new_and_alloc(available);
  GstFlowReturn ret = GST_FLOW_OK;

  if (!buffer)
    return;

  GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(info->position, GST_SECOND, 90000);
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(available, GST_SECOND, 90000);

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

  g_signal_emit_by_name(info->video_stream_source, "push-buffer", buffer, &ret);
  info->position += available;

skip_buffer:
  gst_buffer_unref(buffer);

  if (GST_FLOW_OK != ret)
    return;
}

static void
discourse_subscription_stream_message(MESSENGER_DiscourseSubscriptionInfo *info,
                                      const struct GNUNET_CHAT_Message *message)
{
  g_assert((info) && (message));

  const uint64_t available = GNUNET_CHAT_message_available(message);

  if (!available)
    return;

  const struct GNUNET_ShortHashCode *id = GNUNET_CHAT_discourse_get_id(
    info->discourse->discourse
  );

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
    _stream_audio_message(info, message, available);
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    _stream_video_message(info, message, available);
}

static gboolean
discourse_subscription_link_widget(MESSENGER_DiscourseSubscriptionInfo *info,
                                   GtkContainer *container,
                                   gboolean link)
{
  g_assert((info) && (container));

  GtkWidget *widget;
  if (info->video_stream_sink)
    g_object_get(info->video_stream_sink, "widget", &widget, NULL);
  else
    widget = NULL;

  if (!widget)
    return FALSE;

  GtkWidget *parent = gtk_widget_get_parent(widget);
  
  if (parent)
  {
    GtkContainer *container = GTK_CONTAINER(parent);

    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);

    gtk_widget_hide(widget);
    gtk_widget_unrealize(widget);

    gtk_container_remove(
      container,
      widget
    );
  }

  if (!link)
    return TRUE;

  gtk_box_pack_start(
    GTK_BOX(container),
    widget,
    true,
    true,
    0
  );

  g_object_unref(widget);
  gtk_widget_realize(widget);

  gtk_widget_show_all(GTK_WIDGET(container));

  gst_element_set_state(info->video_stream_pipeline, GST_STATE_PLAYING);
  return TRUE;
}

static void
_setup_audio_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

  info->audio_record_pipeline = gst_parse_launch(
    "autoaudiosrc ! audioconvert ! capsfilter name=filter ! fdsink name=sink",
    NULL
  );

  info->audio_record_sink = gst_bin_get_by_name(
    GST_BIN(info->audio_record_pipeline), "sink"
  );

  GstElement *filter = gst_bin_get_by_name(
    GST_BIN(info->audio_record_pipeline), "filter"
  );

  {
    GstBus *bus = gst_element_get_bus(info->audio_record_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "audio/x-raw",
      "format", G_TYPE_STRING, "S16BE",
      "layout", G_TYPE_STRING, "interleaved",
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 1,
      NULL
    );

    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    const int fd = GNUNET_CHAT_discourse_get_fd(info->discourse);
    if (-1 != fd)
      g_object_set(info->audio_record_sink, "fd", fd, NULL);

    gst_element_set_state(info->audio_record_pipeline, GST_STATE_PLAYING);
  }

  info->audio_mix_pipeline = gst_parse_launch(
    "audiomixer name=mixer ! volume name=control ! autoaudiosink",
    NULL
  );

  info->audio_mix_element = gst_bin_get_by_name(
    GST_BIN(info->audio_mix_pipeline), "mixer"
  );

  info->audio_volume_element = gst_bin_get_by_name(
    GST_BIN(info->audio_mix_pipeline), "control"
  );

  {
    GstBus *bus = gst_element_get_bus(info->audio_mix_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    gst_element_set_state(info->audio_mix_pipeline, GST_STATE_PLAYING);
  }
}

static void
_setup_video_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

  info->video_record_pipeline = gst_parse_launch(
    "autovideosrc ! videoconvert ! video/x-raw,format=I420 ! x264enc tune=zerolatency ! capsfilter name=filter ! fdsink name=sink",
    NULL
  );

  info->video_record_sink = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline), "sink"
  );

  GstElement *filter = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline), "filter"
  );

  {
    GstBus *bus = gst_element_get_bus(info->video_record_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "video/x-h264",
      "stream-format", G_TYPE_STRING, "avc",
      "alignment", G_TYPE_STRING, "au",
      NULL
    );

    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    const int fd = GNUNET_CHAT_discourse_get_fd(info->discourse);
    if (-1 != fd)
      g_object_set(info->video_record_sink, "fd", fd, NULL);

    gst_element_set_state(info->video_record_pipeline, GST_STATE_PLAYING);
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

  info->audio_record_pipeline = NULL;
  info->audio_record_sink = NULL;

  info->video_record_pipeline = NULL;
  info->video_record_sink = NULL;

  info->audio_mix_pipeline = NULL;
  info->audio_mix_element = NULL;
  info->audio_volume_element = NULL;

  info->sending_task = 0;
  pthread_mutex_init(&(info->mutex), NULL);

  info->samples = NULL;
  info->subscriptions = NULL;

  const struct GNUNET_ShortHashCode *id = GNUNET_CHAT_discourse_get_id(
    info->discourse
  );

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
     _setup_audio_gst_pipelines(info);
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    _setup_video_gst_pipelines(info);

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

  if (info->video_record_pipeline)
  {
    gst_element_set_state(info->video_record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->video_record_pipeline));
  }

  if (info->audio_mix_pipeline)
  {
    gst_element_set_state(info->audio_mix_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->audio_mix_pipeline));
  }

  if (info->audio_record_pipeline)
  {
    gst_element_set_state(info->audio_record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->audio_record_pipeline));
  }

  pthread_mutex_lock(&(info->mutex));

  if (info->samples)
  {
    GArray *array;
    GList *list = info->samples;
    while (list)
    {
      array = (GArray*) (list->data);
      g_array_free(array, TRUE);
      list = g_list_next(list);
    }

    g_list_free(info->samples);
  }

  if (info->sending_task)
    util_source_remove(info->sending_task);
  
  pthread_mutex_unlock(&(info->mutex));
  pthread_mutex_destroy(&(info->mutex));

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
discourse_has_controls(struct GNUNET_CHAT_Discourse *discourse,
                       MESSENGER_DiscourseControl control)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  switch (control)
  {
    case MESSENGER_DISCOURSE_CTRL_MICROPHONE:
      return (info->audio_record_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_SPEAKERS:
      return (info->audio_mix_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_WEBCAM:
      return (info->video_record_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_SCREEN_CAPTURE:
      return FALSE;
    default:
      return FALSE;
  }
}

void
discourse_set_volume(struct GNUNET_CHAT_Discourse *discourse,
                     double volume)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_mix_pipeline)) || (!(info->audio_volume_element)))
    return;

  g_object_set(info->audio_volume_element, "volume", volume, NULL);
}

double
discourse_get_volume(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_mix_pipeline)) || (!(info->audio_volume_element)))
    return 0.0;

  gdouble volume;
  g_object_get(info->audio_volume_element, "volume", &volume, NULL);

  return volume;
}

void
discourse_set_mute(struct GNUNET_CHAT_Discourse *discourse,
                   bool mute)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_record_pipeline)))
    return;

  gst_element_set_state(
    info->audio_record_pipeline,
    mute? GST_STATE_NULL : GST_STATE_PLAYING
  );
}

bool
discourse_is_mute(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_record_pipeline)))
    return TRUE;

  GstState state;
  gst_element_get_state(
    info->audio_record_pipeline,
    &state,
    NULL,
    GST_CLOCK_TIME_NONE
  );

  return (GST_STATE_PLAYING != state);
}

gboolean
discourse_link_widget(const struct GNUNET_CHAT_Discourse *discourse,
                      const struct GNUNET_CHAT_Contact *contact,
                      GtkContainer *container)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if (contact == sub_info->contact)
      break;

    sub = g_list_next(sub);
  }

  if (!sub_info)
    return FALSE;

  return discourse_subscription_link_widget(sub_info, container, TRUE);
}

gboolean
discourse_unlink_widget(const struct GNUNET_CHAT_Discourse *discourse,
                        const struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if (contact == sub_info->contact)
      break;

    sub = g_list_next(sub);
  }

  if (!sub_info)
    return FALSE;

  return discourse_subscription_link_widget(sub_info, NULL, FALSE);
}
