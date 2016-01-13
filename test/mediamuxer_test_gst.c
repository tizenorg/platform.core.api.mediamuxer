/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ============================================================================
|                                                                              |
|  INCLUDE FILES                                                               |
|                                                                              |
============================================================================= */
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <inttypes.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mediamuxer_port.h"
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <media_packet_internal.h>
#include <media_codec.h>

/* ---------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
---------------------------------------------------------------------- */
char *aud_caps, *vid_caps;
bool aud_eos = 0;
bool vid_eos = 0;
extern int track_index_vid, track_index_aud, track_index_aud2;
extern mediamuxer_h myMuxer;
extern bool validate_multitrack;
extern char media_file[2048];
extern char data_sink[2048];
extern char media_file[2048];
extern bool have_aud_track;
extern bool have_vid_track;
const gchar *new_pad_type_aud = NULL; /* demuxer pad type for audio */
const gchar *new_pad_type_vid = NULL; /* demuxer pad type for video */
/* demuxer sturcture for demux_mp4() */
typedef struct _CustomData {
	GstElement *pipeline;
	GstElement *source;
	GstElement *demuxer;
	GstElement *audioqueue;
	GstElement *videoqueue;

	GstElement *audio_appsink;	/* o/p of demuxer */
	GstElement *video_appsink;
	GstElement *dummysink;

	char *saveLocation_audio;	/* aac stream */
	char *saveLocation_video;	/* h264 stream */
	GMainLoop *loop;

	GTimer *timer;
} CustomData;

/* ----------------------------------------------------------------------
|    HELPER  FUNCTION                                                   |
---------------------------------------------------------------------- */
/* Demuxer audio-appsink buffer receive callback */
static void __audio_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h audfmt;
	media_packet_h aud_pkt;
	guint8 *dptr;
	static int count = 0;
	GstSample *sample;
	uint64_t ns;
	int key;
	GstMapInfo map;

	if (count == 0)
		g_print("Called __audio_app_sink_callback\n");

	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);
	if (buffer) {
		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
				/* g_print( "  Key Frame  \n"); */
				key = 1;
			} else {
				/* g_print( "  NOT a Key Frame  \n"); */
				key = 0;
			}

			if (media_format_create(&audfmt)) {
				g_print("media_format_create failed\n");
				return;
			}

			g_print("audio data_sink = %s\n",data_sink);
			/* check if the mime selected during set_data_sink is matching with the mime of the file inputted.*/
			if (g_str_has_prefix(new_pad_type_aud, "audio/mpeg")
				&& (strncmp(data_sink, "11", 2) == 0 || strncmp(data_sink, "12", 2) == 0 || strncmp(data_sink, "13", 2) == 0
					|| strncmp(data_sink, "21", 2) == 0 || strncmp(data_sink, "22", 2) == 0)) {
				if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AAC_LC)) {
					g_print("media_format_set_audio_mime failed\n");
					return;
				}
			} else if (g_str_has_prefix(new_pad_type_aud, "audio/AMR-WB")
				&& strncmp(data_sink, "42", 2) == 0) {
				g_print("For amr-wb, setting encoded media type as MEDIA_FORMAT_AMR_WB\n");
				if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AMR_WB)) {
					g_print("media_format_set_audio_mime failed\n");
					return;
				}
			} else if (g_str_has_prefix(new_pad_type_aud, "audio/AMR")
				&& (strncmp(data_sink, "23", 2) == 0 || strncmp(data_sink, "41", 2) == 0)) {
				g_print("For amr-nb, setting encoded media type as MEDIA_FORMAT_AMR_NB\n");
				if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AMR_NB)) {
					g_print("media_format_set_audio_mime failed\n");
					return;
				}
			} else if (g_str_has_prefix(new_pad_type_aud, "audio/x-wav")
				&& strncmp(data_sink, "31", 2) == 0) {
				g_print("creating audio-wav\n");
				if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_PCM)) {
					g_print("media_format_set_audio_mime failed\n");
					return;
				}
				if (media_format_set_audio_bit(audfmt, 16))
					g_print("wav media_format_set_audio_bit failed");
				if (media_format_set_audio_channel(audfmt, 2))
					g_print("wav media_format_set_audio_channel failed");
				if (media_format_set_audio_samplerate(audfmt, 44100))
					g_print("wav media_format_set_audio_samplerate failed");
			}

			if (media_packet_create(audfmt, NULL, NULL, &aud_pkt)) {
				g_print("create audio media_packet failed\n");
				return;
			}

			if (media_packet_alloc(aud_pkt)) {
				g_print("audio media_packet alloc failed\n");
				return;
			}

			media_packet_get_buffer_data_ptr(aud_pkt, (void **)&dptr);
			memcpy((char*)dptr, map.data, map.size);

			if (media_packet_set_buffer_size(aud_pkt, (uint64_t)(map.size))) {
				g_print("audio set_buffer_size failed\n");
				return;
			}

			if (media_packet_get_buffer_size(aud_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %"PRIu64"\n",
					(unsigned int)map.size, ns);
				return;
			}

			if (strncmp(data_sink, "31", 2) == 0 || strncmp(data_sink, "wav", 3) == 0) {
				aud_caps = "audio/x-raw, format=(string)S16LE, layout=(string)interleaved, channels=(int)2, channel-mask=(bitmask)0x0000000000000003, rate=(int)44100";
				/* no need to set the rest of the parameters for wav */
				goto SET_CAPS;
			} else if (strncmp(data_sink, "41", 2) == 0 || strncmp(data_sink, "amr-nb", 6) == 0) {
				/* ToDo: Query caps from amrparse src pad */
				aud_caps = "audio/AMR, channels=1, rate=8000";
				goto SET_CAPS;
			} else if (strncmp(data_sink, "42", 2) == 0 || strncmp(data_sink, "amr-wb", 6) == 0) {
				/* ToDo: Query caps from amrparse src pad */
				aud_caps = "audio/AMR-WB, channels=1, rate=16000";
				goto SET_CAPS;
			}

			if (media_packet_set_pts(aud_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(aud_pkt, buffer->dts)) {
				g_print("unable to set the dts\n");
				return;
			}

			if (media_packet_set_duration(aud_pkt, buffer->duration)) {
				g_print("unable to set the duration\n");
				return;
			}

			if (media_packet_set_flags(aud_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}

SET_CAPS:
			if (media_packet_set_extra(aud_pkt, aud_caps)) {
				g_print("unable to set the audio codec data \n");
				return;
			}

			/* Print count and size to indicate a received buffer */
			g_print("Received audio buffer count : %4d (size : %5"PRIu64", pts : %12"PRIu64")\n",
					++count, ns, buffer->pts);
			mediamuxer_write_sample(myMuxer, track_index_aud, aud_pkt);
			if (validate_multitrack)
				mediamuxer_write_sample(myMuxer, track_index_aud2, aud_pkt);

			media_packet_destroy(aud_pkt);
		}
	}
}

/* Demuxer video-appsink buffer receive callback */
static void __video_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h vidfmt;
	media_packet_h vid_pkt;
	uint64_t ns;
	static int count = 0;
	unsigned int vsize;
	int key;
	guint8 *dptr;
	GstMapInfo map;
	GstSample *sample;

	if (count == 0)
		g_print("Called __video_app_sink_callback\n");

	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);

	if (buffer) {
		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (media_format_create(&vidfmt)) {
				g_print("media_format_create failed\n");
				return;
			}
			g_print("video data_sink = %s\n",data_sink);

			/* check if the mime selected during set_data_sink is matching with the mime of the file inputted.*/
			if (g_str_has_prefix(new_pad_type_vid, "video/x-h264")
				&& (strncmp(data_sink, "11", 2) == 0  || strncmp(data_sink, "21", 2) == 0 )) {
				if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H264_SP)) {
					g_print("media_format_set_video_mime to H264_SP failed\n");
					return;
				}
			} else if (g_str_has_prefix(new_pad_type_vid, "video/mpeg")
				&& strncmp(data_sink, "13", 2) == 0  ) {
				g_print("For mpeg4, setting encoded media type as MEDIA_FORMAT_MPEG4_SP\n");
				if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_MPEG4_SP)) {
					g_print("media_format_set_video_mime to MPEG4_SP failed\n");
					return;
				}
			} else if (g_str_has_prefix(new_pad_type_vid, "video/x-h263")
				&& (strncmp(data_sink, "12", 2) == 0 || strncmp(data_sink, "22", 2) == 0 || strncmp(data_sink, "23", 2) == 0)) {
				g_print("For h263, setting encoded media type as MEDIA_FORMAT_H263\n");
				if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H263)) {
					g_print("media_format_set_vidio_mime failed\n");
					return;
				}
			} else {
				g_print("Unsupported encoded video mime. Currently muxer supports:h263, h264 & mpeg4\n");
			}

			if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
				/* g_print("Key Frame\n"); */
				key = 1;
			} else {
				/* g_print("NOT a Key Frame\n"); */
				key = 0;
			}

			vsize = map.size;
			media_format_set_video_width(vidfmt, vsize/2+1);
			media_format_set_video_height(vidfmt, vsize/2+1);
			/* frame rate is came from the caps filter of demuxer */
			if (media_format_set_video_frame_rate(vidfmt, 30)) {
				g_print("media_format_set_video_frame_rate failed\n");
				return;
			}

			if (media_packet_create(vidfmt, NULL, NULL, &vid_pkt)) {
				g_print("create video media_packet failed\n");
				return;
			}

			if (media_packet_alloc(vid_pkt)) {
				g_print("video media_packet alloc failed\n");
				return;
			}

			media_packet_get_buffer_data_ptr(vid_pkt, (void**)&dptr);
			memcpy((char*)dptr, map.data, map.size);

			if (media_packet_set_buffer_size(vid_pkt, (uint64_t)(map.size))) {
				g_print("video set_buffer_size failed\n");
				return;
			}

			if (media_packet_get_buffer_size(vid_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %"PRIu64"\n",
					(unsigned int)map.size, ns);
				return;
			}

			if (media_packet_set_pts(vid_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(vid_pkt, buffer->dts)) {
				g_print("unable to set the dts\n");
				return;
			}

			if (media_packet_set_duration(vid_pkt, buffer->duration)) {
				g_print("unable to set the duration\n");
				return;
			}

			if (media_packet_set_flags(vid_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}
			if (media_packet_set_extra(vid_pkt, vid_caps)) {
				g_print("unable to set the video codec data e\n");
				return;
			}

			/* Print count and size to indicate a received buffer */
			g_print("Received video buffer count : %4d (size : %5"PRIu64", pts : %12"PRIu64")\n",
					++count, ns, buffer->pts);
			mediamuxer_write_sample(myMuxer, track_index_vid, vid_pkt);
			media_packet_destroy(vid_pkt);
		}
	}

}

/* demuxer audio appsink eos callback */
void __audio_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	g_print("__audio_app_sink_eos_callback, closing track_index = %d\n", track_index_aud);
	mediamuxer_close_track(myMuxer, track_index_aud);
	if (validate_multitrack)
		mediamuxer_close_track(myMuxer, track_index_aud2);
	g_print("audio EOS cb reached \n");
	aud_eos = 1;
	if (!have_vid_track || vid_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer video appsink eos callback */
static void __video_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	mediamuxer_close_track(myMuxer, track_index_vid);
	g_print("Encoded video EOS cb reached \n");
	vid_eos = 1;
	if (!have_aud_track || aud_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer on_pad callback */
static void __on_pad_added(GstElement *element, GstPad *pad, CustomData *data)
{
	GstPadLinkReturn ret;
	GstPad *sink_pad_audioqueue = gst_element_get_static_pad(data->audioqueue, "sink");
	GstPad *sink_pad_videoqueue = gst_element_get_static_pad(data->videoqueue, "sink");
	GstCaps *new_pad_aud_caps = NULL;
	GstCaps *new_pad_vid_caps = NULL;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	char *caps;
	g_print("Received new pad '%s' from '%s' :\n", GST_PAD_NAME(pad), GST_ELEMENT_NAME(element));

	new_pad_caps = gst_pad_get_current_caps(pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	if (have_aud_track && (g_str_has_prefix(new_pad_type, "audio/mpeg")
		|| g_str_has_prefix(new_pad_type, "audio/AMR"))) {
		new_pad_aud_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_aud_caps);
		g_print("   Audio caps :%s\n", caps);
		aud_caps = caps;

		/* Link demuxer to audioqueue */
		ret = gst_pad_link(pad, sink_pad_audioqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print("   Type is but link failed.\n %s", new_pad_type);
		else
			g_print("   Link succeeded (type '%s').\n", new_pad_type);
		new_pad_type_aud = new_pad_type;
		gst_element_link(data->audioqueue, data->audio_appsink);
		g_object_set(data->audio_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->audio_appsink, "new-sample", G_CALLBACK(__audio_app_sink_callback), data);
		g_signal_connect(data->audio_appsink, "eos", G_CALLBACK(__audio_app_sink_eos_callback), data);
		/* Link audioqueue->audio_appsink and save/Give to appsrc of muxer */
		gst_element_set_state(data->audio_appsink, GST_STATE_PLAYING);
		/* one has to set the newly added element to the same state as the rest of the elements. */
	} else if (have_vid_track && (g_str_has_prefix(new_pad_type, "video/x-h264")
		|| g_str_has_prefix(new_pad_type, "video/x-h263")
		|| g_str_has_prefix(new_pad_type, "video/mpeg"))) {
		new_pad_vid_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_vid_caps);
		g_print("   Video caps :%s\n", caps);
		vid_caps = caps;

		/* Link demuxer with videoqueue */
		ret = gst_pad_link(pad, sink_pad_videoqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print("   Type is '%s' but link failed.\n", new_pad_type);
		else
			g_print("   Link succeeded (type '%s').\n", new_pad_type);
		new_pad_type_vid = new_pad_type;
		gst_element_link(data->videoqueue, data->video_appsink);
		g_object_set(data->video_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->video_appsink, "new-sample", G_CALLBACK(__video_app_sink_callback), data);
		g_signal_connect(data->video_appsink, "eos", G_CALLBACK(__video_app_sink_eos_callback), data);
		/* Link videoqueue->audio_appsink and save/Give to appsrc of muxer */
		gst_element_set_state(data->video_appsink, GST_STATE_PLAYING);
		/* one has to set the newly added element to the same state as the rest of the elements. */
	} else {
		g_print(" It has type '%s' which is not raw A/V. Ignoring.\n", new_pad_type);
		goto exit;
	}

exit:
	if (new_pad_caps != NULL)
		gst_caps_unref(new_pad_caps);

	gst_object_unref(sink_pad_audioqueue);
	gst_object_unref(sink_pad_videoqueue);
}

/* Demuxer bus_call */
static gboolean __bus_call(GstBus *bus, GstMessage *mesg, gpointer data)
{
	GMainLoop *dmxr_loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE(mesg)) {
	case GST_MESSAGE_EOS:
		g_print("Demuxer: End of stream\n");
		g_main_loop_quit(dmxr_loop);
		break;

	case GST_MESSAGE_ERROR:
	{
		gchar *dbg;
		GError *err;
		gst_message_parse_error(mesg, &err, &dbg);
		g_free(dbg);
		g_printerr("Demuxer-Error: %s\n", err->message);
		g_error_free(err);
		g_main_loop_quit(dmxr_loop);
		break;
	}
	default:
		break;
	}
	return TRUE;
}


/* Demux audio (wav/amr) file and generate raw data */
int demux_audio()
{
	CustomData data;
	GMainLoop *loop_dmx;
	GstBus *bus;
	guint watch_id_for_bus;

	g_print("Start of _demux_audio()\n");

	if (access(media_file, F_OK) == -1) {
		/* wav/amr file doesn't exist */
		g_print("wav/amr Invalid file path.");
		return -1;
	}

	gst_init(NULL, NULL);
	loop_dmx = g_main_loop_new(NULL, FALSE);
	data.loop = loop_dmx;
	new_pad_type_aud = "audio/x-wav"; /* Update to aid cb. */
	/* Create gstreamer elements for demuxer */
	data.pipeline = gst_pipeline_new("DemuxerPipeline");
	data.source = gst_element_factory_make("filesrc", "file-source");
	if (strncmp(data_sink, "31", 2) == 0 || strncmp(data_sink, "wav", 3) == 0) {
		data.demuxer = gst_element_factory_make("wavparse", "wavparse");
		new_pad_type_aud = "audio/x-wav";	/* Update to aid cb */
	} else if (strncmp(data_sink, "41", 2) == 0) {
		data.demuxer = gst_element_factory_make("amrparse", "amrparse");
		new_pad_type_aud = "audio/AMR";	/* Update to aid cb */
	} else if (strncmp(data_sink, "42", 2) == 0 || strncmp(data_sink, "amr-wb", 6) == 0) {
		data.demuxer = gst_element_factory_make("amrparse", "amrparse");
		new_pad_type_aud = "audio/AMR-WB";	/* Update to aid cb */
	}
	data.audioqueue = gst_element_factory_make("queue", "audio-queue");
	data.audio_appsink = gst_element_factory_make("appsink", "encoded_audio_appsink");

	if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue || !data.audio_appsink) {
		g_print("Test-Suite: One gst-element can't be created. Exiting\n");
		return -1;
	}

	/* Add msg-handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
	watch_id_for_bus = gst_bus_add_watch(bus, __bus_call, loop_dmx);
	gst_object_unref(bus);

	/* Add gstreamer-elements into gst-pipeline */
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.demuxer, data.audioqueue, data.audio_appsink, NULL);

	/* we set the input filename to the source element */
	g_object_set(G_OBJECT(data.source), "location", media_file, NULL);

	/* we link the elements together */
	if (!gst_element_link_many(data.source, data.demuxer, data.audioqueue, data.audio_appsink, NULL)) {
		g_print("Demuxer pipeline link failed");
		return -1;
	}

	g_object_set(data.audio_appsink, "emit-signals", TRUE, NULL);
	g_signal_connect(data.audio_appsink, "new-sample", G_CALLBACK(__audio_app_sink_callback), &data);
	g_signal_connect(data.audio_appsink, "eos", G_CALLBACK(__audio_app_sink_eos_callback), &data);

	/* No demuxer callback for wav, playing the pipeline */
	g_print("Now playing: %s\n", media_file);
	gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

	/* Run the loop till quit */
	g_print("gst-wav/amr-pipeline -Running...\n");
	g_main_loop_run(loop_dmx);

	/* Done with gst-loop. Free resources */
	gst_element_set_state(data.pipeline, GST_STATE_NULL);

	g_print("gst-wav-pipeline - Unreferencing...\n");
	gst_object_unref(GST_OBJECT(data.pipeline));
	g_source_remove(watch_id_for_bus);
	g_main_loop_unref(loop_dmx);
	g_print("End of demux_audio()\n");
	return 0;
}

/* Demux an mp4 file and generate encoded streams and extra data  */
int demux_mp4()
{
	CustomData data;
	GMainLoop *loop_dmx;
	GstBus *bus;
	guint watch_id_for_bus;

	g_print("Start of _demux_mp4()\n");

	if (access(media_file, F_OK) == -1) {
		/* mp4 file doesn't exist */
		g_print("mp4 Invalid file path.");
		return -1;
	}

	gst_init(NULL, NULL);
	loop_dmx = g_main_loop_new(NULL, FALSE);
	data.loop = loop_dmx;

	/* Create gstreamer elements for demuxer */
	data.pipeline = gst_pipeline_new("DemuxerPipeline");
	data.source = gst_element_factory_make("filesrc", "file-source");
	data.demuxer = gst_element_factory_make("qtdemux", "mp4-demuxer");
	data.audioqueue = gst_element_factory_make("queue", "audio-queue");
	data.videoqueue = gst_element_factory_make("queue", "video-queue");

	data.dummysink = gst_element_factory_make("fakesink", "fakesink");
	data.video_appsink = gst_element_factory_make("appsink", "encoded_video_appsink");
	data.audio_appsink = gst_element_factory_make("appsink", "encoded_audio_appsink");

	if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue
		|| !data.dummysink || !data.videoqueue || !data.audio_appsink || !data.video_appsink) {
		g_print("Test-Suite: One gst-element can't be created. Exiting\n");
		return -1;
	}

	/* Add msg-handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
	watch_id_for_bus = gst_bus_add_watch(bus, __bus_call, loop_dmx);
	gst_object_unref(bus);

	/* Add gstreamer-elements into gst-pipeline */
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.demuxer, data.dummysink, \
		data.audioqueue, data.videoqueue, data.audio_appsink, data.video_appsink, NULL);

	/* we set the input filename to the source element */
	g_object_set(G_OBJECT(data.source), "location", media_file, NULL);

	/* we link the elements together */
	gst_element_link(data.source, data.demuxer);

	/* Register demuxer callback */
	g_signal_connect(data.demuxer, "pad-added", G_CALLBACK(__on_pad_added), &data);

	/* Play the pipeline */
	g_print("Now playing : %s\n", media_file);
	gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

	/* Run the loop till quit */
	g_print("gst-pipeline -Running...\n");
	g_main_loop_run(loop_dmx);

	/* Done with gst-loop. Free resources */
	gst_element_set_state(data.pipeline, GST_STATE_NULL);

	g_print("gst-pipeline - Unreferencing...\n");
	gst_object_unref(GST_OBJECT(data.pipeline));
	g_source_remove(watch_id_for_bus);
	g_main_loop_unref(loop_dmx);
	g_print("End of _demux_mp4()\n");
	return 0;
}

