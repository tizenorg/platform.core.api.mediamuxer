/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

/*
 * @file mediamuxer_port_gst.h
 * @brief decleartions related to gst port content of mediamuxer
 * Contact: Satheesan E N <satheesan.en@samsung.com>,
 EDIT HISTORY FOR MODULE
 This section contains comments describing changes made to the module.
 Notice that changes are listed in reverse chronological order.
 when            who                                              what, where, why
 ---------   ------------------------   ----------------------------------------------------------
 03/2015      <kd.mahesh@samsung.com>                    Modified
 */

#ifndef __TIZEN_MEDIAMUXER_PORT_GST_H__
#define __TIZEN_MEDIAMUXER_PORT_GST_H__

#include <tizen.h>
#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video-format.h>

#define MEDIAMUXER_ELEMENT_SET_STATE( x_element, x_state ) \
	MX_I("setting state [%s:%d] to [%s]\n", #x_state, x_state, GST_ELEMENT_NAME( x_element ) ); \
	if ( GST_STATE_CHANGE_FAILURE == gst_element_set_state ( x_element, x_state) ) \
	{ \
		MX_E("failed to set state %s to %s\n", #x_state, GST_ELEMENT_NAME( x_element )); \
		goto STATE_CHANGE_FAILED; \
	}

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	_GST_EVENT_TYPE_COMPLETE,
	_GST_EVENT_TYPE_ERROR,
	_GST_EVENT_TYPE_EOS,
	_GST_EVENT_TYPE_NUM
} _gst_event_e;

/* GST port Private data */
typedef struct _mx_gst_track {
	void *media_format;
	char *caps;
	int track_index;
	int start_feed;
	int stop_feed;
} mx_gst_track;

typedef struct _mxgst_handle_t {
	void *hmux;			/**< mux handle */
	int state;			/**< mx current state */
	bool is_prepared;

	mx_gst_track video_track;
	mx_gst_track audio_track;

	mediamuxer_output_format_e muxed_format;
	char *output_uri;
	bool eos_flg;
	guint bus_watch_id;
	GstElement *pipeline;
	GstElement *audio_appsrc;		/* Input audio buffers to be muxed */
	GstElement *video_appsrc;		/* Input video buffers to be muxed */
	GstElement *audioparse;
	GstElement *videoparse;
	GstElement *muxer;
	GstElement *sink;			/* sink for the muxed output */
	char *saveLocation;			/* Save path for muxed data */
	void* user_cb[_GST_EVENT_TYPE_NUM];	/* for user cb */
	void* user_data[_GST_EVENT_TYPE_NUM];
} mxgst_handle_t;

/**
 * @brief Called when the error has occured.
 * @remarks It will be invoked when the error has occured.
 * @since_tizen 3.0
 * @param[in] error_code  The error code
 * @param[in] user_data  The user data passed from the callback registration function
 * @pre It will be invoked when the error has occured if user register this callback using mediamuxer_set_error_cb().
 * @see mediamuxer_set_error_cb()
 * @see mediamuxer_unset_error_cb()
 */
typedef void (*gst_error_cb)(mediamuxer_error_e error, void *user_data);

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIAMUXER_PORT_GST_H__ */
