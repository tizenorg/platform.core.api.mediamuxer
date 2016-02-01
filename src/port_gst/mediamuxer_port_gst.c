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
#define SEND_FULL_CAPS_VIA_CODEC_DATA /* For debug purpose */
#define ASYCHRONOUS_WRITE  /* write sample is not blocking */

static int gst_muxer_init(MMHandleType *pHandle);
static int gst_muxer_set_data_sink(MMHandleType pHandle, char *uri,
			mediamuxer_output_format_e format);
static int gst_muxer_add_track(MMHandleType pHandle,
			media_format_h media_format, int *track_index);
static int gst_muxer_prepare(MMHandleType pHandle);
static int gst_muxer_start(MMHandleType pHandle);
static int gst_muxer_write_sample(MMHandleType pHandle, int track_index,
			media_packet_h inbuf);
static int gst_muxer_close_track(MMHandleType pHandle, int track_index);
static int gst_muxer_pause(MMHandleType pHandle);
static int gst_muxer_resume(MMHandleType pHandle);
static int gst_muxer_stop(MMHandleType pHandle);
static int gst_muxer_unprepare(MMHandleType pHandle);
static int gst_muxer_destroy(MMHandleType pHandle);
static int gst_set_error_cb(MMHandleType pHandle,
			gst_error_cb callback, void* user_data);

/* Media Muxer API common */
static media_port_muxer_ops def_mux_ops = {
	.n_size = 0,
	.init = gst_muxer_init,
	.set_data_sink = gst_muxer_set_data_sink,
	.add_track = gst_muxer_add_track,
	.prepare = gst_muxer_prepare,
	.start = gst_muxer_start,
	.write_sample = gst_muxer_write_sample,
	.close_track = gst_muxer_close_track,
	.pause = gst_muxer_pause,
	.resume = gst_muxer_resume,
	.stop = gst_muxer_stop,
	.unprepare = gst_muxer_unprepare,
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
	new_mediamuxer->track_info.audio_track_cnt = 0;
	new_mediamuxer->track_info.video_track_cnt = 0;
	new_mediamuxer->track_info.subtitle_track_cnt = 0;
	new_mediamuxer->track_info.total_track_cnt = 0;
	new_mediamuxer->track_info.track_head = NULL;

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
}

static int gst_muxer_add_track(MMHandleType pHandle,
			media_format_h media_format, int *track_index)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *mx_handle_gst = (mxgst_handle_t *) pHandle;
	media_format_mimetype_e mimetype = 0;
	mx_gst_track *current = NULL;
	mx_gst_track *last = NULL;

	if (!mx_handle_gst->output_uri) {
		MX_E("URI is null. Possibly, set_data_sink failed. returning. \n");
		return MX_ERROR_INVALID_ARGUMENT;
	}

	current = (mx_gst_track *)g_malloc(sizeof(mx_gst_track));
	if (!current) {
		MX_E("Not able to allocate memory\n");
		return MX_ERROR;
	}
	MX_I("Memory allocated to track = %p\n", current);
	current->media_format = (void *)media_format;
	current->start_feed = 1;
	current->stop_feed = 0;
	current->caps = NULL;
	current->next = NULL;
	if (mx_handle_gst->track_info.track_head == NULL) {
		MX_I("Adding first-ever track\n");
		mx_handle_gst->track_info.track_head = current;
	} else {
		MX_I("Adding %d-track)\n", 1+mx_handle_gst->track_info.total_track_cnt);
		last = mx_handle_gst->track_info.track_head;
		while (last->next != NULL)
			last = last->next;
		/* Add current track after present-last */
		last->next = current;
	}

	if (media_format_get_video_info(media_format, &mimetype, NULL, NULL, NULL, NULL) !=
		MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
		if ((mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_MP4
				&& (mimetype == MEDIA_FORMAT_H264_SP  || mimetype == MEDIA_FORMAT_H264_MP || mimetype == MEDIA_FORMAT_H264_HP
				|| mimetype == MEDIA_FORMAT_H263
				|| mimetype == MEDIA_FORMAT_MPEG4_SP))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP
				&& (mimetype == MEDIA_FORMAT_H264_SP  || mimetype == MEDIA_FORMAT_H264_MP || mimetype == MEDIA_FORMAT_H264_HP
				|| mimetype == MEDIA_FORMAT_H263))) {

			current->track_index = NO_OF_TRACK_TYPES*(mx_handle_gst->track_info.video_track_cnt);
			(mx_handle_gst->track_info.video_track_cnt)++;
			(mx_handle_gst->track_info.total_track_cnt)++;
			*track_index = current->track_index;

			MX_I("Video track added successfully: %p, with head: %p \n",
				current->media_format, mx_handle_gst->track_info.track_head->media_format);
		}  else {
			MX_E("Unsupported/Mismatched video MIME Type: %x\n", mimetype);
		}
	} else if (media_format_get_audio_info(media_format, &mimetype, NULL, NULL, NULL, NULL) !=
		MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
		if ((mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_MP4
				&& (mimetype == MEDIA_FORMAT_AAC_LC || mimetype == MEDIA_FORMAT_AAC_HE || mimetype == MEDIA_FORMAT_AAC_HE_PS))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP
				&& (mimetype == MEDIA_FORMAT_AAC_LC || mimetype == MEDIA_FORMAT_AAC_HE || mimetype == MEDIA_FORMAT_AAC_HE_PS
				|| mimetype == MEDIA_FORMAT_AMR_NB))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_WAV
				&& (mimetype == MEDIA_FORMAT_PCM))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS
				&& (mimetype == MEDIA_FORMAT_AAC))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_NB
				&& (mimetype == MEDIA_FORMAT_AMR_NB))
			|| (mx_handle_gst->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_WB
				&& (mimetype == MEDIA_FORMAT_AMR_WB))) {

			current->track_index = 1 + NO_OF_TRACK_TYPES*(mx_handle_gst->track_info.audio_track_cnt);
			(mx_handle_gst->track_info.audio_track_cnt)++;
			(mx_handle_gst->track_info.total_track_cnt)++;
			*track_index = current->track_index;

			MX_I("Audio track added successfully: %p, with head: %p \n",
				current->media_format, mx_handle_gst->track_info.track_head->media_format);

		} else {
			MX_E("Unsupported/Mismatched audio MIME Type: %x\n", mimetype);
		}
	} else {
		MX_E("Unsupported A/V MIME Type: %x\n", mimetype);
	}
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
	gboolean ret = TRUE;
	int error_val  = MX_ERROR_NONE;
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
				MX_E("GStreamer callback error message parsing failed\n");
				ret = FALSE;
				break;
			}
			MX_I("GStreamer callback Error: %s\n", error->message);
			if (error) {
				if (error->domain == GST_RESOURCE_ERROR)
					error_val = __gst_handle_resource_error(gst_handle, error->code);
				else if (error->domain == GST_LIBRARY_ERROR)
					error_val = __gst_handle_library_error(gst_handle, error->code);
				else if (error->domain == GST_CORE_ERROR)
					error_val = __gst_handle_core_error(gst_handle, error->code);
				else
					MX_I("Unknown GStreamer callback error\n");
				/* Update the user callback with ret value */
				((gst_error_cb)gst_handle->user_cb[_GST_EVENT_TYPE_ERROR])(error_val, (void*)error->message);

				return ret;
			}
			g_free(debug);
			MX_E("Error: %s\n", error->message);
			g_error_free(error);
		}
		break;
	default:
		MX_I("unhandled gst callback message: 0x%x\n", GST_MESSAGE_TYPE(msg));
		break;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
}

/*
  * This signal callback triggers when appsrc needs mux_data.
  * Here, we add an idle handler to the mainloop to start pushing data into the appsrc
  */
static void _audio_start_feed(GstElement *source, guint size, mx_gst_track *current)
{
	if (current) {
		MX_I("\nAudio Start feeding..., current->track_index = %d\n", current->track_index);
		current->stop_feed = 0;
		current->start_feed = 1;
	} else
		MX_I("Audio start feed called, however current handle is null\n");
}

static void _video_start_feed(GstElement *source, guint size, mx_gst_track *current)
{
	if (current) {
		MX_I("\nVideo Start feeding cb... current->track_index = %d\n", current->track_index);
		current->stop_feed = 0;
		current->start_feed = 1;
	} else
		MX_I("Video start feed called, however, current is null\n");
}

/*
  * This callback triggers when appsrc has enough data and we can stop sending.
  * We remove the idle handler from the mainloop.
  */
static void _audio_stop_feed(GstElement *source, mx_gst_track *current)
{
	if (current) {
		MX_I("\nAudio Stop feeding cb... current->track_index = %d\n", current->track_index);
		current->stop_feed = 1;
		current->start_feed = 0;
	} else
		MX_I("Audio stop feed called, however, current is null\n");
}

static void _video_stop_feed(GstElement *source, mx_gst_track *current)
{
	if (current) {
		MX_I("\nVideo Stop feeding... current->track_index = %d\n", current->track_index);
		current->stop_feed = 1;
		current->start_feed = 0;
	} else
		MX_I("Video stop feed called, however, current is null\n");
}

mx_ret_e _gst_create_pipeline(mxgst_handle_t *gst_handle)
{
	MEDIAMUXER_FENTER();
	gint ret = MX_ERROR_NONE;
	GstBus *bus = NULL;
	/* Note: Use a loop, if needed. GMainLoop *loop; */
	GstPad *audio_pad, *video_pad, *aud_src, *vid_src;
	char str_appsrc[MAX_STRING_LENGTH];
	char str_parser[MAX_STRING_LENGTH];
	char track_no[MAX_STRING_LENGTH];
	int vid_track_cnt = 0;
	int aud_track_cnt = 0;
	mx_gst_track *current = NULL;
	media_format_mimetype_e mimetype = 0;

	/* Initialize GStreamer */
	/* Note: Replace the arguments of gst_init to pass the command line args to GStreamer. */
	gst_init(NULL, NULL);

	/* Create the empty pipeline */
	gst_handle->pipeline = gst_pipeline_new("Muxer Gst pipeline");

	/* Link the pipeline */
	gst_handle->sink = gst_element_factory_make("filesink", "muxer filesink");

	if (gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_MP4
		&& gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_3GP
		&& gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_WAV
		&& gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS
		&& gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_AMR_NB
		&& gst_handle->muxed_format != MEDIAMUXER_CONTAINER_FORMAT_AMR_WB) {
		MX_E("Unsupported container-format. Currently suports only MP4, 3GP, WAV & AMR\n");
		ret = MEDIAMUXER_ERROR_INVALID_PATH;
		goto ERROR;
	} else {
		if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_MP4)
			gst_handle->muxer = gst_element_factory_make("qtmux", "qtmux");
			/* gst_element_factory_make("mp4mux", "mp4mux"); */
		else if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP)
			gst_handle->muxer = gst_element_factory_make("3gppmux", "3gppmux");
			/* gst_handle->muxer = gst_element_factory_make("avmux_3gp", "avmux_3gp");*/
			/* gst_handle->muxer = gst_element_factory_make("qtmux", "qtmux"); */
		else if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_WAV)
			gst_handle->muxer = gst_element_factory_make("wavenc", "wavenc");
		else if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS)
			gst_handle->muxer = gst_element_factory_make("avmux_adts", "avmux_adts");
		else if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_NB
			|| gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_WB)
			gst_handle->muxer = gst_element_factory_make("avmux_amr", "avmux_amr");

		if ((!gst_handle->pipeline) || (!gst_handle->muxer) || (!gst_handle->sink)) {
			MX_E("One element could not be created. Exiting.\n");
			ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
			goto ERROR;
		}

		/* Build the pipeline */
		gst_bin_add_many(GST_BIN(gst_handle->pipeline), gst_handle->muxer, gst_handle->sink, NULL);

		if (!gst_element_link(gst_handle->muxer, gst_handle->sink))
			MX_E("muxer-sink link failed\n");

		if (gst_handle->track_info.video_track_cnt) { /* Video track(s) exist */
			for (current = gst_handle->track_info.track_head; current; current = current->next) {
				if (current->track_index%NO_OF_TRACK_TYPES == 0) { /* Video track */

					snprintf(str_appsrc, MAX_STRING_LENGTH - 1, "video_appsrc%d", current->track_index);
					snprintf(str_parser, MAX_STRING_LENGTH - 1, "video_parser%d", current->track_index);

					current->appsrc = gst_element_factory_make("appsrc", str_appsrc);

					if (media_format_get_video_info((media_format_h)(current->media_format), &mimetype, NULL, NULL, NULL, NULL)
						!= MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
							if (mimetype == MEDIA_FORMAT_H264_SP  || mimetype == MEDIA_FORMAT_H264_MP || mimetype == MEDIA_FORMAT_H264_HP)
								current->parser = gst_element_factory_make("h264parse", str_parser);
							else if (mimetype == MEDIA_FORMAT_H263 || mimetype == MEDIA_FORMAT_H263P)
								current->parser = gst_element_factory_make("h263parse", str_parser);
							else if (mimetype == MEDIA_FORMAT_MPEG4_SP)
								current->parser = gst_element_factory_make("mpeg4videoparse", str_parser);
					} else {
						MX_E("Can't retrive mimetype for the current track. Unsupported MIME Type\n");
						ret = MEDIAMUXER_ERROR_INVALID_PARAMETER;
						goto ERROR;
					}

					if ((!current->appsrc)  || (!current->parser)) {
						MX_E("One element (video_appsrc/vparse) could not be created. Exiting.\n");
						ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
						goto ERROR;
					}

					gst_bin_add_many(GST_BIN(gst_handle->pipeline), current->appsrc, current->parser, NULL);

#ifdef ASYCHRONOUS_WRITE
					/* ToDo: Use a function pointer, and create independent fucntions to each track */
					MX_I("\nRegistering video callback for cur->tr_ind = %d\n", current->track_index);
					g_signal_connect(current->appsrc, "need-data",
						G_CALLBACK(_video_start_feed), current);
					g_signal_connect(current->appsrc, "enough-data",
						G_CALLBACK(_video_stop_feed), current);
#else
					g_object_set(current->appsrc, "block", TRUE, NULL);
					gst_app_src_set_stream_type((GstAppSrc *)current->appsrc,
						GST_APP_STREAM_TYPE_STREAM);
#endif
					gst_element_link(current->appsrc, current->parser);

					/* Link videoparse to muxer_video_pad.   Request for muxer A/V pads. */
					snprintf(track_no, MAX_STRING_LENGTH - 1, "video_%.2d", vid_track_cnt++);  /* sprintf(track_no,"video_00"); */

					video_pad = gst_element_get_request_pad(gst_handle->muxer, track_no);
					vid_src = gst_element_get_static_pad(current->parser, "src");

					if (gst_pad_link(vid_src, video_pad) != GST_PAD_LINK_OK)
						MX_E("video parser to muxer link failed\n");

					gst_object_unref(GST_OBJECT(vid_src));
					gst_object_unref(GST_OBJECT(video_pad));
				}
			}
		}

		if (gst_handle->track_info.audio_track_cnt) { /* Audio track(s) exist */
			for (current = gst_handle->track_info.track_head; current; current = current->next) {
				if (current->track_index%NO_OF_TRACK_TYPES == 1) {

					snprintf(str_appsrc, MAX_STRING_LENGTH - 1, "audio_appsrc%d", current->track_index);
					snprintf(str_parser, MAX_STRING_LENGTH - 1, "audio_parser%d", current->track_index);

					current->appsrc = gst_element_factory_make("appsrc", str_appsrc);

					if (media_format_get_audio_info((media_format_h)(current->media_format), &mimetype, NULL, NULL, NULL, NULL) !=
						MEDIA_FORMAT_ERROR_INVALID_OPERATION) {
						if (mimetype == MEDIA_FORMAT_AAC_LC || mimetype == MEDIA_FORMAT_AAC_HE || mimetype == MEDIA_FORMAT_AAC_HE_PS)
							current->parser = gst_element_factory_make("aacparse", str_parser);
						 else if (mimetype == MEDIA_FORMAT_PCM
							|| (mimetype == MEDIA_FORMAT_AAC && gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS)
							|| (mimetype == MEDIA_FORMAT_AMR_NB && gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP))
							 MX_I("Do Nothing, as there is no need of parser for wav, 3gp(amr-nb) and AAC_ADTS\n");
						else if (mimetype == MEDIA_FORMAT_AMR_NB || mimetype == MEDIA_FORMAT_AMR_WB)
							current->parser = gst_element_factory_make("amrparse", str_parser);
					} else {
						MX_E("Can't retrive mimetype for the current track. Unsupported MIME Type. Proceeding to the next track\n");
					}

					if (!current->appsrc) {
						MX_E("One element (audio_appsrc) could not be created. Exiting.\n");
						ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
						goto ERROR;
					}

					gst_bin_add_many(GST_BIN(gst_handle->pipeline), current->appsrc, NULL);
					if (mimetype == MEDIA_FORMAT_PCM
						|| (mimetype == MEDIA_FORMAT_AAC && gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS)
						|| (mimetype == MEDIA_FORMAT_AMR_NB && gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP)) {
						MX_I("Do Nothing, as there is no need of parser for wav and AAC_ADTS\n");
					} else {
						if (!current->parser) {
							MX_E("One element (audio-parser) could not be created. Exiting.\n");
							ret = MEDIAMUXER_ERROR_RESOURCE_LIMIT;
							goto ERROR;
						}
						gst_bin_add_many(GST_BIN(gst_handle->pipeline), current->parser, NULL);
					}

#ifdef ASYCHRONOUS_WRITE
					/* ToDo: Use a function pointer, and create independent fucntions to each track */
					MX_I("\nRegistering audio callback for cur->tr_ind = %d\n", current->track_index);
					g_signal_connect(current->appsrc, "need-data",
						G_CALLBACK(_audio_start_feed), current);
					g_signal_connect(current->appsrc, "enough-data",
						G_CALLBACK(_audio_stop_feed), current);
					g_object_set (current->appsrc, "format", GST_FORMAT_TIME, NULL);
#else
					g_object_set(current->appsrc, "block", TRUE, NULL);
					gst_app_src_set_stream_type((GstAppSrc *)current->appsrc,
						GST_APP_STREAM_TYPE_STREAM);
#endif
					/* For wav, wavenc is muxer */
					if (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_WAV
						|| gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AAC_ADTS
						|| gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_NB
						|| gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_AMR_WB
						|| (gst_handle->muxed_format == MEDIAMUXER_CONTAINER_FORMAT_3GP && mimetype == MEDIA_FORMAT_AMR_NB)) {
						gst_element_link(current->appsrc, gst_handle->muxer);
					} else {
						gst_element_link(current->appsrc, current->parser);
						/* Link videoparse to muxer_video_pad.   Request for muxer A/V pads. */
						snprintf(track_no, MAX_STRING_LENGTH - 1, "audio_%.2d", aud_track_cnt++);  /* sprintf(track_no,"audio_00"); */

						audio_pad = gst_element_get_request_pad(gst_handle->muxer, track_no);
						aud_src = gst_element_get_static_pad(current->parser, "src");

						if (gst_pad_link(aud_src, audio_pad) != GST_PAD_LINK_OK)
							MX_E("audio parser to muxer link failed\n");

						gst_object_unref(GST_OBJECT(aud_src));
						gst_object_unref(GST_OBJECT(audio_pad));
					}
				}
			}
		}
	}

	MX_I("Output_uri= %s\n", gst_handle->output_uri);
	g_object_set(GST_OBJECT(gst_handle->sink), "location",
				gst_handle->output_uri, NULL);

	/* connect signals, bus watcher */
	bus = gst_pipeline_get_bus(GST_PIPELINE(gst_handle->pipeline));
	gst_handle->bus_watch_id = gst_bus_add_watch(bus, _mx_gst_bus_call, gst_handle);
	gst_object_unref(bus);

	/* set pipeline state to READY */
	MEDIAMUXER_ELEMENT_SET_STATE(GST_ELEMENT_CAST(gst_handle->pipeline),
					GST_STATE_READY);
	return MX_ERROR_NONE;

STATE_CHANGE_FAILED:
ERROR:

	if (gst_handle->pipeline)
		gst_object_unref(GST_OBJECT(gst_handle->pipeline));

	for (current = gst_handle->track_info.track_head; current; current = current->next) {
		if (current->appsrc)
			gst_object_unref(GST_OBJECT(current->appsrc));
		if (current->parser)
			gst_object_unref(GST_OBJECT(current->parser));
	}

	if (gst_handle->muxer)
		gst_object_unref(GST_OBJECT(gst_handle->muxer));

	if (gst_handle->sink)
		gst_object_unref(GST_OBJECT(gst_handle->sink));
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_prepare(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *new_mediamuxer = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_prepare adding elements to the pipeline:%p\n", new_mediamuxer);
	ret = _gst_create_pipeline(new_mediamuxer);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_start(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("__gst_muxer_start making pipeline to playing:%p\n", gst_handle);

	/* set pipeline state to PLAYING */
	MEDIAMUXER_ELEMENT_SET_STATE(GST_ELEMENT_CAST(gst_handle->pipeline),
				GST_STATE_PLAYING);

	MEDIAMUXER_FLEAVE();
	return ret;

STATE_CHANGE_FAILED:
	MX_E("muxer state change failed, returning \n");
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
	/* video */
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
		/* audio */
	case MEDIA_FORMAT_L16:
		break;
	case MEDIA_FORMAT_ALAW:
		break;
	case MEDIA_FORMAT_ULAW:
		break;
	case MEDIA_FORMAT_AMR_NB:
		break;
	case MEDIA_FORMAT_AMR_WB:
		break;
	case MEDIA_FORMAT_G729:
		break;
	case MEDIA_FORMAT_AAC_LC:
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


int _gst_set_caps(MMHandleType pHandle, media_packet_h packet, int track_index)
{
	MEDIAMUXER_FENTER();
	gint ret = MX_ERROR_NONE;
	GstCaps *new_cap;
	media_format_h format;
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;
	media_format_type_e formattype;
	char *codec_data;
	unsigned int codec_data_size;
	mx_gst_track *current = NULL;
	media_format_mimetype_e track_mime;
	media_format_mimetype_e current_mime;

	/* Reach that track index and set the codec data */
	for (current = gst_handle->track_info.track_head; current; current = current->next)
		if (current->track_index == track_index)
			break;

	if ((!current) || (current->track_index != track_index)) {
		ret = MX_ERROR_COMMON_INVALID_ARGUMENT;
		goto ERROR;
	}

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
		/* Following check is safe but not mandatory. */
		if ((current->track_index)%NO_OF_TRACK_TYPES != 1) {
			MX_E("This is not an audio track_index. Track_index is not in 3*n+1 format\n");
			goto ERROR;
		}

		/* return if track_mime is different to current_mime */
		if (media_format_get_audio_info((media_format_h)(current->media_format), &track_mime, NULL, NULL, NULL, NULL)
			== MEDIA_FORMAT_ERROR_NONE) {
			if (media_format_get_audio_info((media_format_h)(format), &current_mime, NULL, NULL, NULL, NULL)
				== MEDIA_FORMAT_ERROR_NONE) {
				if (track_mime != current_mime) {
					MX_E("audio track_mime is not matching with packet mime. returning\n");
					return MX_ERROR_INVALID_ARGUMENT;
				}
			} else {
				MX_E("cant read audio mime in packet. returning\n");
				return MX_ERROR_INVALID_ARGUMENT;
			}
		} else {
			MX_E("cant read audio mime, set during add_track. returning\n");
			return MX_ERROR_INVALID_ARGUMENT;
		}

		if (media_packet_get_extra(packet,
				(void **)&codec_data)) {
			MX_E("media_packet_get_extra call failed\n");
			ret = MX_ERROR_UNKNOWN;
			break;
		}
		codec_data_size = strlen(codec_data) + 1;

		if ((strlen(codec_data)+1) != codec_data_size) {
			MX_E("strlen(codec_data)+1 is not matching with codec_data_size. They are supposed to be equal.\n");
			return MX_ERROR_INVALID_ARGUMENT;
		}
		MX_I("Extracted codec data is =%s size is %d\n", codec_data, codec_data_size);

		if (current->caps == NULL ||
			g_strcmp0(codec_data, current->caps) != 0) {

#ifdef SEND_FULL_CAPS_VIA_CODEC_DATA
			/* Debugging purpose. The whole caps filter can be sent via codec_data */
			new_cap = gst_caps_from_string(codec_data);
			MX_I("codec  cap is=%s\n", codec_data);
			g_object_set(current->appsrc, "caps", new_cap, NULL);
			if (current->caps == NULL) {
				current->caps = (char *)g_malloc(codec_data_size);
				if (current->caps == NULL) {
					MX_E("[%s][%d] memory allocation failed\n", __func__, __LINE__);
					gst_caps_unref(new_cap);
					ret = MX_ERROR_UNKNOWN;
					break;
				}
			}
			g_stpcpy(current->caps, codec_data);
#else
			gchar *caps_string = NULL;
			int channel = 0;
			int samplerate = 0;
			int bit = 0;
			int avg_bps = 0;
			media_format_mimetype_e mimetype = MEDIA_FORMAT_MAX;
			if (media_format_get_audio_info(format,
				&mimetype, &channel, &samplerate,
				&bit, &avg_bps)) {
				MX_E("media_format_get_audio_info call failed\n");
				ret = MX_ERROR_UNKNOWN;
				break;
			}
			if (current->caps == NULL) {
				current->caps = (char *)g_malloc(codec_data_size);
				if (current->caps == NULL) {
					MX_E("[%s][%d]memory allocation failed\n", __func__, __LINE__);
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
			MX_I("New cap set by codec data is = %s\n",
			     caps_string);
			if (caps_string)
				g_free(caps_string);
			g_object_set(current->appsrc, "caps", new_cap, NULL);
			MX_I("copying   current->caps = codec_data\n");
			g_stpcpy(current->caps, codec_data);
#endif
			gst_caps_unref(new_cap);
		}
		break;
	case MEDIA_FORMAT_VIDEO:
		/* Following check is safe but not mandatory. */
		if ((current->track_index)%NO_OF_TRACK_TYPES != 0) {
			MX_E("This is not an video track_index. Video track_index is not in 3*n format\n");
			goto ERROR;
		}

		/* return if track_mime is different to current_mime */
		if (media_format_get_video_info((media_format_h)(current->media_format), &track_mime, NULL, NULL, NULL, NULL)
			== MEDIA_FORMAT_ERROR_NONE) {
			if (media_format_get_video_info((media_format_h)(format), &current_mime, NULL, NULL, NULL, NULL)
				== MEDIA_FORMAT_ERROR_NONE) {
				if (track_mime != current_mime) {
					MX_E("video track_mime is not matching with packet mime. returning\n");
					return MX_ERROR_INVALID_ARGUMENT;
				}
			} else {
				MX_E("cant read video mime. returning\n");
				return MX_ERROR_INVALID_ARGUMENT;
			}
		} else {
			MX_E("cant read video mime in packet. returning\n");
			return MX_ERROR_INVALID_ARGUMENT;
		}

		if (media_packet_get_extra(packet,
			(void **)&codec_data)) {
			MX_E("media_packet_get_extra call failed\n");
			ret = MX_ERROR_UNKNOWN;
			break;
		}
		codec_data_size = strlen(codec_data) + 1;

		if ((strlen(codec_data)+1) != codec_data_size) {
			MX_E("strlen(codec_data)+1 is not matching with codec_data_size. They are supposed to be equal.\n");
			return MX_ERROR_INVALID_ARGUMENT;
		}
		MX_I("codec data is =%s size is %d\n", codec_data, codec_data_size);
		if (current->caps == NULL ||
		    g_strcmp0(codec_data, current->caps) != 0) {
#ifdef SEND_FULL_CAPS_VIA_CODEC_DATA
			/* Debugging purpose. The whole caps filter can be sent via codec_data */
			codec_data_size = strlen(codec_data) + 1;
			MX_I("extracted codec data is =%s\n", codec_data);
			new_cap = gst_caps_from_string(codec_data);
			MX_I("New cap is=%s\n", codec_data);
			g_object_set(current->appsrc, "caps", new_cap, NULL);
			if (current->caps == NULL) {
				current->caps = (char *)g_malloc(codec_data_size);
				if (current->caps == NULL) {
					MX_E("[%s][%d] memory allocation failed\n", __func__, __LINE__);
					gst_caps_unref(new_cap);
					ret = MX_ERROR_UNKNOWN;
					break;
				}
			}
			g_stpcpy(current->caps, codec_data);
#else
			 gchar *caps_string = NULL;
			GValue val = G_VALUE_INIT;
			int numerator = 1;
			int denominator = 1;
			int width = 0;
			int height = 0;
			int avg_bps = 0;
			int max_bps = 0;
			media_format_mimetype_e mimetype = MEDIA_FORMAT_MAX;

			if (media_format_get_video_info(format,
				&mimetype, &width, &height,
				&avg_bps, &max_bps)) {
				MX_E("media_format_get_video_info call failed\n");
				ret = MX_ERROR_UNKNOWN;
				break;
			}
			if (current->caps == NULL) {
				current->caps = (char *)g_malloc(codec_data_size);
				if (current->caps == NULL) {
					MX_E("[%s][%d] memory allocation failed\n", __func__, __LINE__);
					ret = MX_ERROR_UNKNOWN;
					break;
				}
			}
			new_cap = gst_caps_from_string(codec_data);
			MX_I("New cap set by codec data is=%s\n", codec_data);
			if (__gst_codec_specific_caps(new_cap, mimetype)) {
				MX_E("Setting Video caps failed\n");
				gst_caps_unref(new_cap);
				ret = MX_ERROR_UNKNOWN;
				break;
			}
			g_stpcpy(current->caps, codec_data);

			if (media_format_get_video_frame_rate(format, &numerator))
				MX_E("media_format_get_video_info call failed\n");
			g_value_init(&val, GST_TYPE_FRACTION);
			gst_value_set_fraction(&val, numerator, denominator);
			gst_caps_set_value(new_cap, "framerate", &val);
			caps_string = gst_caps_to_string(new_cap);
			MX_I("New cap set by codec data is = %s\n",
			     caps_string);
			if (caps_string)
				g_free(caps_string);
			g_object_set(current->appsrc, "caps", new_cap, NULL);
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
	/* copy data */
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
	/* if (!gst_buffer_map (buffer, &map, GST_MAP_READ)) {
		MX_E("gst_buffer_map failed\n");
		ret = MX_ERROR_UNKNOWN;
		goto ERROR;
	} */
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
	/* TBD: set falgs is not available now in media_packet */
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
	mx_gst_track *current = NULL;
	for (current = gst_handle->track_info.track_head; current; current = current->next)
		if (current->track_index == track_index)
			break;

	if ((!current) || (current->track_index != track_index)) {
		MX_E("No tracks or mismatched track_index = %d\n", track_index);
		if (current)
			MX_E("\ncurrent->track_index=%d\n", current->track_index);
		else if (gst_handle->track_info.track_head) {
			MX_E("\n\ngst_handle->track_info.track_head->track_index=%d\n", gst_handle->track_info.track_head->track_index);
			if (gst_handle->track_info.track_head->next)
				MX_E("ext=%p\tnext->track_index=%d\n", gst_handle->track_info.track_head->next, gst_handle->track_info.track_head->next->track_index);
		} else
			MX_E("\n\n****Head is NULL****\n");
		ret = MX_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}

	if (_gst_set_caps(pHandle, inbuf, track_index) != MX_ERROR_NONE) {
		ret = MX_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	}

	MX_I("Track_index passed = %d, working-with_track_index = %d\n", track_index, current->track_index);

	GstBuffer *gst_inbuf2 = NULL;
	gst_inbuf2 = gst_buffer_new();
	/* ToDo: Add functionality to the following function */
	/* MX_I("\nBefore  buff=%x\n", gst_inbuf2); */
	_gst_copy_media_packet_to_buf(inbuf, gst_inbuf2);

	if (track_index%NO_OF_TRACK_TYPES == 0) {  /* NO_OF_TRACK_TYPES*n for video */
		MX_I("Waiting till start_feed of current video track, index=%d is active\n", current->track_index);
#ifdef ASYCHRONOUS_WRITE
		/* poll now to make it synchronous */
		while (current->start_feed == 0)
			g_usleep(WRITE_POLL_PERIOD);
		MX_I("pushing video\n");

		g_signal_emit_by_name(current->appsrc, "push-buffer", gst_inbuf2, &ret);

#else
		ret = gst_app_src_push_buffer((GstAppSrc *)current->appsrc, gst_inbuf2);
#endif
		MX_I("attempted a vid-buf push\n");
		if (ret != GST_FLOW_OK) {
			/* We got some error, stop sending data */
			MX_E("--video appsrc push failed--\n");
		}
	} else if (track_index%NO_OF_TRACK_TYPES == 1) {	/* NO_OF_TRACK_TYPES*n+1 for audio */
		MX_I(" Waiting till start_feed of current audio track, index=%d is active\n", current->track_index);
#ifdef ASYCHRONOUS_WRITE
		while (current->start_feed == 0)
			g_usleep(WRITE_POLL_PERIOD);
		MX_I("End of sleep, pushing audio\n");
		g_signal_emit_by_name(current->appsrc, "push-buffer", gst_inbuf2, &ret);
#else
		ret = gst_app_src_push_buffer((GstAppSrc *)current->appsrc, gst_inbuf2);
#endif
		MX_I("Attempted a aud-buf push\n");
		if (ret != GST_FLOW_OK) {
			/* We got some error, stop sending data */
			MX_E("--audio appsrc push failed--\n");
		}
	} else {
		MX_E("Unsupported track index=%d. track_index-mod3= %d. Only 0/1/2 track index is vaild\n", track_index, track_index%NO_OF_TRACK_TYPES);
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	/* Unref the buffer, as it is pushed into the appsrc already */
	gst_buffer_unref(gst_inbuf2);
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
	mx_gst_track *current;
	for (current = gst_handle->track_info.track_head; current; current = current->next)
		if (current->track_index == track_index)
			break;

	if (!current || current->track_index != track_index) {
		(!current) ? MX_E("Current is Null") : MX_E("Mismatched between track index[%d]\n", track_index);
		goto ERROR;
	}

	MX_I("__gst_muxer_stop setting eos to sources:%p\n", gst_handle);
	if (gst_handle->pipeline != NULL) {
		if (track_index%NO_OF_TRACK_TYPES == 0) {
			MX_I("\n-----EOS for videoappsrc-----\n");
			gst_app_src_end_of_stream((GstAppSrc *)(current->appsrc));
		} else if (track_index%NO_OF_TRACK_TYPES == 1) {
			MX_I("\n-----EOS for audioappsrc-----\n");
			gst_app_src_end_of_stream((GstAppSrc *)(current->appsrc));
		} else {
			MX_E("Invalid track Index[%d].\n", track_index);
			goto ERROR;
		}
	}
	/* Reset the media_format and track_index to default. */
	if (current) {
		current->media_format = NULL;
		current->track_index = -1;
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

	MX_I("gst_muxer_pause setting pipeline to pause");
	if (gst_element_set_state(gst_handle->pipeline, GST_STATE_PAUSED)
		== GST_STATE_CHANGE_FAILURE) {
		MX_I("Setting pipeline to pause failed");
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_resume(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("gst_muxer_resume setting pipeline back to playing");
	if (gst_element_set_state(gst_handle->pipeline, GST_STATE_PLAYING)
		== GST_STATE_CHANGE_FAILURE) {
		MX_I("Setting pipeline to resume failed");
		ret = MX_ERROR_INVALID_ARGUMENT;
	}
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_stop(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("gst_muxer_stop making pipeline to ready:%p\n", gst_handle);
	/* set pipeline state to READY */
	MEDIAMUXER_ELEMENT_SET_STATE(GST_ELEMENT_CAST(gst_handle->pipeline),
				GST_STATE_READY);
	MEDIAMUXER_FLEAVE();
	return ret;
STATE_CHANGE_FAILED:
	MX_E("muxer state change failed, returning \n");
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

mx_ret_e _gst_destroy_pipeline(mxgst_handle_t *gst_handle)
{
	gint ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	mx_gst_track *current = NULL, *prev;

	/* Clean up nicely */
	gst_element_set_state(gst_handle->pipeline, GST_STATE_NULL);

	/* Free resources & set unused pointers to NULL */
	if (gst_handle->output_uri != NULL)
		gst_handle->output_uri = NULL;

	current = gst_handle->track_info.track_head;
	while (current) {
		prev = current;

		current = current->next; /* Update current */

		/* Free prev & its contents */
		if (prev->media_format)
			prev->media_format = NULL;
		if (prev->caps)
			g_free(prev->caps);
		g_free(prev);
	}

	if (gst_handle->pipeline)
		gst_object_unref(GST_OBJECT(gst_handle->pipeline));

	g_source_remove(gst_handle->bus_watch_id);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int gst_muxer_unprepare(MMHandleType pHandle)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	MX_I("gst_muxer_unprepare setting eos to sources:%p\n", gst_handle);
	ret = _gst_destroy_pipeline(gst_handle);
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
}

int gst_set_error_cb(MMHandleType pHandle, gst_error_cb callback, void* user_data)
{
	MEDIAMUXER_FENTER();
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_CHECK_NULL(pHandle);
	mxgst_handle_t *gst_handle = (mxgst_handle_t *) pHandle;

	if (!gst_handle) {
		MX_E("fail invaild param\n");
		ret = MX_INVALID_ARGUMENT;
		goto ERROR;
	}

	if (gst_handle->user_cb[_GST_EVENT_TYPE_ERROR]) {
		MX_E("Already set mediamuxer_error_cb\n");
		ret = MX_ERROR_INVALID_ARGUMENT;
		goto ERROR;
	} else {
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
