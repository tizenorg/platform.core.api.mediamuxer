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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mediamuxer_port.h>
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <dlog.h>

#ifndef USE_TASK_QUEUE
#define USE_TASK_QUEUE
#endif

/*
* Public Implementation
*/
static gboolean _mediamuxer_error_cb(mediamuxer_error_e error, void *user_data);

int mediamuxer_create(mediamuxer_h *muxer)
{
	MX_I("mediamuxer_create\n");
	mediamuxer_error_e ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);

	mediamuxer_s *handle;
	handle = (mediamuxer_s *) malloc(sizeof(mediamuxer_s));
	if (handle != NULL) {
		memset(handle, 0, sizeof(mediamuxer_s));
		handle->muxer_state = MEDIAMUXER_STATE_NONE;
	} else {
		MX_E("[CoreAPI][%s] MUXER_ERROR_OUT_OF_MEMORY(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_OUT_OF_MEMORY);
		return MEDIAMUXER_ERROR_OUT_OF_MEMORY;
	}

	ret = mx_create(&handle->mx_handle);
	if (ret != MEDIAMUXER_ERROR_NONE) {
		MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
		free(handle);
		handle = NULL;
		return MEDIAMUXER_ERROR_INVALID_OPERATION;
	} else {
		*muxer = (mediamuxer_h) handle;
		handle->is_stopped = false;
		MX_I("[CoreAPI][%s] new handle : %p", __FUNCTION__, *muxer);

		/* set callback */
		mx_set_error_cb(handle->mx_handle, (mediamuxer_error_cb)_mediamuxer_error_cb, handle);
		if (ret == MEDIAMUXER_ERROR_NONE)
			handle->muxer_state = MEDIAMUXER_STATE_IDLE;
		return MEDIAMUXER_ERROR_NONE;
	}
}

int mediamuxer_set_data_sink(mediamuxer_h muxer, char *path, mediamuxer_output_format_e format)
{
	MX_I("mediamuxer_set_data_sink\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	mediamuxer_s *handle;
	MUXER_INSTANCE_CHECK(muxer);
	handle = (mediamuxer_s *)(muxer);

	if (path == NULL) {
		MX_I("Invalid uri");
		handle->muxer_state = MEDIAMUXER_STATE_NONE;
		return MEDIAMUXER_ERROR_INVALID_PATH;
	}

	if (format != MEDIAMUXER_CONTAINER_FORMAT_MP4
		&& format != MEDIAMUXER_CONTAINER_FORMAT_3GP
		&& format != MEDIAMUXER_CONTAINER_FORMAT_WAV
		&& format != MEDIAMUXER_CONTAINER_FORMAT_AMR_NB
		&& format != MEDIAMUXER_CONTAINER_FORMAT_AMR_WB) {
		MX_E("Unsupported Container format: %d \n", format);
		handle->muxer_state = MEDIAMUXER_STATE_NONE;
		return MEDIAMUXER_ERROR_INVALID_PARAMETER;
	}
	if (handle->muxer_state == MEDIAMUXER_STATE_IDLE) {
		ret = mx_set_data_sink(handle->mx_handle, path, format);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			handle->muxer_state = MEDIAMUXER_STATE_NONE;
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] set_data_sink successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
			__FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		handle->muxer_state = MEDIAMUXER_STATE_NONE;
		ret = MEDIAMUXER_ERROR_INVALID_STATE;
	}
	return ret;
}

int mediamuxer_add_track(mediamuxer_h muxer, media_format_h media_format, int *track_index)
{
	MX_I("mediamuxer_add_track\n");
	mediamuxer_error_e ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle;
	handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_IDLE) {
		ret = mx_add_track(handle->mx_handle, media_format, track_index);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] add_track handle : %p", __FUNCTION__,
			     handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		ret = MEDIAMUXER_ERROR_INVALID_STATE;
	}
	return ret;
}

int mediamuxer_prepare(mediamuxer_h muxer)
{
	MX_I("mediamuxer_prepare\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_IDLE) {
		ret = mx_prepare(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] prepare successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_READY;
	return ret;
}

int mediamuxer_start(mediamuxer_h muxer)
{
	MX_I("mediamuxer_start\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_READY) {
		ret = mx_start(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] start successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_MUXING;
	return ret;
}

int mediamuxer_write_sample(mediamuxer_h muxer, int track_index, media_packet_h inbuf)
{
	MX_I("mediamuxer_write_sample\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (track_index < 0 || inbuf == NULL)
		return MEDIAMUXER_ERROR_INVALID_PARAMETER;
	if (handle->muxer_state == MEDIAMUXER_STATE_READY
		|| handle->muxer_state == MEDIAMUXER_STATE_MUXING) {
		ret = mx_write_sample(handle->mx_handle, track_index, inbuf);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E
			("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)",
			 __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] write_sample successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_MUXING;
	return ret;
}

int mediamuxer_close_track(mediamuxer_h muxer, int track_index)
{
	MX_I("mediamuxer_close_track\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (track_index < 0)
		return MEDIAMUXER_ERROR_INVALID_PARAMETER;
	if (handle->muxer_state == MEDIAMUXER_STATE_MUXING ||
		handle->muxer_state == MEDIAMUXER_STATE_IDLE ||
		handle->muxer_state == MEDIAMUXER_STATE_READY) {
		ret = mx_close_track(handle->mx_handle, track_index);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E
			("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)",
			 __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] close  successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		ret = MEDIAMUXER_ERROR_INVALID_STATE;
	}
	return ret;
}

int mediamuxer_pause(mediamuxer_h muxer)
{
	MX_I("mediamuxer_pause\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);

	if (handle->muxer_state == MEDIAMUXER_STATE_MUXING) {
		ret = mx_pause(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E
			("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)",
			 __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] pause successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_PAUSED;
	return ret;
}

int mediamuxer_resume(mediamuxer_h muxer)
{
	MX_I("mediamuxer_resume\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_PAUSED) {
		ret = mx_resume(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E
			("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)",
			 __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] resume successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_MUXING;
	return ret;
}

int mediamuxer_stop(mediamuxer_h muxer)
{
	MX_I("mediamuxer_stop\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_MUXING
		|| handle->muxer_state == MEDIAMUXER_STATE_PAUSED) {
		ret = mx_stop(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] stop successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_READY;
	return ret;
}

int mediamuxer_unprepare(mediamuxer_h muxer)
{
	MX_I("mediamuxer_unprepare\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_READY
		|| handle->muxer_state == MEDIAMUXER_STATE_MUXING
		|| handle->muxer_state == MEDIAMUXER_STATE_PAUSED) {
		ret = mx_unprepare(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E
			("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)",
			 __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] unprepare successful, handle : %p",
			     __FUNCTION__, handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_IDLE;
	return ret;
}

int mediamuxer_destroy(mediamuxer_h muxer)
{
	MX_I("mediamuxer_destroy\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle;
	handle = (mediamuxer_s *)(muxer);
	if (handle->muxer_state == MEDIAMUXER_STATE_IDLE) {
		ret = mx_destroy(handle->mx_handle);
		if (ret != MEDIAMUXER_ERROR_NONE) {
			MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
			ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
		} else {
			MX_I("[CoreAPI][%s] destroy handle : %p", __FUNCTION__,
			     handle);
		}
	} else {
		MX_E("[CoreAPI][%s] MEDIAMUXER_ERROR_INVALID_STATE(0x%08x)\n",
		     __FUNCTION__, MEDIAMUXER_ERROR_INVALID_STATE);
		return MEDIAMUXER_ERROR_INVALID_STATE;
	}
	if (ret == MEDIAMUXER_ERROR_NONE)
		handle->muxer_state = MEDIAMUXER_STATE_NONE;
	return ret;
}

int mediamuxer_get_state(mediamuxer_h muxer, mediamuxer_state_e *state)
{
	MX_I("mediamuxer_get_state\n");
	int ret = MEDIAMUXER_ERROR_NONE;
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle = (mediamuxer_s *)(muxer);
	if (state != NULL) {
		*state = handle->muxer_state;
	} else {
		MX_E("[CoreAPI][%s] MUXER_ERROR_INVALID_OPERATION(0x%08x)\n",
			__FUNCTION__, MEDIAMUXER_ERROR_INVALID_OPERATION);
		ret = MEDIAMUXER_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mediamuxer_set_error_cb(mediamuxer_h muxer, mediamuxer_error_cb callback, void* user_data)
{
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle;
	handle = (mediamuxer_s *)(muxer);

	handle->error_cb = callback;
	handle->error_cb_userdata = user_data;
	MX_I("set error_cb(%p)", callback);
	return MEDIAMUXER_ERROR_NONE;
}

int mediamuxer_unset_error_cb(mediamuxer_h muxer)
{
	MUXER_INSTANCE_CHECK(muxer);
	mediamuxer_s *handle;
	handle = (mediamuxer_s *)(muxer);

	handle->error_cb = NULL;
	handle->error_cb_userdata = NULL;
	MX_I("mediamuxer_unset_error_cb\n");
	return MEDIAMUXER_ERROR_NONE;
}

static gboolean _mediamuxer_error_cb(mediamuxer_error_e error, void *user_data)
{
	if (user_data == NULL) {
		MX_I("_mediamuxer_error_cb: ERROR %d to report. But call back is not set\n", error);
		return 0;
	}
	mediamuxer_s * handle = (mediamuxer_s *) user_data;

	if (handle->error_cb)
		((mediamuxer_error_cb)handle->error_cb)(error, handle->error_cb_userdata);
	else
		MX_I("_mediamuxer_error_cb: ERROR %d to report. But call back is not set\n", error);

	return 0;
}
