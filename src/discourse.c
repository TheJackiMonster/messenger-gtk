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

  g_free(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, NULL);
}
