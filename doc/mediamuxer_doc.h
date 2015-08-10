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

#ifndef __TIZEN_MEDIAMUXER_DOC_H__
#define __TIZEN_MEDIAMUXER_DOC_H__

/**
 * @file mediamuxer_doc.h
 * @brief This file contains high level documentation of the CAPI MEDIAMUXER.
 */

/**
 * @ingroup CAPI_MEDIA_FRAMEWORK
 * @defgroup CAPI_MEDIAMUXER_MODULE Media Muxer
 * @brief  The @ref CAPI_MEDIAMUXER_MODULE  APIs provides functions for muxing media data
 *
 * @section CAPI_MEDIAMUXER_MODULE_HEADER Required Header
 *   \#include <mediamuxer.h>
 *
 * @section CAPI_MEDIAMUXER_MODULE_OVERVIEW Overview
 *
 * MEDIAMUXER API set allows :
 * The API set allows one to directly access media muxer on device.
 * Application can create, add relevent media track(s) and write corresponding
 * samples to get muxed media files. mediamuxer takes encoded media as input
 * and gives muxed media in a compatable container format.
 *
 * Typical Call Flow of mediamuxer APIs is:
 * mediamuxer_create()
 * mediamuxer_set_data_sink()
 * mediamuxer_add_track(1)
 * mediamuxer_add_track(2)	[add more tracks, if needed]
 * mediamuxer_prepare()
 * mediamuxer_start()
 * while()
 *	if (is_track(1)_data_available)
 * 		mediamuxer_write_sample(track(1)),
 *	else
 * 		mediamuxer_close_track(1)
 *	if (is_track(2)_data_available)
 * 		mediamuxer_write_sample(track(2))
 *	else
 *		mediamuxer_close_track(2)
 * mediamuxer_stop()
 * mediamuxer_unprepare()
 * mediamuxer_destroy()
 */

#endif  /* __TIZEN_MEDIAMUXER_DOC_H__ */
