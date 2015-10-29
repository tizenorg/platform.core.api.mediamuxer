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
#include <mediamuxer_port_custom.h>

static int custom_muxer_init(MMHandleType * pHandle);
static int custom_muxer_set_data_sink(MMHandleType pHandle, char *uri, mediamuxer_output_format_e format);

/*Media Muxer API common*/
static media_port_muxer_ops def_mux_ops = {
	.n_size = 0,
	.init = custom_muxer_init,
	.set_data_sink = custom_muxer_set_data_sink,
};

int custom_port_register(media_port_muxer_ops * pOps)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MEDIAMUXER_CHECK_NULL(pOps);

	def_mux_ops.n_size = sizeof(def_mux_ops);

	memcpy((char *)pOps + sizeof(pOps->n_size), (char *)&def_mux_ops + sizeof(def_mux_ops.n_size), pOps->n_size - sizeof(pOps->n_size));

	MEDIAMUXER_FLEAVE();
	return ret;
 ERROR:
	ret = MX_ERROR_INVALID_ARGUMENT;
	return ret;
}

static int custom_muxer_init(MMHandleType * pHandle)
{
	int ret = MX_ERROR_NONE;
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return ret;
}

static int custom_muxer_set_data_sink(MMHandleType pHandle, char *uri, mediamuxer_output_format_e format)
{
	MEDIAMUXER_FENTER();
	MX_E("%s:exit: Not implemented\n", __func__);
	MEDIAMUXER_FLEAVE();
	return 0;
}
