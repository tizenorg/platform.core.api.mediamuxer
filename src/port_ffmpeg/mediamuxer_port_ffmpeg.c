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
#include "mediamuxer_port.h"
#include <mediamuxer_error.h>
#include "mm_debug.h"
#include "mediamuxer_private.h"
#include "mediamuxer_port_ffmpeg.h"

static int ffmpeg_muxer_init(MMHandleType *pHandle);
static int ffmpeg_muxer_set_data_sink(MMHandleType pHandle, char *uri, mediamuxer_output_format_e format);
static int ffmpeg_muxer_get_data_count(MMHandleType pHandle, int *count);
static int ffmpeg_muxer_get_track_info(MMHandleType pHandle, void *info);
static int ffmpeg_muxer_set_track(MMHandleType pHandle, int track);
static int ffmpeg_muxer_get_data(MMHandleType pHandle, char *buffer);

/*Media Muxer API common*/
static media_port_muxer_ops def_mux_ops = {
	.n_size = 0,
	.init = ffmpeg_muxer_init,
	.set_data_sink = ffmpeg_muxer_set_data_sink,
	.get_data_count = ffmpeg_muxer_get_data_count,
	.get_track_info = ffmpeg_muxer_get_track_info,
	.set_track = ffmpeg_muxer_set_track,
	.get_data = ffmpeg_muxer_get_data,
};

int ffmpeg_port_register(media_port_muxer_ops *pOps)
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
	return ret;
}

static int ffmpeg_muxer_init(MMHandleType *pHandle)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int ffmpeg_muxer_set_data_sink(MMHandleType pHandle, char *uri, mediamuxer_output_format_e format)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int ffmpeg_muxer_get_data_count(MMHandleType pHandle, int *count)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int ffmpeg_muxer_get_track_info(MMHandleType pHandle, void *info)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int ffmpeg_muxer_set_track(MMHandleType pHandle, int track)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int ffmpeg_muxer_get_data(MMHandleType pHandle, char *buffer)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}
