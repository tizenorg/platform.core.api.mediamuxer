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

#ifndef __TIZEN_MEDIAMUXER_UTIL_H__
#define __TIZEN_MEDIAMUXER_UTIL_H__

#include <glib.h>
#include <mediamuxer_ini.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define PRINT_ON_CONSOLE
#ifdef PRINT_ON_CONSOLE
#include <stdlib.h>
#include <stdio.h>
#define PRINT_F          g_print
#define MX_FENTER();     PRINT_F("function:[%s] ENTER\n",__func__);
#define MX_FLEAVE();     PRINT_F("function [%s] LEAVE\n",__func__);
#define MX_C             PRINT_F
#define MX_E             PRINT_F
#define MX_W             PRINT_F
#define MX_I             PRINT_F
#define MX_L             PRINT_F
#define MX_V             PRINT_F
#else
#include <stdlib.h>
#include <stdio.h>
#define MX_FENTER();     LOGI("function:[%s] ENTER\n",__func__);
#define MX_FLEAVE();     LOGI("function [%s] LEAVE\n",__func__);
#define MX_C             LOGE	/*MMF_DEBUG_LEVEL_0 */
#define MX_E             LOGE	/*MMF_DEBUG_LEVEL_1 */
#define MX_W             LOGW	/*MMF_DEBUG_LEVEL_2 */
#define MX_I             LOGI	/*MMF_DEBUG_LEVEL_3 */
#define MX_L             LOGI	/*MMF_DEBUG_LEVEL_4 */
#define MX_V             LOGV	/*MMF_DEBUG_LEVEL_5 */
#define MX_F             LOGF	/*MMF_DEBUG_LEVEL_6 */
#endif

/* general */
#define MEDIAMUXER_FREEIF(x) \
	do \
	{ \
		if ( x ) \
			g_free( x ); \
		x = NULL; \
	}while(0)

#if 1
#define MEDIAMUXER_FENTER();              MX_FENTER();
#define MEDIAMUXER_FLEAVE();              MX_FLEAVE();
#else
#define MEDIAMUXER_FENTER();
#define MEDIAMUXER_FLEAVE();
#endif

#define MEDIAMUXER_CHECK_NULL( x_var ) \
	do \
	{ \
		if ( ! x_var ) \
		{ \
			MX_E("[%s] is NULL\n", #x_var ); \
			goto ERROR; \
		} \
	} while (0)
#define MEDIAMUXER_CHECK_SET_AND_PRINT( x_var, x_cond, ret, ret_val, err_text )\
	do \
	{ \
		if ( x_var != x_cond ) \
		{ \
			ret = ret_val; \
			MX_E("%s\n", #err_text ); \
			goto ERROR; \
		} \
	} while (0)
#ifdef __cplusplus
}
#endif
#endif							/* __TIZEN_MEDIAMUXER_UTIL_H__ */
