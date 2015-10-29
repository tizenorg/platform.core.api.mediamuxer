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

#include <string.h>
#include <mm_types.h>
#include <mm_message.h>
#include <mm_debug.h>
#include <mediamuxer.h>
#include <mediamuxer_ini.h>
#include <mediamuxer_error.h>
#include <mediamuxer_private.h>
#include <mediamuxer_port.h>

/* function type */
extern int gst_port_register(media_port_muxer_ops * pOps);
extern int ffmpeg_port_register(media_port_muxer_ops * pOps);
extern int custom_port_register(media_port_muxer_ops * pOps);

/*
  * Sequence of functions should be same as the port enumeration "port_mode"
  * in mx_ini.h file
  */
typedef int (*register_port) (media_port_muxer_ops *);
register_port register_port_func[] = {
	&gst_port_register,
	&ffmpeg_port_register,
	&custom_port_register
};

int mx_create(MMHandleType * muxer)
{
	int result = MX_ERROR_NONE;
	media_port_muxer_ops *pOps = NULL;
	mx_handle_t *new_muxer = NULL;
	MEDIAMUXER_FENTER();
	new_muxer = (mx_handle_t *) g_malloc(sizeof(mx_handle_t));
	MEDIAMUXER_CHECK_NULL(new_muxer);
	memset(new_muxer, 0, sizeof(mx_handle_t));

	/* alloc ops structure */
	pOps = (media_port_muxer_ops *) g_malloc(sizeof(media_port_muxer_ops));
	MEDIAMUXER_CHECK_NULL(pOps);

	new_muxer->muxer_ops = pOps;
	MX_I("mx_create allocating new_muxer->muxer_ops %p:\n", new_muxer->muxer_ops);
	pOps->n_size = sizeof(media_port_muxer_ops);
	/* load ini files */
	result = mx_ini_load(&new_muxer->ini);
	MEDIAMUXER_CHECK_SET_AND_PRINT(result, MX_ERROR_NONE, result, MX_COURRPTED_INI, "can't load ini");

	register_port_func[new_muxer->ini.port_type] (pOps);
	result = pOps->init(&new_muxer->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(result, MX_ERROR_NONE, result, MX_NOT_INITIALIZED, "mx_create failed");
	*muxer = (MMHandleType) new_muxer;
	MEDIAMUXER_FLEAVE();
	return result;
 ERROR:
	*muxer = (MMHandleType) 0;
	if (pOps)
		g_free(pOps);
	if (new_muxer)
		g_free(new_muxer);
	MEDIAMUXER_FLEAVE();
	return result;
}

int mx_set_data_sink(MMHandleType mediamuxer, char *uri, mediamuxer_output_format_e format)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) mediamuxer;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->set_data_sink(mx_handle->mxport_handle, uri, format);
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_add_track(MMHandleType mediamuxer, media_format_h media_format, int *track_index)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) mediamuxer;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->add_track(mx_handle->mxport_handle, media_format, track_index);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while adding track");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_prepare(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->prepare(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while preparing");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_start(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->start(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while starting");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_write_sample(MMHandleType mediamuxer, int track_index, media_packet_h inbuf)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) mediamuxer;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->write_sample(mx_handle->mxport_handle, track_index, inbuf);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while writing sample");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_close_track(MMHandleType mediamuxer, int track_index)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->close_track(mx_handle->mxport_handle, track_index);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while closing track");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_pause(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->pause(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while pausing");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_resume(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->resume(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while resuming");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_stop(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->stop(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while stopping");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_unprepare(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) (mediamuxer);
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->unprepare(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while destroying");
	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_destroy(MMHandleType mediamuxer)
{
	int ret = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) mediamuxer;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	ret = pOps->destroy(mx_handle->mxport_handle);
	MEDIAMUXER_CHECK_SET_AND_PRINT(ret, MX_ERROR_NONE, ret, MX_ERROR, "error while destroying");

	/* free mediamuxer structure */
	if (mx_handle) {
		if (mx_handle->muxer_ops) {
			MX_I("mx_destroy deallocating mx_handle->muxer_ops %p:\n", mx_handle->muxer_ops);
			g_free((void *)(mx_handle->muxer_ops));
		}
		MX_I("mx_destroy deallocating mx_handle %p:\n", mx_handle);
		g_free((void *)mx_handle);
		mx_handle = NULL;
	}
 ERROR:
	MEDIAMUXER_FLEAVE();
	return ret;
}

int mx_set_error_cb(MMHandleType muxer, mediamuxer_error_cb callback, void *user_data)
{
	MEDIAMUXER_FENTER();
	int result = MX_ERROR_NONE;
	mx_handle_t *mx_handle = (mx_handle_t *) muxer;
	MEDIAMUXER_CHECK_NULL(mx_handle);
	media_port_muxer_ops *pOps = mx_handle->muxer_ops;
	MEDIAMUXER_CHECK_NULL(pOps);
	result = pOps->set_error_cb(mx_handle->mxport_handle, callback, user_data);
	MEDIAMUXER_CHECK_SET_AND_PRINT(result, MX_ERROR_NONE, result, MX_ERROR, "error while setting error call back");
	MEDIAMUXER_FLEAVE();
	return result;
 ERROR:
	result = MX_ERROR_INVALID_ARGUMENT;
	MEDIAMUXER_FLEAVE();
	return result;
}
