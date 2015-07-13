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

#include <mm_debug.h>
#include <mediamuxer_error.h>
#include <mediamuxer_private.h>
#include <mediamuxer_port.h>
#include <mediamuxer_port_gst.h>

#define EOS_POLL_PERIOD 1000
#define WRITE_POLL_PERIOD 100
/*#define SEND_FULL_CAPS_VIA_CODEC_DATA *//*For debug purpose*/
#define ASYCHRONOUS_WRITE  /*write sample is not blocking */

static int gst_muxer_init(MMHandleType *pHandle);
static int gst_muxer_set_data_sink(MMHandleType pHandle, char *uri,
			mediamuxer_output_format_e format);
static int gst_muxer_add_track(MMHandleType pHandle,
			media_format_h media_format, int *track_index);
static int gst_muxer_start(MMHandleType pHandle);
static int gst_muxer_write_sample(MMHandleType pHandle, int track_index,
			media_packet_h inbuf);
static int gst_muxer_close_track(MMHandleType pHandle, int track_index);
static int gst_muxer_pause(MMHandleType pHandle);
static int gst_muxer_resume(MMHandleType pHandle);
static int gst_muxer_stop(MMHandleType pHandle);
static int gst_muxer_destroy(MMHandleType pHandle);
static int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void* user_data);

/*Media Muxer API common*/
static media_port_muxer_ops def_mux_ops = {
	.n_size = 0,
	.init = gst_muxer_init,
	.set_data_sink = gst_muxer_set_data_sink,
	.add_track = gst_muxer_add_track,
	.start = gst_muxer_start,
	.write_sample = gst_muxer_write_sample,
	.close_track = gst_muxer_close_track,
	.pause = gst_muxer_pause,
	.resume = gst_muxer_resume,
	.stop = gst_muxer_stop,
	.destroy = gst_muxer_destroy,
	.set_error_cb = gst_set_error_cb,
};

int gst_port_register(media_port_muxer_ops *pOps)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(pOps);

	def_mux_ops.n_size = sizeof(def_mux_ops);

	memcpy((char *)pOps + sizeof(pOps->n_size),
	       (char *)&def_mux_ops + sizeof(def_mux_ops.n_size),
	       pOps->n_size - sizeof(pOps->n_size));

	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_init(MMHandleType *pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	mxgst_handle_t *new_mediamuxer = NULL;

	new_mediamuxer = (mxgst_handle_t *) g_malloc(sizeof(mxgst_handle_t));
	MX_I("GST_Muxer_Init allocating memory for new_mediamuxer: %p\n", new_mediamuxer);
	if (!new_mediamuxer) {
		MX_E("Cannot allocate memory for muxer \n");
		ret = MX_ERROR;
		goto ERROR;
	}
	memset(new_mediamuxer, 0, sizeof(mxgst_handle_t));
	/* Set desired parameters */
	new_mediamuxer->output_uri = NULL;
	new_mediamuxer->muxed_format = -1;
	new_mediamuxer->video_track.media_format = NULL;
	new_mediamuxer->audio_track.media_format = NULL;
	new_mediamuxer->video_track.track_index = -1;
	new_mediamuxer->audio_track.track_index = -1;
	new_mediamuxer->video_track.start_feed = 1;
	new_mediamuxer->audio_track.start_feed = 1;
	new_mediamuxer->video_track.stop_feed = 0;
	new_mediamuxer->audio_track.stop_feed = 0;

	MX_I("V.start_feed[%d],V.stop_feed[%d],A.start_feed[%d],A.stop_feed[%d]\n",
	     new_mediamuxer->video_track.start_feed,
	     new_mediamuxer->video_track.stop_feed,
	     new_mediamuxer->audio_track.start_feed,
	     new_mediamuxer->audio_track.stop_feed);

	new_mediamuxer->eos_flg = false;
	*pHandle = (MMHandleType) new_mediamuxer;
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	MX_E("%s: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_set_data_sink(MMHandleType pHandle,
                  char *uri, mediamuxer_output_format_e format)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *mx_handle_gst = (mxgst_handle_t *) pHandle;

	/* Set desired parameters */
	mx_handle_gst->output_uri = uri;
	mx_handle_gst->muxed_format = format;
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	MX_E("muxer handle already NULL, returning \n");
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_add_track(MMHandleType pHandle,
                  media_format_h media_format, int *track_index)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *mx_handle_gst = (mxgst_handle_t *) pHandle;

	media_format_mimetype_e mimetype = 0;
	if (media_format_get_video_info(media_format, &mimetype, NULL, NULL, NULL, NULL) !=
                                                                    MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
		if (mimetype == MEDIA_FORMAT_H264_SP  ||
		    mimetype == MEDIA_FORMAT_H264_MP ||
		    mimetype == MEDIA_FORMAT_H264_HP) {
			if (mx_handle_gst->video_track.media_format == NULL) {
				mx_handle_gst->video_track.media_format = (void *)media_format;
				/* ToDo: track_index should be incremented accordingly for multiple tracks */
				mx_handle_gst->video_track.track_index = 1;
				mx_handle_gst->video_track.caps = NULL;
				*track_index = mx_handle_gst->video_track.track_index;
				MX_I("Video track added successfully: %p \n", mx_handle_gst->video_track.media_format);
			} else {
				MX_E("muxer handle already have a video track, \
				add failed, as Multiple video tracks are not currently supported");
				ret = MX_ERROR_NOT_SUPPORT_API;
				goto ERROR;
			}
		}
	} else if (media_format_get_audio_info(media_format, &mimetype, NULL, NULL, NULL, NULL) !=
	                                                                MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
		if (mimetype == MEDIA_FORMAT_AAC ||
		    mimetype == MEDIA_FORMAT_AAC_LC ||
		    mimetype == MEDIA_FORMAT_AAC_HE ||
		    mimetype == MEDIA_FORMAT_AAC_HE_PS)  {
			if (mx_handle_gst->audio_track.media_format == NULL) {
				mx_handle_gst->audio_track.media_format = (void *)media_format;
				/* ToDo: track_index should be incremented accordingly for multiple tracks */
				mx_handle_gst->audio_track.track_index = 2;
				mx_handle_gst->audio_track.caps = NULL;
				*track_index = mx_handle_gst->audio_track.track_index;
				MX_I("Audio track added successfully: %p \n", mx_handle_gst->audio_track.media_format);
			} else {
				MX_E("muxer handle already have a audio track, add failed, \
				as Multiple audio tracks are not currently supported");
				ret = MX_ERROR_NOT_SUPPORT_API;
				goto ERROR;
			}
		} else {
			MX_E("Unsupported MIME Type\n");
		}
	} else {
		MX_E("Unsupported A/V MIME Type\n");
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	MX_E(" gst_muxer_add_track failed \n");
	MEDIAMUXER_FLEAVE();
	return ret;
}

static gint __gst_handle_resource_error(mxgst_handle_t* gst_handle, int code)
{
	MEDIAMUXER_FENTER();
	gint trans_err = MEDIAMUXER_ERROR_NONE;
	g_return_val_if_fail(gst_handle, MX_PARAM_ERROR);

	switch (code) {
		/* ToDo: Add individual actions as and when needed */
		case GST_RESOURCE_ERROR_NO_SPACE_LEFT:
			trans_err = MX_ERROR_COMMON_NO_FREE_SPACE;
			break;
		case GST_RESOURCE_ERROR_WRITE:
		case GST_RESOURCE_ERROR_FAILED:
		case GST_RESOURCE_ERROR_SEEK:
		case GST_RESOURCE_ERROR_TOO_LAZY:
		case GST_RESOURCE_ERROR_BUSY:
		case GST_RESOURCE_ERROR_OPEN_WRITE:
		case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
		case GST_RESOURCE_ERROR_CLOSE:
		case GST_RESOURCE_ERROR_SYNC:
		case GST_RESOURCE_ERROR_SETTINGS:
		default:
			trans_err = MX_INTERNAL_ERROR;
			break;
	}
	MEDIAMUXER_FLEAVE();
	return trans_err;
}

static gint __gst_handle_core_error(mxgst_handle_t* gst_handle, int code)
{
	MEDIAMUXER_FENTER();
	gint trans_err = MEDIAMUXER_ERROR_NONE;

	/* g_return_val_if_fail(core, MX_PARAM_ERROR); */
	switch (code) {
		/* ToDo: Add individual actions as and when needed */
		case GST_CORE_ERROR_MISSING_PLUGIN:
		case GST_CORE_ERROR_STATE_CHANGE:
		case GST_CORE_ERROR_SEEK:
		case GST_CORE_ERROR_NOT_IMPLEMENTED:
		case GST_CORE_ERROR_FAILED:
		case GST_CORE_ERROR_TOO_LAZY:
		case GST_CORE_ERROR_PAD:
		case GST_CORE_ERROR_THREAD:
		case GST_CORE_ERROR_NEGOTIATION:
		case GST_CORE_ERROR_EVENT:
		case GST_CORE_ERROR_CAPS:
		case GST_CORE_ERROR_TAG:
		case GST_CORE_ERROR_CLOCK:
		case GST_CORE_ERROR_DISABLED:
		default:
			trans_err =  MX_INTERNAL_ERROR;
			break;
	}
	MEDIAMUXER_FLEAVE();
	return trans_err;
}

static gint __gst_handle_library_error(mxgst_handle_t* gst_handle, int code)
{
	MEDIAMUXER_FENTER();
	gint trans_err = MEDIAMUXER_ERROR_NONE;
	g_return_val_if_fail(gst_handle, MX_PARAM_ERROR);

	switch (code) {
		/* ToDo: Add individual actions as and when needed */
		case GST_LIBRARY_ERROR_FAILED:
		case GST_LIBRARY_ERROR_TOO_LAZY:
		case GST_LIBRARY_ERROR_INIT:
		case GST_LIBRARY_ERROR_SHUTDOWN:
		case GST_LIBRARY_ERROR_SETTINGS:
		case GST_LIBRARY_ERROR_ENCODE:
		default:
			trans_err =  MX_INTERNAL_ERROR;
			break;
	}
	MEDIAMUXER_FLEAVE();
	return trans_err;
}

static gboolean _mx_gst_bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	MEDIAMUXER_FENTER();
	int ret  = MX_ERROR_NONE;
	mxgst_handle_t *gst_handle = (mxgst_handle_t*)data;
	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS:
			MX_I("End of stream\n");
			break;
		case GST_MESSAGE_ERROR: {
				gchar *debug;
				GError *error;
				gst_message_parse_error(msg, &error, &debug);
				if (!error) {
					MX_E("GST error message parsing failed");
					break;
				}
				MX_E("Error: %s\n", error->message);
				if (error) {
					if (error->domain == GST_RESOURCE_ERROR)
						ret = __gst_handle_resource_error(gst_handle, error->code);
					else if (error->domain == GST_LIBRARY_ERROR)
						ret = __gst_handle_library_error(gst_handle, error->code);
					else if (error->domain == GST_CORE_ERROR)
						ret = __gst_handle_core_error(gst_handle, error->code);
					else
						MX_E("Unexpected error has occured");
					/* ToDo: Update the user callback with ret... */
					return ret;
				}
				g_free(debug);
				MX_E("Error: %s\n", error->message);
				g_error_free(error);
			}
			break;
		default:
			MX_E("unhandled message: 0x%x", GST_MESSAGE_TYPE(msg));
			break;
	}
	MEDIAMUXER_FLEAVE();
	return TRUE;
}

/*
  * This signal callback triggers when appsrc needs mux_data.
  * Here, we add an idle handler to the mainloop to start pushing data into the appsrc
  */
static void _audio_start_feed(GstElement *source, guint size, mxgst_handle_t *gst_handle)
{
	MX_I("\nAudio Start feeding...\n");
	gst_handle->audio_track.stop_feed = 0;
	gst_handle->audio_track.start_feed = 1;
}

static void _video_start_feed(GstElement *source, guint size, mxgst_handle_t *gst_handle)
{
	MX_I("\nVideo Start feeding cb...\n");
	gst_handle->video_track.stop_feed = 0;
	gst_handle->video_track.start_feed = 1;
}

/*
  * This callback triggers when appsrc has enough data and we can stop sending.
  * We remove the idle handler from the mainloop.
  */
static void _audio_stop_feed(GstElement *source, mxgst_handle_t *gst_handle)
{
	MX_I("\nAudio Stop feeding cb...\n");
	gst_handle->audio_track.stop_feed = 1;
	gst_handle->audio_track.start_feed = 0;
}

static void _video_stop_feed(GstElement *source, mxgst_handle_t *gst_handle)
{
	MX_I("\nVideo Stop feeding...\n");
	gst_handle->video_track.stop_feed = 1;
	gst_handle->video_track.start_feed = 0;
}

mx_ret_e _gst_create_pipeline(mxgst_handle_t *gst_handle)
{
	MEDIAMUXER_FENTER();
	gint ret = MX_ERROR_NONE;
	GstBus *bus = NULL;
	/* Note: Use a loop, if needed. GMainLoop *loop; */
	GstPad *audio_pad, *video_pad, *aud_src, *vid_src;

	/* Initialize GStreamer */
	/* Note: Replace the arguments of gst_init to pass the command line args to GStreamer. */
	gst_init(NULL, NULL);

	/* Create the empty pipeline */
	gst_handle->pipeline = gst_pipeline_new("Muxer Gst pipeline");

	if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_MP4) {
		gst_handle->muxer = gst_element_factory_make("qtmux", "media-muxer");
		/* gst_element_factory_make("mp4mux", "media-muxer"); */
		if (gst_handle->video_track.track_index == 1) { /* Video track */
			gst_handle->video_appsrc = gst_element_factory_make("appsrc", "video appsrc");
			gst_handle->videoparse = gst_element_factory_make("h264parse", "h264parse");
		}
		if (gst_handle->audio_track.track_index == 2) { /* Audio track */
			gst_handle->audio_appsrc = gst_element_factory_make("appsrc", "audio appsrc");
			gst_handle->audioparse = gst_element_factory_make("aacparse", "aacparse");
		}
	} else {
		MX_E("Unsupported format. Currently suports only MPEG4");
		ret = MEDIAMUXER_ERROR_INVALID_PATH;
		goto ERROR;
	}
	gst_handle->sink = gst_element_factory_make("filesink", "muxer filesink");

	if (!gst_handle->pipeline || !gst_handle->muxer || !gst_handle->sink) {
		MX_E("One element could not be created. Exiting.\n");
		ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
		goto ERROR;
	}

	/* Build the pipeline */
	gst_bin_add_many(GST_BIN(gst_handle->pipeline),
	                                                    gst_handle->muxer,
	                                                    gst_handle->sink,
	                                                    NULL);

	if (gst_handle->video_track.track_index == 1) {
		/* video track */
		if (!gst_handle->video_appsrc || !gst_handle->videoparse) {
			MX_E("One element (vparse) could not be created. Exiting.\n");
			ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
			goto ERROR;
		}
		gst_bin_add_many(GST_BIN(gst_handle->pipeline),
                                                                    gst_handle->video_appsrc,
                                                                    gst_handle->videoparse,
                                                                    NULL);
		/* Set video caps for corresponding src elements */
		g_object_set(gst_handle->video_appsrc, "caps",
		                        gst_caps_from_string(gst_handle->video_track.caps), NULL);
#ifdef ASYCHRONOUS_WRITE
		g_signal_connect(gst_handle->video_appsrc, "need-data",
		                        G_CALLBACK(_video_start_feed), gst_handle);
		g_signal_connect(gst_handle->video_appsrc, "enough-data",
		                        G_CALLBACK(_video_stop_feed), gst_handle);
#else
		g_object_set(gst_handle->video_appsrc, "block", TRUE, NULL);
		gst_app_src_set_stream_type((GstAppSrc *)gst_handle->video_appsrc,
		                        GST_APP_STREAM_TYPE_STREAM);
#endif
	}

	if (gst_handle->audio_track.track_index == 2) {
		/* Audio track */
		if (!gst_handle->audio_appsrc || !gst_handle->audioparse) {
			MX_E("One element (aparse) could not be created. Exiting.\n");
			ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
			goto ERROR;
		}
		gst_bin_add_many(GST_BIN(gst_handle->pipeline),
                                                                    gst_handle->audio_appsrc,
                                                                    gst_handle->audioparse,
                                                                    NULL);
		/* Set audio caps for corresponding src elements */
		g_object_set(gst_handle->audio_appsrc, "caps",
		                        gst_caps_from_string(gst_handle->audio_track.caps), NULL);
#ifdef ASYCHRONOUS_WRITE
		g_signal_connect(gst_handle->audio_appsrc, "need-data",
		                        G_CALLBACK(_audio_start_feed), gst_handle);
		g_signal_connect(gst_handle->audio_appsrc, "enough-data",
		                        G_CALLBACK(_audio_stop_feed), gst_handle);
#else
		g_object_set(gst_handle->audio_appsrc, "block", TRUE, NULL);
		gst_app_src_set_stream_type((GstAppSrc *)gst_handle->audio_appsrc,
		                        GST_APP_STREAM_TYPE_STREAM);
#endif
	}

	if (!gst_element_link(gst_handle->muxer, gst_handle->sink))
		MX_E("muxer-sink link failed");

	if (gst_handle->video_track.track_index == 1) {
		/* video track */
		gst_element_link(gst_handle->video_appsrc, gst_handle->videoparse);
		/* Link videoparse to muxer_video_pad.   Request for muxer A/V pads. */
		video_pad = gst_element_get_request_pad(gst_handle->muxer, "video_00");
		vid_src = gst_element_get_static_pad(gst_handle->videoparse, "src");
		if (gst_pad_link(vid_src, video_pad) != GST_PAD_LINK_OK)
			MX_E("video parser to muxer link failed");
	}

	if (gst_handle->audio_track.track_index == 2) {
		/* audio track */
		/* we add all elements into the pipeline */
		gst_element_link(gst_handle->audio_appsrc, gst_handle->audioparse);
		/* Link audioparse to muxer_audio_pad.   Request for muxer A/V pads. */
		audio_pad = gst_element_get_request_pad(gst_handle->muxer, "audio_00");
		aud_src = gst_element_get_static_pad(gst_handle->audioparse, "src");
		if (gst_pad_link(aud_src, audio_pad) != GST_PAD_LINK_OK)
			MX_E("audio parser to muxer link failed");
	}

	MX_I("Output_uri= %s\n", gst_handle->output_uri);
	g_object_set(GST_OBJECT(gst_handle->sink), "location",
	             gst_handle->output_uri, NULL);

	/* connect signals, bus watcher */
	bus = gst_pipeline_get_bus(GST_PIPELINE(gst_handle->pipeline));
	gst_handle->bus_watch_id = gst_bus_add_watch(bus, _mx_gst_bus_call, gst_handle);
	gst_object_unref(bus);

	/* set pipeline state to PLAYING */
	MEDIAMUXER_ELEMENT_SET_STATE(GST_ELEMENT_CAST(gst_handle->pipeline),
	                             GST_STATE_PLAYING);
	return MX_ERROR_NONE;

STATE_CHANGE_FAILED:
ERROR:

	if (gst_handle->pipeline)
		gst_object_unref(GST_OBJECT(gst_handle->pipeline));

	if (gst_handle->video_track.track_index == 1) { /* Video track */
		if (gst_handle->videoparse)
			gst_object_unref(GST_OBJECT(gst_handle->videoparse));

		if (gst_handle->video_appsrc)
			gst_object_unref(GST_OBJECT(gst_handle->video_appsrc));
	}

	if (gst_handle->audio_track.track_index == 2) { /* audio track */
		if (gst_handle->audio_appsrc)
			gst_object_unref(GST_OBJECT(gst_handle->audio_appsrc));
		if (gst_handle->audioparse)
			gst_object_unref(GST_OBJECT(gst_handle->audioparse));
	}

	if (gst_handle->muxer)
		gst_object_unref(GST_OBJECT(gst_handle->muxer));

	if (gst_handle->sink)
		gst_object_unref(GST_OBJECT(gst_handle->sink));
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_start(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *new_mediamuxer = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_start adding elements to the pipeline:%p\n", new_mediamuxer);
	ret = _gst_create_pipeline(new_mediamuxer);
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	MX_E("muxer handle NULL, returning \n");
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

int __gst_codec_specific_caps(GstCaps *new_cap,
                              media_format_mimetype_e mimetype)
{
	MEDIAMUXER_FENTER();
	GValue val = G_VALUE_INIT;
	switch (mimetype) {
			/*video*/
		case MEDIA_FORMAT_H261:
			break;
		case MEDIA_FORMAT_H263:
			break;
		case MEDIA_FORMAT_H263P:
			break;
		case MEDIA_FORMAT_H264_SP:
			break;
		case MEDIA_FORMAT_H264_MP:
			break;
		case MEDIA_FORMAT_H264_HP:
			break;
		case MEDIA_FORMAT_MJPEG:
			break;
		case MEDIA_FORMAT_MPEG1:
			break;
		case MEDIA_FORMAT_MPEG2_SP:
			break;
		case MEDIA_FORMAT_MPEG2_MP:
			break;
		case MEDIA_FORMAT_MPEG2_HP:
			break;
		case MEDIA_FORMAT_MPEG4_SP:
			break;
		case MEDIA_FORMAT_MPEG4_ASP:
			break;
		case MEDIA_FORMAT_HEVC:
			break;
		case MEDIA_FORMAT_VP8:
			break;
		case MEDIA_FORMAT_VP9:
			break;
		case MEDIA_FORMAT_VC1:
			break;
		case MEDIA_FORMAT_I420:
			break;
		case MEDIA_FORMAT_NV12:
			break;
		case MEDIA_FORMAT_NV12T:
			break;
		case MEDIA_FORMAT_YV12:
			break;
		case MEDIA_FORMAT_NV21:
			break;
		case MEDIA_FORMAT_NV16:
			break;
		case MEDIA_FORMAT_YUYV:
			break;
		case MEDIA_FORMAT_UYVY:
			break;
		case MEDIA_FORMAT_422P:
			break;
		case MEDIA_FORMAT_RGB565:
			break;
		case MEDIA_FORMAT_RGB888:
			break;
		case MEDIA_FORMAT_RGBA:
			break;
		case MEDIA_FORMAT_ARGB:
			break;
			/*audio*/
		case MEDIA_FORMAT_L16:
			break;
		case MEDIA_FORMAT_ALAW:
			break;
		case MEDIA_FORMAT_ULAW:
			break;
		case MEDIA_FORMAT_AMR:
			break;
		case MEDIA_FORMAT_AMR_WB:
			break;
		case MEDIA_FORMAT_G729:
			break;
		case MEDIA_FORMAT_AAC:
			g_value_init(&val, G_TYPE_INT);
			g_value_set_int(&val, 4);
			gst_caps_set_value(new_cap, "mpegversion", &val);
			break;
		case MEDIA_FORMAT_AAC_HE:
			g_value_init(&val, G_TYPE_INT);
			g_value_set_int(&val, 4);
			gst_caps_set_value(new_cap, "mpegversion", &val);
			break;
		case MEDIA_FORMAT_AAC_HE_PS:
			g_value_init(&val, G_TYPE_INT);
			g_value_set_int(&val, 4);
			gst_caps_set_value(new_cap, "mpegversion", &val);
			break;
		case MEDIA_FORMAT_MP3:
			break;
		case MEDIA_FORMAT_VORBIS:
			break;
		case MEDIA_FORMAT_PCM:
			break;
		case MEDIA_FORMAT_PCMA:
			break;
		case MEDIA_FORMAT_PCMU:
			break;
		default:
			MX_E("Unknown media mimeype %d. Assuming H264\n", mimetype);
			break;
	}
	MEDIAMUXER_FLEAVE();
	return 0;
}


int _gst_set_caps(MMHandleType pHandle, media_packet_h packet)
{
	MEDIAMUXER_FENTER();
	gint ret = MX_ERROR_NONE;
	GstCaps *new_cap;
	media_format_mimetype_e mimetype;
	media_format_h format;
	GValue val = G_VALUE_INIT;
	int numerator;
	int denominator = 1;
	int channel;
	int samplerate;
	int bit;
	int width;
	int height;
	int avg_bps;
	int max_bps;
	gchar *caps_string = NULL;

	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;
	media_format_type_e formattype;
	char *codec_data;
	unsigned int codec_data_size;

	if (media_packet_get_format(packet, &format)) {
		MX_E("media_format_get_formati call failed \n");
		goto ERROR;
	}

	if (media_format_get_type(format, &formattype)) {
		MX_E("media_format_get_type failed\n");
		goto ERROR;
	}

	switch (formattype) {
		case MEDIA_FORMAT_AUDIO:
			if (media_packet_get_codec_data(packet,
			                                (void **)&codec_data, &codec_data_size)) {
				MX_E("media_packet_get_codec_data call failed\n");
				ret = MX_ERROR_UNKNOWN;
				break;
			}
			MX_I("extracted codec data is =%s size is %d\n",
			     codec_data, codec_data_size);
			if (gst_handle->audio_track.caps == NULL ||
			    g_strcmp0(codec_data, gst_handle->audio_track.caps) != 0) {

#ifndef SEND_FULL_CAPS_VIA_CODEC_DATA

				if (media_format_get_audio_info(format,
				                                &mimetype, &channel, &samplerate,
				                                &bit, &avg_bps)) {
					MX_E("media_format_get_audio_info call failed\n");
					ret = MX_ERROR_UNKNOWN;
					break;
				}
				if (gst_handle->audio_track.caps == NULL) {
					gst_handle->audio_track.caps =
					    (char *)g_malloc(codec_data_size);
					if (gst_handle->audio_track.caps == NULL) {
						MX_E("[%s][%d]memory allocation failed\n",
						     __func__, __LINE__);
						ret = MX_ERROR_UNKNOWN;
						break;
					}
				}
				new_cap = gst_caps_from_string(codec_data);
				if (__gst_codec_specific_caps(new_cap, mimetype)) {
					MX_E("Setting Audio caps failed\n");
					gst_caps_unref(new_cap);
					ret = MX_ERROR_UNKNOWN;
					break;
				}
				caps_string = gst_caps_to_string(new_cap);
				MX_I("New cap set by codec data is=%s\n",
				     caps_string);
				if (caps_string)
					g_free(caps_string);
				g_object_set(gst_handle->audio_appsrc,
				             "caps", new_cap, NULL);
				g_stpcpy(gst_handle->audio_track.caps, codec_data);
#else
				/*Debugging purpose. The whole caps filter can be sent via codec_data*/
				new_cap = gst_caps_from_string(codec_data);
				MX_I("codec  cap is=%s\n", codec_data);
				g_object_set(gst_handle->audio_appsrc,
				             "caps", new_cap, NULL);
				if (gst_handle->audio_track.caps == NULL) {
					gst_handle->audio_track.caps =
					    (char *)g_malloc(codec_data_size);
					if (gst_handle->audio_track.caps == NULL) {
						MX_E("[%s][%d] \
					memory allocation failed\n",
						     __func__, __LINE__);
						gst_caps_unref(new_cap);
						ret = MX_ERROR_UNKNOWN;
						break;
					}
				}
				g_stpcpy(gst_handle->audio_track.caps, codec_data);
#endif
				gst_caps_unref(new_cap);
			}
			break;
		case MEDIA_FORMAT_VIDEO:
			if (media_packet_get_codec_data(packet,
			                                (void **)&codec_data, &codec_data_size)) {
				MX_E("media_packet_get_codec_data call failed\n");
				ret = MX_ERROR_UNKNOWN;
				break;
			}
			MX_I("codec data is =%s size is %d\n",
			     codec_data, codec_data_size);
			if (gst_handle->video_track.caps == NULL ||
			    g_strcmp0(codec_data, gst_handle->video_track.caps) != 0) {

#ifndef SEND_FULL_CAPS_VIA_CODEC_DATA

				if (media_format_get_video_info(format,
				                                &mimetype, &width, &height,
				                                &avg_bps, &max_bps)) {
					MX_E("media_format_get_video_info call failed\n");
					ret = MX_ERROR_UNKNOWN;
					break;
				}
				if (gst_handle->video_track.caps == NULL) {
					gst_handle->video_track.caps =
					    (char *)g_malloc(codec_data_size);
					if (gst_handle->video_track.caps == NULL) {
						MX_E("[%s][%d] \
					memory allocation failed\n",
						     __func__, __LINE__);
						ret = MX_ERROR_UNKNOWN;
						break;
					}
				}
				new_cap = gst_caps_from_string(codec_data);
				MX_I("New cap set by codec data is=%s\n",
				     codec_data);
				if (__gst_codec_specific_caps(new_cap, mimetype)) {
					MX_E("Setting Audio caps failed\n");
					gst_caps_unref(new_cap);
					ret = MX_ERROR_UNKNOWN;
					break;
				}
				g_stpcpy(gst_handle->video_track.caps, codec_data);

				if (media_format_get_video_frame_rate(format,
				                                      &numerator)) {
					MX_E("media_format_get_video_info call failed\n");
				}
				g_value_init(&val, GST_TYPE_FRACTION);
				gst_value_set_fraction(&val, numerator, denominator);
				gst_caps_set_value(new_cap, "framerate", &val);
				caps_string = gst_caps_to_string(new_cap);
				MX_I("New cap set by codec data is=%s\n",
				     caps_string);
				if (caps_string)
					g_free(caps_string);
				g_object_set(gst_handle->video_appsrc, "caps",
				             new_cap, NULL);
#else
				/*Debugging purpose. The whole caps filter can be sent via codec_data*/
				media_packet_get_codec_data(packet, &codec_data,
				                            &codec_data_size);
				MX_I("extracted codec data is =%s\n", codec_data);
				new_cap = gst_caps_from_string(codec_data);
				MX_I("New cap is=%s\n", codec_data);
				g_object_set(gst_handle->video_appsrc, "caps",
				             new_cap, NULL);
				if (gst_handle->video_track.caps == NULL) {
					gst_handle->video_track.caps =
					    (char *)g_malloc(codec_data_size);
					if (gst_handle->video_track.caps == NULL) {
						MX_E("[%s][%d] \
					memory allocation failed\n",
						     __func__, __LINE__);
						gst_caps_unref(new_cap);
						ret = MX_ERROR_UNKNOWN;
						break;
					}
				}
				g_stpcpy(gst_handle->video_track.caps, codec_data);
#endif
				gst_caps_unref(new_cap);
			}
			break;
		case MEDIA_FORMAT_CONTAINER:
		case MEDIA_FORMAT_TEXT:
		case MEDIA_FORMAT_UNKNOWN:
		default:
			MX_E("Unknown format type\n");
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_UNKNOWN;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int _gst_copy_media_packet_to_buf(media_packet_h out_pkt,
                                         GstBuffer *buffer)
{
	MEDIAMUXER_FENTER();
	void *pkt_data;
	uint64_t size;
	unsigned char *data_ptr;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(out_pkt);
	/* GstMapInfo map; */
	int ret = MX_ERROR_NONE;
	/* copy data*/
	media_packet_get_buffer_size(out_pkt, &size);
	MX_I("Media packet Buffer capacity: %llu\n", size);
	data_ptr = (unsigned char *) g_malloc(size);
	if (!data_ptr) {
		MX_E("Memory allocation failed in %s \n", __FUNCTION__);
		ret = MX_MEMORY_ERROR;
		goto ERROR;
	}

	if (media_packet_get_buffer_data_ptr(out_pkt, &pkt_data)) {
		MX_E("unable to get the buffer pointer \
				from media_packet_get_buffer_data_ptr\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}
	/*if (!gst_buffer_map (buffer, &map, GST_MAP_READ)) {
		MX_E("gst_buffer_map failed\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}*/
	uint64_t info;
	memcpy(data_ptr, (char *)pkt_data, size);
	gst_buffer_insert_memory(buffer, -1,
	                         gst_memory_new_wrapped(0, data_ptr, size, 0,
	                                                size, data_ptr, g_free));

	if (media_packet_get_pts(out_pkt, &info)) {
		MX_E("unable to get the pts\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}
	buffer->pts = info;

	if (media_packet_get_dts(out_pkt, &info)) {
		MX_E("unable to get the dts\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}
	buffer->dts = info;
	if (media_packet_get_duration(out_pkt, &info)) {
		MX_E("unable to get the duration\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}
	buffer->duration = info;
	/*TBD: set falgs is not available now in media_packet*/
	media_buffer_flags_e flags;
	if (media_packet_get_flags(out_pkt, &flags)) {
		MX_E("unable to get the buffer size\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	}
	GST_BUFFER_FLAG_SET(buffer, flags);
ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_write_sample(MMHandleType pHandle, int track_index,
                                         media_packet_h inbuf)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	_gst_set_caps(pHandle, inbuf);
	MX_I("\nTrack_index=%d", track_index);

	GstBuffer *gst_inbuf2 = NULL;
	gst_inbuf2 = gst_buffer_new();
	/* ToDo: Add functionality to the following function */
	/* MX_I("\nBefore  buff=%x", gst_inbuf2); */
	_gst_copy_media_packet_to_buf(inbuf, gst_inbuf2);

	if ((gst_handle->video_track.track_index != -1) &&
             (track_index == gst_handle->video_track.track_index)) {
		MX_I("\n pushing video");
#ifdef ASYCHRONOUS_WRITE
		/*poll now to make it synchronous*/
		while (gst_handle->video_track.start_feed == 0) {
			g_usleep(WRITE_POLL_PERIOD);
		}
		g_signal_emit_by_name(gst_handle->video_appsrc, "push-buffer", gst_inbuf2, &ret);
#else
		ret = gst_app_src_push_buffer((GstAppSrc *)gst_handle->video_appsrc, gst_inbuf2);
#endif
		MX_I("\n attempted a vid-buf push");
		if (ret != GST_FLOW_OK) {
			/* We got some error, stop sending data */
			MX_E("--video appsrc push failed--");
		}
	} else if ((gst_handle->audio_track.track_index != -1) &&
	                (track_index == gst_handle->audio_track.track_index)) {
		MX_I("\n pushing audio");
#ifdef ASYCHRONOUS_WRITE
		while (gst_handle->audio_track.start_feed == 0) {
			g_usleep(WRITE_POLL_PERIOD);
		}
		g_signal_emit_by_name(gst_handle->audio_appsrc, "push-buffer", gst_inbuf2, &ret);
#else
		ret = gst_app_src_push_buffer((GstAppSrc *)gst_handle->audio_appsrc, gst_inbuf2);
#endif
		MX_I("\n attempted a aud-buf push");
		if (ret != GST_FLOW_OK) {
			/* We got some error, stop sending data */
			MX_E("--audio appsrc push failed--");
		}
	} else {
		MX_E("\nUnsupported track index. Only 1/2 track index is vaild");
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_close_track(MMHandleType pHandle, int track_index)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_stop setting eos to sources:%p\n", gst_handle);
	if (gst_handle->pipeline!= NULL) {
		if (gst_handle->audio_track.track_index == track_index) {
			MX_I("\n-----EOS for audioappsrc-----\n");
			gst_app_src_end_of_stream((GstAppSrc *)(gst_handle->audio_appsrc));
		} else if (gst_handle->video_track.track_index == track_index) {
			MX_I("\n-----EOS for videoappsrc-----\n");
			gst_app_src_end_of_stream((GstAppSrc *)(gst_handle->video_appsrc));
		} else {
			MX_E("\nInvalid track Index[%d].\n", track_index);
			goto ERROR;
		}
	}
	if (gst_handle->audio_track.track_index == track_index) {
		gst_handle->audio_track.media_format = NULL;
		gst_handle->audio_track.track_index  = -1;
	} else if (gst_handle->video_track.track_index == track_index) {
		gst_handle->video_track.media_format = NULL;
		gst_handle->video_track.track_index  = -1;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_pause(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	GstState state;
	MX_I("gst_muxer_pause setting pipeline to pause");
	gst_element_get_state(gst_handle->pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
	if (state == GST_STATE_PLAYING) {
		if (gst_element_set_state(gst_handle->pipeline, GST_STATE_PAUSED) ==
                        GST_STATE_CHANGE_FAILURE) {
			MX_I("Setting pipeline to pause failed");
			ret = MX_ERROR_INVALID_ARGUMENT;
		}
	} else {
		MX_I("pipeline is not in playing, PAUSE is intended to pause a playing pipeline. \
			exiting with out state change");
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_resume(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(pHandle);
	int ret = MX_ERROR_NONE;
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("gst_muxer_resume setting pipeline to playing");
	if (gst_element_set_state(gst_handle->pipeline, GST_STATE_PLAYING) ==
                GST_STATE_CHANGE_FAILURE) {
		MX_I("Setting pipeline to resume failed");
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

mx_ret_e _gst_destroy_pipeline(mxgst_handle_t *gst_handle)
{
	gint ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();

	/* Clean up nicely */
	gst_element_set_state(gst_handle->pipeline, GST_STATE_NULL);

	/* Free resources & set unused pointers to NULL */
	if (gst_handle->output_uri != NULL) {
		gst_handle->output_uri = NULL;
	}

	if (gst_handle->video_track.track_index == 1) { /* Video track */
		if (gst_handle->video_track.caps != NULL) {
			g_free(gst_handle->video_track.caps);
		}

		if (gst_handle->video_track.media_format != NULL) {
			gst_handle->video_track.media_format = NULL;
		}
	}

	if (gst_handle->audio_track.track_index == 2) { /* audio track */
		if (gst_handle->audio_track.caps != NULL) {
			g_free(gst_handle->audio_track.caps);
		}

		if (gst_handle->audio_track.media_format != NULL) {
			gst_handle->audio_track.media_format = NULL;
		}
	}

	if (gst_handle->pipeline)
		gst_object_unref(GST_OBJECT(gst_handle->pipeline));

	g_source_remove(gst_handle->bus_watch_id);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_stop(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_stop setting eos to sources:%p\n", gst_handle);
	ret = _gst_destroy_pipeline(gst_handle);
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_destroy(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *new_mediamuxer = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_destroy deallocating new_mediamuxer:%p\n",  new_mediamuxer);
	g_free(new_mediamuxer);
	MEDIAMUXER_FLEAVE();
	return ret;
ERROR:
	MX_E("muxer handle already NULL, returning \n");
	MEDIAMUXER_FLEAVE();
	return ret;
}

int gst_set_error_cb(MMHandleType pHandle, gst_error_cb callback, void* user_data)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	if (!gst_handle) {
		MX_E("fail invaild param\n");
		ret = MX_INVALID_ARG;
		goto ERROR;
	}

	if (gst_handle->user_cb[_GST_EVENT_TYPE_ERROR]) {
		MX_E("Already set mediamuxer_error_cb\n");
		ret = MX_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}
	else {
		if (!callback) {
			ret = MX_ERROR_INVALID_ARGUMENT;
			goto ERROR;
		}
	}

	MX_I("Set event handler callback(cb = %p, data = %p)\n", callback, user_data);
	gst_handle->user_cb[_GST_EVENT_TYPE_ERROR] = (gst_error_cb) callback;
	gst_handle->user_data[_GST_EVENT_TYPE_ERROR] = user_data;
	MEDIAMUXER_FLEAVE();
	return MX_ERROR_NONE;
ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}
