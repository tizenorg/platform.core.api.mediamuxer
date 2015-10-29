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

#ifndef __TIZEN_MEDIAMUXER_PRIVATE_H__
#define __TIZEN_MEDIAMUXER_PRIVATE_H__
#include <mediamuxer.h>
#include <mediamuxer_port.h>
#include <mediamuxer_ini.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_MEDIAMUXER"

#define MUXER_CHECK_CONDITION(condition,error,msg)     \
	do \
	{ \
		if (condition) {} else \
			{ MX_E("[%s] %s(0x%08x)",__FUNCTION__, msg,error); return error;}; \
	}while(0)

#define MUXER_INSTANCE_CHECK(muxer)   \
	MUXER_CHECK_CONDITION(muxer != NULL, \
	                      MEDIAMUXER_ERROR_INVALID_PARAMETER,"MUXER_ERROR_INVALID_PARAMETER")

#define MUXER_STATE_CHECK(muxer,expected_state)       \
	MUXER_CHECK_CONDITION(muxer->state == expected_state, \
	                      MEDIAMUXER_ERROR_INVALID_STATE,"MUXER_ERROR_INVALID_STATE")

#define MUXER_NULL_ARG_CHECK(arg)      \
	MUXER_CHECK_CONDITION(arg != NULL,MEDIAMUXER_ERROR_INVALID_PARAMETER, \
	                      "MUXER_ERROR_INVALID_PARAMETER")

/**
 * @brief Enumerations of media muxer state
 * @since_tizen 3.0
 */

	int _convert_error_code(int code, char *func_name);

	typedef enum {
		MEDIAMUXER_EVENT_TYPE_PREPARE,
		MEDIAMUXER_EVENT_TYPE_COMPLETE,
		MEDIAMUXER_EVENT_TYPE_INTERRUPT,
		MEDIAMUXER_EVENT_TYPE_ERROR,
		MEDIAMUXER_EVENT_TYPE_BUFFERING,
		MEDIAMUXER_EVENT_TYPE_SUBTITLE,
		MEDIAMUXER_EVENT_TYPE_CLOSED_CAPTION,
		MEDIAMUXER_EVENT_TYPE_CAPTURE,
		MEDIAMUXER_EVENT_TYPE_SEEK,
		MEDIAMUXER_EVENT_TYPE_VIDEO_FRAME,
		MEDIAMUXER_EVENT_TYPE_AUDIO_FRAME,
		MEDIAMUXER_EVENT_TYPE_VIDEO_FRAME_RENDER_ERROR,
		MEDIAMUXER_EVENT_TYPE_PD,
		MEDIAMUXER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT,
		MEDIAMUXER_EVENT_TYPE_SUPPORTED_AUDIO_EFFECT_PRESET,
		MEDIAMUXER_EVENT_TYPE_MISSED_PLUGIN,
		MEDIAMUXER_EVENT_TYPE_IMAGE_BUFFER,
		MEDIAMUXER_EVENT_TYPE_OTHERS,
		MEDIAMUXER_EVENT_TYPE_NUM
	} mediamuxer_event_e;

	typedef struct _mediamuxer_s {
		MMHandleType mx_handle;
		const void *user_cb[MEDIAMUXER_EVENT_TYPE_NUM];
		void *user_data[MEDIAMUXER_EVENT_TYPE_NUM];
		void *display_handle;
		int state;
		bool is_set_pixmap_cb;
		bool is_stopped;
		bool is_display_visible;
		bool is_progressive_download;
		pthread_t prepare_async_thread;
		mediamuxer_error_cb error_cb;
		void *error_cb_userdata;
		mediamuxer_state_e muxer_state;
	} mediamuxer_s;

	typedef struct {
		/* initialize values */
		media_port_muxer_ops *muxer_ops;
		/* initialize values */
		mx_ini_t ini;
		/* port specific handle */
		MMHandleType mxport_handle;
	} mx_handle_t;

#ifdef __cplusplus
}
#endif
#endif							/* __TIZEN_MEDIAMUXER_PRIVATE_H__ */
