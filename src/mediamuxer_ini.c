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

#ifndef __MEDIAMUXER_INI_C__
#define __MEDIAMUXER_INI_C__

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <mm_debug.h>
#include <mm_error.h>
#include "iniparser.h"
#include <mediamuxer_ini.h>
#include <mediamuxer_private.h>
#include <mediamuxer_error.h>

/* macro */
#define MEDIAMUXER_INI_GET_STRING( x_dict, x_item, x_ini, x_default ) \
	do \
	{ \
		gchar* str = iniparser_getstring(x_dict, x_ini, x_default); \
		\
		if ( str &&  \
		     ( strlen( str ) > 0 ) && \
		     ( strlen( str ) < MEDIAMUXER_INI_MAX_STRLEN ) ) \
		{ \
			strcpy ( x_item, str ); \
		} \
		else \
		{ \
			strcpy ( x_item, x_default ); \
		} \
	}while(0)

#define MEDIAMUXER_INI_GET_COLOR( x_dict, x_item, x_ini, x_default ) \
	do \
	{ \
		gchar* str = iniparser_getstring(x_dict, x_ini, x_default); \
		\
		if ( str &&  \
		     ( strlen( str ) > 0 ) && \
		     ( strlen( str ) < MEDIAMUXER_INI_MAX_STRLEN ) ) \
		{ \
			x_item = (guint) strtoul(str, NULL, 16); \
		} \
		else \
		{ \
			x_item = (guint) strtoul(x_default, NULL, 16); \
		} \
	}while(0)

/* x_ini is the list of index to set TRUE at x_list[index] */
#define MEDIAMUXER_INI_GET_BOOLEAN_FROM_LIST( x_dict, x_list, \
                                              x_list_max, x_ini, x_default ) \
do \
{ \
	int index = 0; \
	const char *delimiters = " ,"; \
	char *usr_ptr = NULL; \
	char *token = NULL; \
	gchar temp_arr[MEDIAMUXER_INI_MAX_STRLEN] = {0}; \
	MMMEDIAMUXER_INI_GET_STRING( x_dict, temp_arr, x_ini, x_default); \
	token = strtok_r( temp_arr, delimiters, &usr_ptr ); \
	while (token) \
	{ \
		index = atoi(token); \
		if (index < 0 || index > x_list_max -1) \
		{ \
			MX_W("%d is not valid index\n", index); \
		} \
		else \
		{ \
			x_list[index] = TRUE; \
		} \
		token = strtok_r( NULL, delimiters, &usr_ptr ); \
	} \
}while(0)

/* x_ini is the list of value to be set at x_list[index] */
#define MEDIAMUXER_INI_GET_INT_FROM_LIST( x_dict, x_list, \
                                          x_list_max, x_ini, x_default ) \
do \
{ \
	int index = 0; \
	int value = 0; \
	const char *delimiters = " ,"; \
	char *usr_ptr = NULL; \
	char *token = NULL; \
	gchar temp_arr[MEDIAMUXER_INI_MAX_STRLEN] = {0}; \
	MMMEDIAMUXER_INI_GET_STRING(x_dict, temp_arr, x_ini, x_default); \
	token = strtok_r( temp_arr, delimiters, &usr_ptr ); \
	while (token) \
	{ \
		if ( index > x_list_max -1) \
		{ \
			MX_E("%d is not valid index\n", index); \
			break; \
		} \
		else \
		{ \
			value = atoi(token); \
			x_list[index] = value; \
			index++; \
		} \
		token = strtok_r( NULL, delimiters, &usr_ptr ); \
	} \
}while(0)

/* internal functions, macros here */
#ifdef MEDIAMUXER_DEFAULT_INI
static gboolean _generate_default_ini(void);
#endif

static void _mx_ini_check_ini_status(void);

int mx_ini_load(mx_ini_t *ini)
{
	dictionary *dict = NULL;

	_mx_ini_check_ini_status();

	/* first, try to load existing ini file */
	dict = iniparser_load(MEDIAMUXER_INI_DEFAULT_PATH);

	/* if no file exists. create one with set of default values */
	if (!dict) {
#ifdef MEDIAMUXER_DEFAULT_INI
		MX_L("No inifile found. muxer will create default inifile.\n");
		if (FALSE == _generate_default_ini()) {
			MX_W("Creating default inifile failed. \
			MediaMuxer will use default values.\n");
		} else {
			/* load default ini */
			dict = iniparser_load(MEDIAMUXER_INI_DEFAULT_PATH);
		}
#else
		MX_L("No ini file found. \n");
		return MM_ERROR_FILE_NOT_FOUND;
#endif
	}

	/* get ini values */
	memset(ini, 0, sizeof(mx_ini_t));

	if (dict) {		/* if dict is available */
		/* general */
		MEDIAMUXER_INI_GET_STRING(dict, ini->port_name,
		                          "port_in_use:mediamuxer_port",
		                          DEFAULT_PORT);
	} else {
		/* if dict is not available just fill
		   the structure with default value */
		MX_W("failed to load ini. using hardcoded default\n");
		strncpy(ini->port_name, DEFAULT_PORT,
		        MEDIAMUXER_INI_MAX_STRLEN - 1);
	}

	if (0 == strcmp(ini->port_name, "GST_PORT")) {
		ini->port_type = GST_PORT;
	} else if (0 == strcmp(ini->port_name, "FFMPEG_PORT")) {
		ini->port_type = FFMPEG_PORT;
	} else if (0 == strcmp(ini->port_name, "CUSTOM_PORT")) {
		ini->port_type = CUSTOM_PORT;
	} else {
		MX_E("Invalid port is set to [%s] [%d]\n", ini->port_name,
		     ini->port_type);
		goto ERROR;
	}
	MX_L("The port is set to [%s] [%d]\n", ini->port_name, ini->port_type);

	/* free dict as we got our own structure */
	iniparser_freedict(dict);

	/* dump structure */
	MX_L("muxer settings -----------------------------------\n");

	/* general */
	MX_L("port_name: %s\n", ini->port_name);
	MX_L("port_type : %d\n", ini->port_type);

	return MM_ERROR_NONE;
ERROR:
	return MX_COURRPTED_INI;
}

static void _mx_ini_check_ini_status(void)
{
	struct stat ini_buff;

	if (g_stat(MEDIAMUXER_INI_DEFAULT_PATH, &ini_buff) < 0) {
		MX_W("failed to get muxer ini status\n");
	} else {
		if (ini_buff.st_size < 5) {
			MX_W("muxer.ini file size=%d, Corrupted! So, Removed\n",
			     (int)ini_buff.st_size);

			if (g_remove(MEDIAMUXER_INI_DEFAULT_PATH) == -1)
				MX_E("failed to delete corrupted ini");
		}
	}
}

#ifdef MEDIAMUXER_DEFAULT_INI
static gboolean _generate_default_ini(void)
{
	FILE *fp = NULL;
	gchar *default_ini = MEDIAMUXER_DEFAULT_INI;

	/* create new file */
	fp = fopen(MEDIAMUXER_INI_DEFAULT_PATH, "wt");

	if (!fp)
		return FALSE;

	/* writing default ini file */
	if (strlen(default_ini) !=
	    fwrite(default_ini, 1, strlen(default_ini), fp)) {
		fclose(fp);
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}
#endif

#endif /* #ifdef _MEDIAMUXER_INI_C_ */
