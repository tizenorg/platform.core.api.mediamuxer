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

/*=============================================================================
|                                                                              |
|  INCLUDE FILES                                                               |
|                                                                              |
==============================================================================*/
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

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define MAX_STRING_LEN 100
#define PACKAGE "mediamuxer_test"

mediamuxer_h myMuxer = 0;
media_format_h media_format = NULL;
media_format_h media_format_a = NULL;

bool aud_eos = 0;
bool vid_eos = 0;
char *aud_caps, *vid_caps;
char file_mp4[1000];
bool have_mp4 = false;
int track_index_vid, track_index_aud;

/* demuxer sturcture */
typedef struct _CustomData
{
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


/* demuxer helper functions */
static void _video_app_sink_callback(GstElement *sink, CustomData *data);
static void _video_app_sink_eos_callback(GstElement *sink, CustomData *data);
static void _audio_app_sink_eos_callback(GstElement *sink, CustomData *data);
static void _on_pad_added(GstElement *element, GstPad *pad, CustomData *data);
static gboolean _bus_call(GstBus *bus, GstMessage *msg, gpointer data);

/* Demuxer audio-appsink buffer receive callback*/
static void _audio_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h audfmt;
	media_packet_h aud_pkt;
	track_index_aud = 2;	/* track_index=2 for audio */
	guint8 *dptr;
	static int count = 0;
	if (count == 0)
		g_print("\ngst-1.0 audio\n");
	GstSample *sample;
	uint64_t ns;
	int key;
	GstMapInfo map;
	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);
	if (buffer) {
		/* Print a * to indicate a received buffer */
		g_print("\na%d: ",++count);

		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (!GST_BUFFER_FLAG_IS_SET(buffer,GST_BUFFER_FLAG_DELTA_UNIT)) {
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

			if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AAC)) {
				g_print("media_format_set_audio_mime failed\n");
				return;
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

			g_print(" fixed size %"PRIu64"\n", ns);

			if (media_packet_set_pts(aud_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(aud_pkt, buffer->dts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_duration(aud_pkt, buffer->duration)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_flags(aud_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}

			if (media_packet_set_codec_data(aud_pkt, aud_caps, strlen(aud_caps)+1)) {
				g_print("unable to set the audio codec data e\n");
				return;
			}

			g_print("A write sample call. packet add:%p\n", aud_pkt);
			mediamuxer_write_sample(myMuxer, track_index_aud, aud_pkt);

			media_packet_destroy(aud_pkt);
		}
	}
}

/* Demuxer video-appsink buffer receive callback*/
static void _video_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h vidfmt;
	media_packet_h vid_pkt;
	track_index_vid = 1; /* track_index=1 for video */
	uint64_t ns;
	static int count = 0;
	unsigned int vsize;
	int key;
	guint8 *dptr;
	GstMapInfo map;
	if (count == 0)
		g_print("\ngst-1.0 Video\n");
	GstSample *sample;
	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);

	if (buffer) {
		/* Print a * to indicate a received buffer */
		g_print("v%d: ", ++count);

		g_print("PTS=%"PRIu64"\n", buffer->pts);

		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (media_format_create(&vidfmt)) {
				g_print("media_format_create failed\n");
				return;
			}

			if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H264_SP)) {
				g_print("media_format_set_vidio_mime failed\n");
				return;
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
			/*frame rate is came from the caps filter of demuxer*/
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
			media_packet_get_buffer_size(vid_pkt, &ns);
			g_print("set v buf size as %"PRIu64", data size=%d\n", ns, vsize);
			memcpy((char*)dptr, map.data,map.size);

			if (media_packet_set_buffer_size(vid_pkt, (uint64_t)(map.size))) {
				g_print("video set_buffer_size failed\n");
				return;
			}

			if (media_packet_get_buffer_size(vid_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %"PRIu64"\n",
					(unsigned int)map.size, ns);
				return;
			}

			g_print("fixed size %"PRIu64"\n",ns);

			if (media_packet_set_pts(vid_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(vid_pkt, buffer->dts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_duration(vid_pkt, buffer->duration)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_flags(vid_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}
			if (media_packet_set_codec_data(vid_pkt, vid_caps, strlen(vid_caps)+1)) {
				g_print("unable to set the video codec data e\n");
				return;
			}

			g_print("A write sample call. packet add:%p\n", vid_pkt);
			mediamuxer_write_sample(myMuxer, track_index_vid, vid_pkt);
			media_packet_destroy(vid_pkt);
		}
	}

}

/* demuxer video appsink eos callback */
static void _video_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	mediamuxer_close_track(myMuxer, track_index_vid);
	g_print("\n video h264 eos reached \n");
	vid_eos = 1;
	if (aud_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer audio appsink eos callback */
static void _audio_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	mediamuxer_close_track(myMuxer, track_index_aud);
	g_print("\n audio AAC eos reached \n");
	aud_eos = 1;
	if (vid_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer on_pad callback */
static void _on_pad_added(GstElement *element, GstPad *pad, CustomData *data)
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
	g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(pad), GST_ELEMENT_NAME(element));

	new_pad_caps = gst_pad_get_current_caps(pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	if (g_str_has_prefix(new_pad_type, "audio/mpeg")) {
		new_pad_aud_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_aud_caps);
		g_print("Aud caps:%s\n", caps);
		aud_caps = caps;

		/* Link demuxer to audioqueue */
		ret = gst_pad_link(pad, sink_pad_audioqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print(" Type is but link failed.\n %s", new_pad_type);
		else
			g_print(" Link succeeded  (type '%s').\n", new_pad_type);

		gst_element_link(data->audioqueue, data->audio_appsink);
		g_object_set(data->audio_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->audio_appsink, "new-sample", G_CALLBACK(_audio_app_sink_callback), data);
		g_signal_connect(data->audio_appsink, "eos", G_CALLBACK(_audio_app_sink_eos_callback), data);

		/* Link audioqueue->audio_appsink and save/Give to appsrc of muxer */
		gst_element_set_state(data->audio_appsink, GST_STATE_PLAYING);
		/* one has to set the newly added element to the same state as the rest of the elements. */
	} else if (g_str_has_prefix(new_pad_type, "video/x-h264")) {
		new_pad_vid_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_vid_caps);
		g_print("Video:%s\n",caps);
		vid_caps = caps;

		/* link demuxer with videoqueue */
		ret = gst_pad_link(pad, sink_pad_videoqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print("Type is '%s' but link failed.\n", new_pad_type);
		else
			g_print("Link succeeded (type '%s').\n", new_pad_type);
		gst_element_link(data->videoqueue, data->video_appsink);
		g_object_set(data->video_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->video_appsink, "new-sample", G_CALLBACK(_video_app_sink_callback), data);
		g_signal_connect(data->video_appsink, "eos", G_CALLBACK(_video_app_sink_eos_callback), data);
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
static gboolean _bus_call(GstBus *bus, GstMessage *mesg, gpointer data)
{
	GMainLoop *dmxr_loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE(mesg))
	{
		case GST_MESSAGE_EOS:
			g_print("End of stream\n");
			g_main_loop_quit(dmxr_loop);
			break;

		case GST_MESSAGE_ERROR:
		{
			gchar *dbg;
			GError *err;
			gst_message_parse_error(mesg, &err, &dbg);
			g_free(dbg);
			g_printerr("Demuxer-Error:%s \n", err->message);
			g_error_free(err);
			g_main_loop_quit(dmxr_loop);
			break;
		}
		default:
			break;
	}
	return TRUE;
}

/* Demux an mp4 file and generate encoded streams and extra data  */
int demux_mp4()
{
	CustomData data;
	GMainLoop *loop_dmx;
	GstBus *bus;
	guint watch_id_for_bus;

	if (access(file_mp4, F_OK) == -1) {
		/* mp4 file doesn't exist */
		g_print("mp4 Invalid file path.");
		return -1;
	}

	gst_init(NULL,NULL);
	loop_dmx = g_main_loop_new(NULL, FALSE);
	data.loop = loop_dmx;

	/* Create gstreamer elements for demuxer*/
	data.pipeline = gst_pipeline_new("DemuxerPipeline");
	data.source = gst_element_factory_make("filesrc", "file-source");
	data.demuxer = gst_element_factory_make("qtdemux", "mp4-demuxer");
	data.audioqueue = gst_element_factory_make("queue", "audio-queue");
	data.videoqueue = gst_element_factory_make("queue", "video-queue");

	data.dummysink = gst_element_factory_make("fakesink", "fakesink");
	data.video_appsink = gst_element_factory_make("appsink", "video (h264) appsink");
	data.audio_appsink = gst_element_factory_make("appsink", "audio (AAC) appsink");

	if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue
		|| !data.dummysink || !data.videoqueue || !data.audio_appsink || !data.video_appsink) {
		g_print("Test-Suite: One gst-element can't be created. Exiting\n");
		return -1;
	}

	/* Add msg-handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
	watch_id_for_bus = gst_bus_add_watch(bus, _bus_call, loop_dmx);
	gst_object_unref(bus);

	/* Add gstreamer-elements into gst-pipeline */
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.demuxer, data.dummysink, \
		      data.audioqueue, data.videoqueue,data.audio_appsink, data.video_appsink, NULL);

	/* we set the input filename to the source element */
	g_object_set(G_OBJECT(data.source), "location", file_mp4, NULL);

	/* we link the elements together */
	gst_element_link(data.source, data.demuxer);

	/* Register demuxer callback */
	g_signal_connect(data.demuxer, "pad-added", G_CALLBACK(_on_pad_added), &data);

	/* play the pipeline */
	g_print("Now playing: %s\n", file_mp4);
	gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

	/* Run the loop till quit */
	g_print("gst-pipeline-Running...\n");
	g_main_loop_run(loop_dmx);

	/* Done with gst-loop. Free resources */
	gst_element_set_state(data.pipeline, GST_STATE_NULL);

	g_print("Unreferencing the gst-pipeline\n");
	gst_object_unref(GST_OBJECT(data.pipeline));
	g_source_remove(watch_id_for_bus);
	g_main_loop_unref(loop_dmx);
	return 0;
}


int test_mediamuxer_create()
{
	g_print("test_mediamuxer_create\n");
	g_print("%p", myMuxer);

	if (mediamuxer_create(&myMuxer) != MEDIAMUXER_ERROR_NONE) {
		g_print("mediamuxer create is failed\n");
	}

	g_print("\n Muxer->mx_handle created successfully with address=%p",
	        (void *)((mediamuxer_s *)myMuxer)->mx_handle);
	g_print("\n Muxer handle created successfully with address=%p",
	        myMuxer);

	return 0;
}

int test_mediamuxer_set_data_sink()
{
	char *op_uri = "MyTest.mp4";

	/* Set data source after creating */
	mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_MP4);
	return 0;
}

int test_mediamuxer_destroy()
{
	int ret = 0;
	g_print("test_mediamuxer_destroy\n");
	ret = mediamuxer_destroy(myMuxer);
	myMuxer = NULL;
	g_print("\nDestroy operation returned: %d\n", ret);
	return ret;
}

int test_mediamuxer_prepare()
{
	g_print("mediamuxer_prepare completed \n");
	mediamuxer_prepare(myMuxer);
	return 0;
}

int test_mediamuxer_start()
{
	g_print("mediamuxer_start completed \n");
	mediamuxer_start(myMuxer);
	return 0;
}

int test_mediamuxer_add_track_video()
{
	int track_index_vid = -1;
	media_format_mimetype_e mimetype;
	int width;
	int height;
	int avg_bps;
	int max_bps;

	g_print("test_mediamuxer_add_track_video\n");
	media_format_create(&media_format);

	/* MEDIA_FORMAT_H264_SP  MEDIA_FORMAT_H264_MP  MEDIA_FORMAT_H264_HP */
	media_format_set_video_mime(media_format, MEDIA_FORMAT_H264_SP);

	media_format_set_video_width(media_format, 640);
	media_format_set_video_height(media_format, 480);
	media_format_set_video_avg_bps(media_format, 256000);
	media_format_set_video_max_bps(media_format, 256000);

	media_format_get_video_info(media_format, &mimetype, &width, &height, &avg_bps, &max_bps);

	g_print("\n Video Mime trying to set: %x   %x\n",(int)(mimetype),(int)(MEDIA_FORMAT_H264_SP));
	g_print("\n Video param trying to set: (width, height, avg_bps, max_bps): %d %d %d %d  \n",
	        width, height, avg_bps, max_bps);

	/* To add video track */
	mediamuxer_add_track(myMuxer, media_format, &track_index_vid);
	g_print("video track index returned is: %d", track_index_vid);
	return 0;
}

int test_mediamuxer_add_track_audio()
{
	int track_index_aud = -1;
	media_format_mimetype_e mimetype;
	int channel;
	int samplerate;
	int bit;
	int avg_bps;

	g_print("test_mediamuxer_add_track\n");
	media_format_create(&media_format_a);

	/* MEDIA_FORMAT_AAC  MEDIA_FORMAT_AAC_LC  MEDIA_FORMAT_AAC_HE  MEDIA_FORMAT_AAC_HE_PS */
	if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AAC) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
		g_print("Problem during media_format_set_audio_mime operation");

	if (media_format_set_audio_channel(media_format_a, 2) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
		g_print("Problem during media_format_set_audio_channel operation");
	media_format_set_audio_samplerate(media_format_a, 44100);
	media_format_set_audio_bit(media_format_a, 32);
	media_format_set_audio_avg_bps(media_format_a, 128000);
	media_format_set_audio_aac_type(media_format_a, true);

	media_format_get_audio_info(media_format_a, &mimetype, &channel, &samplerate, &bit, &avg_bps);

	g_print("\n Audio Mime trying to set: %x   %x\n", (int)(mimetype), (int)(MEDIA_FORMAT_AAC));
	g_print("\n Audio Param trying to set: (ch, samplert, bt, avg_bps) %d %d %d %d \n",
	        channel, samplerate, bit, avg_bps);

	/* To add audio track */
	mediamuxer_add_track(myMuxer, media_format_a, &track_index_aud);
	g_print("track index returned is: %d", track_index_aud);
	return 0;
}

void app_err_cb(mediamuxer_error_e error, void *user_data)
{
	printf("Got Error %d from mediamuxer\n",error);
}

int test_mediamuxer_set_error_cb()
{
	int ret = 0;
	g_print("test_mediamuxer_set_error_cb\n");
	ret = mediamuxer_set_error_cb(myMuxer, app_err_cb, myMuxer);
	return ret;
}


int test_mediamuxer_write_sample()
{
	demux_mp4();
	return 0;
}

int test_mediamuxer_stop()
{
	g_print("test_mediamuxer_stop\n");
	mediamuxer_stop(myMuxer);
	return 0;
}


int test_mediamuxer_unprepare()
{
	g_print("test_mediamuxer_unprepare\n");
	mediamuxer_unprepare(myMuxer);
	media_format_unref(media_format_a);
	media_format_unref(media_format);
	return 0;
}

int test_mediamuxer_pause()
{
	g_print("test_mediamuxer_pause\n");
	mediamuxer_state_e state;
	if (mediamuxer_get_state(myMuxer, &state) == MEDIAMUXER_ERROR_NONE) {
		g_print("\nMediamuxer_state=%d",state);
		if (state == MEDIAMUXER_STATE_MUXING)
			mediamuxer_pause(myMuxer);
	}
	return 0;
}

int test_mediamuxer_resume()
{
	g_print("test_mediamuxer_resume\n");
	mediamuxer_resume(myMuxer);
	return 0;
}

int test_mediamuxer_add_track()
{
	g_print("test_mediamuxer_add_track\n");
	return 0;
}

void quit_testApp(void)
{
	/* To Do: Replace exit(0) with smooth exit */
	exit(0);
}

enum {
	CURRENT_STATUS_MAINMENU,
	CURRENT_STATUS_MP4_FILENAME
};

int g_menu_state = CURRENT_STATUS_MAINMENU;
static void display_sub_basic();

void reset_menu_state()
{
	g_menu_state = CURRENT_STATUS_MAINMENU;
	return;
}

static void input_filepath(char *filename)
{
	g_print("Opening file %s\n", filename);
	return;
}

void _interpret_main_menu(char *cmd)
{
	int len = strlen(cmd);
	if (len == 1) {
		if (strncmp(cmd, "c", 1) == 0) {
			test_mediamuxer_create();
		} else if (strncmp(cmd, "o", 1) == 0) {
			test_mediamuxer_set_data_sink();
		} else if (strncmp(cmd, "d", 1) == 0) {
			test_mediamuxer_destroy();
		} else if (strncmp(cmd, "e", 1) == 0) {
			test_mediamuxer_prepare();
		} else if (strncmp(cmd, "s", 1) == 0) {
			test_mediamuxer_start();
		} else if (strncmp(cmd, "a", 1) == 0) {
			if (have_mp4 == false) {
				g_menu_state = CURRENT_STATUS_MP4_FILENAME;
				have_mp4 = true;
			}
			test_mediamuxer_add_track_audio();
		} else if (strncmp(cmd, "v", 1) == 0) {
			if (have_mp4 == false) {
				g_menu_state = CURRENT_STATUS_MP4_FILENAME;
				have_mp4 = true;
			}
			test_mediamuxer_add_track_video();
		} else if (strncmp(cmd, "m", 1) == 0) {
			test_mediamuxer_write_sample();
		} else if (strncmp(cmd, "t", 1) == 0) {
			test_mediamuxer_stop();
		} else if (strncmp(cmd, "u", 1) == 0) {
			test_mediamuxer_unprepare();
		} else if (strncmp(cmd, "p", 1) == 0) {
			test_mediamuxer_pause();
		} else if (strncmp(cmd, "r", 1) == 0) {
			test_mediamuxer_resume();
		} else if (strncmp(cmd, "b", 1) == 0) {
			test_mediamuxer_set_error_cb();
		} else if (strncmp(cmd, "q", 1) == 0) {
			quit_testApp();
		} else {
			g_print("unknown menu command. Please try again\n");
		}
	} else {
		g_print("unknown menu command. Please try again\n");
	}

	return;
}

static void displaymenu(void)
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU) {
		display_sub_basic();
	} else if (g_menu_state == CURRENT_STATUS_MP4_FILENAME) {
		g_print("*** input mp4 file path:\n");
		g_print("[This is the file from where demuxed data is fed to muxer]:");
	} else {
		g_print("*** unknown status.\n");
		exit(0);
	}
	g_print(" >>> ");
}

gboolean timeout_menu_display(void *data)
{
	displaymenu();
	return FALSE;
}

static void interpret(char *cmd)
{

	switch (g_menu_state) {
		case CURRENT_STATUS_MAINMENU: {
				_interpret_main_menu(cmd);
				break;
			}
		case CURRENT_STATUS_MP4_FILENAME: {
				input_filepath(cmd);
				strcpy(file_mp4, cmd);
				g_menu_state = CURRENT_STATUS_MAINMENU;
				break;
			}
		default:
			break;
	}
	g_timeout_add(100, timeout_menu_display, 0);
}

static void display_sub_basic()
{
	g_print("\n");
	g_print("==========================================================\n");
	g_print("                    media muxer test\n");
	g_print("----------------------------------------------------------\n");
	g_print("c. Create \t");
	g_print("o. Set Data Sink \t");
	g_print("a. AddAudioTrack \t");
	g_print("v. AddVideoTrack \t");
	g_print("e. prepare \t");
	g_print("s. start \t");
	g_print("m. startMuxing \t");
	g_print("p. PauseMuxing \t");
	g_print("r. ResumeMuxing \t");
	g_print("b. set error callback \t");
	g_print("t. stop \t");
	g_print("u. UnPrepare \t");
	g_print("d. destroy \t");
	g_print("q. quit \t");
	g_print("\n");
	g_print("==========================================================\n");
}

/**
 * This function is to execute command.
 *
 * @param	channel [in]    1st parameter
 *
 * @return	This function returns TRUE/FALSE
 * @remark
 * @see
 */
gboolean input(GIOChannel *channel)
{
	gchar buf[MAX_STRING_LEN];
	gsize read;
	GError *error = NULL;
	g_io_channel_read_chars(channel, buf, MAX_STRING_LEN, &read, &error);
	buf[read] = '\0';
	g_strstrip(buf);
	interpret(buf);
	return TRUE;
}

/**
 * This function is the example main function for mediamuxer API.
 *
 * @param
 *
 * @return      This function returns 0.
 * @remark
 * @see         other functions
 */
int main(int argc, char *argv[])
{
	GIOChannel *stdin_channel;
	GMainLoop *loop = g_main_loop_new(NULL, 0);
	stdin_channel = g_io_channel_unix_new(0);
	g_io_channel_set_flags(stdin_channel, G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);

	displaymenu();
	/* g_print("RUN main loop\n"); */
	g_main_loop_run(loop);
	g_print("STOP main loop\n");

	g_main_loop_unref(loop);
	return 0;
}
