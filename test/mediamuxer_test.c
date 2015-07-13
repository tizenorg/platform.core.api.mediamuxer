/*
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
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
 * @file mediamuxer_test.c
 * @brief contain the test suite of mediamuxer
 * Contact: Satheesan E N <satheesan.en@samsung.com>,
   EDIT HISTORY FOR MODULE
    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order.
    when            who                            what, where, why
    ---------   ------------------------   ------------------------------------
    03/2015      <kd.mahesh@samsung.com>              Modified
 */

/*=============================================================================
|                                                                              |
|  INCLUDE FILES                                                               |
|                                                                              |
==============================================================================*/
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mediamuxer_port.h"
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <media_packet_internal.h>
/* Read encoded medial files locally: encoded A/V files along with info & caps */
#define H264_FILE video_data
#define H264_INFO video_extra_info
#define AAC_FILE audio_data
#define AAC_INFO  audio_extra_info

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define MAX_STRING_LEN 100
#define PACKAGE "mediamuxer_test"

mediamuxer_h myMuxer = 0;
media_format_h media_format = NULL;
media_format_h media_format_a = NULL;

bool aud_eos = 0;
bool vid_eos = 0;
char audio_extra_info[1000];
char audio_data[1000];
char video_extra_info[1000];
char video_data[1000];

int test_mediamuxer_create()
{
	g_print("test_mediamuxer_create\n");
	g_print("%p", myMuxer);

	if (mediamuxer_create(&myMuxer)
	    != MEDIAMUXER_ERROR_NONE) {
		g_print("mediamuxer create is failed\n");
	}
	g_print("\n Muxer->mx_handle created successfully with address=%p",
	        (void *)((mediamuxer_s *) myMuxer)->mx_handle);
	g_print("\n Muxer handle created successfully with address=%p",
	        myMuxer);

	return 0;
}

int test_mediamuxer_set_data_sink()
{
	char *op_uri = "MyTest.mp4";

	/* Set data source after creating */
	mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_MP4);
	return 0;
}

int test_mediamuxer_destroy()
{
	int ret = 0;
	g_print("test_mediamuxer_destroy\n");
	ret = mediamuxer_destroy(myMuxer);
	myMuxer = NULL;
	g_print("\nDestroy operation returned: %d\n", ret);
	return ret;
}

int test_mediamuxer_start()
{
	g_print("mediamuxer_start completed \n");
	mediamuxer_start(myMuxer);
	return 0;
}

int test_mediamuxer_add_track_video()
{
	int track_index_vid = -1;
	media_format_mimetype_e mimetype;
	int width;
	int height;
	int avg_bps;
	int max_bps;

	g_print("test_mediamuxer_add_track_video\n");
	media_format_create(&media_format);

	/* MEDIA_FORMAT_H264_SP  MEDIA_FORMAT_H264_MP  MEDIA_FORMAT_H264_HP */
	media_format_set_video_mime(media_format, MEDIA_FORMAT_H264_SP);

	media_format_set_video_width(media_format, 640);
	media_format_set_video_height(media_format, 480);
	media_format_set_video_avg_bps(media_format, 10);
	media_format_set_video_max_bps(media_format, 10);

	media_format_get_video_info(media_format, &mimetype, &width, &height, &avg_bps, &max_bps);

	g_print("\n Video Mime trying to set: %x   %x\n", (int)(mimetype), (int)(MEDIA_FORMAT_H264_SP));
	g_print("\n Video param trying to set: (width, height, avg_bps, max_bps): %d %d %d %d  \n",
	        width, height, avg_bps, max_bps);

	/* To add video track */
	mediamuxer_add_track(myMuxer, media_format, &track_index_vid);
	g_print("audio track index returned is: %d", track_index_vid);
	return 0;
}

int test_mediamuxer_add_track_audio()
{
	int track_index_aud = -1;
	media_format_mimetype_e mimetype;
	int channel;
	int samplerate;
	int bit;
	int avg_bps;

	g_print("test_mediamuxer_add_track\n");
	media_format_create(&media_format_a);

	/* MEDIA_FORMAT_AAC  MEDIA_FORMAT_AAC_LC  MEDIA_FORMAT_AAC_HE  MEDIA_FORMAT_AAC_HE_PS */
	if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AAC) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
		g_print("Problem during media_format_set_audio_mime operation");

	if (media_format_set_audio_channel(media_format_a, 2) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
		g_print("Problem during media_format_set_audio_channel operation");
	media_format_set_audio_samplerate(media_format_a, 44000);
	media_format_set_audio_bit(media_format_a, 1);
	media_format_set_audio_avg_bps(media_format_a, 10);
	media_format_set_audio_aac_type(media_format_a, true);

	media_format_get_audio_info(media_format_a, &mimetype, &channel, &samplerate, &bit, &avg_bps);

	g_print("\n Audio Mime trying to set: %x   %x\n", (int)(mimetype), (int)(MEDIA_FORMAT_AAC));
	g_print("\n Audio Param trying to set: (ch, samplert, bt, avg_bps) %d %d %d %d \n",
	        channel, samplerate, bit, avg_bps);

	/* To add audio track */
	mediamuxer_add_track(myMuxer, media_format_a, &track_index_aud);
	g_print("track index returned is: %d", track_index_aud);
	return 0;
}

void app_err_cb(mediamuxer_error_e error, void *user_data)
{
	printf("Got Error %d from mediamuxer\n",error);
}

int test_mediamuxer_set_error_cb()
{
	int ret = 0;
	g_print("test_mediamuxer_set_error_cb\n");
	ret = mediamuxer_set_error_cb(myMuxer, app_err_cb, myMuxer);
	return ret;
}

void *_write_video_data()
{
	FILE *pvFile;
	FILE *pvFileInfo;
	unsigned int size;
	unsigned int vsize;
	unsigned int is_video_readable = 1;
	unsigned int is_video_pts_readable;
	unsigned int is_video_dts_readable;
	unsigned int is_video_duration_readable;
	unsigned int is_video_flag_readable;
	unsigned int is_video_key_readable;
	unsigned long long int pts_vid;
	unsigned long long int dts_vid;
	unsigned long long int duration_vid;
	int flg_vid;
	int *status = (int *)g_malloc(sizeof(int) * 1);
	*status = -1;
	int track_index_vid = 1; /* track_index=2 for video */
	int vcount = 0;
	guint8 *ptr_vid;
	int key_vid;
	char *vid_caps;
	int ret_scan;
	media_packet_h vid_pkt;
	media_format_h vidfmt;
	unsigned int vcap_size;

	pvFile = fopen(H264_FILE, "rb");
	pvFileInfo = fopen(H264_INFO, "rt");

	if (pvFile == NULL || pvFileInfo == NULL) {
		g_print("\nOne of the files (info/data) cant be loaded...\n");
		return (void *)status;
	}

	ret_scan = fscanf(pvFileInfo, "%d\n", &size);
	vid_caps = (char *)malloc(size + 1);
	ret_scan = fscanf(pvFileInfo, "%[^\n]s\n", vid_caps);
	g_print("\nV_Caps = %s\n", vid_caps);
	vcap_size = size + 1;

	if (!ret_scan) { /* ToDo: repeat the same for every scanf */
		g_print("\nscan failed");
		return (void *)status;
	}

	while (is_video_readable == 1) {
		/* Read encoded video data */
		is_video_readable = fscanf(pvFileInfo, "%d\n", &vsize);
		is_video_pts_readable = fscanf(pvFileInfo, "%llu\n", &pts_vid);
		is_video_dts_readable = fscanf(pvFileInfo, "%llu\n", &dts_vid);
		is_video_duration_readable = fscanf(pvFileInfo, "%llu\n", &duration_vid);
		is_video_flag_readable = fscanf(pvFileInfo, "%u\n", &flg_vid);
		is_video_key_readable = fscanf(pvFileInfo, "%d\n", &key_vid);

		if (is_video_readable == 1 && is_video_pts_readable == 1 && is_video_dts_readable == 1
		    && is_video_duration_readable == 1 && is_video_flag_readable == 1
		    && is_video_key_readable == 1) {
			g_print("\nv%d: ", ++vcount);
			g_print("\nv_Size: %d, v_pts: %llu, v_dts: %llu v_duration: %llu, v_flag: %d",
			        vsize, pts_vid, dts_vid, duration_vid, key_vid);
			ptr_vid = g_malloc(vsize);
			g_assert(ptr_vid);
			vsize = fread(ptr_vid, 1, vsize, pvFile);

			if (media_format_create(&vidfmt)) {
				g_print("media_format_create failed\n");
				return (void *)status;
			}
			if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H264_SP)) {
				g_print("media_format_set_audio_mime failed\n");
				return (void *)status;
			}
			media_format_set_video_width(vidfmt, vsize / 2 + 1);
			media_format_set_video_height(vidfmt, vsize / 2 + 1);
			/*frame rate is came from the caps filter of demuxer*/
			if (media_format_set_video_frame_rate(vidfmt, 30)) {
				g_print("media_format_set_video_frame_rate failed\n");
				return (void *)status;
			}

			uint64_t ns;
			guint8 *dptr;

			if (media_packet_create(vidfmt, NULL, NULL, &vid_pkt)) {
				g_print("\ncreate v media packet failed tc\n");
				return (void *)status;
			}

			if (media_packet_alloc(vid_pkt)) {
				g_print(" v media packet alloc failed\n");
				return (void *)status;
			}
			media_packet_get_buffer_data_ptr(vid_pkt, (void **)&dptr);
			media_packet_get_buffer_size(vid_pkt, &ns);
			g_print("set v buf size as %d, data size=%d\n", (int)ns, vsize);

			memcpy((char *)dptr, ptr_vid, vsize);
			if (media_packet_set_buffer_size(vid_pkt, vsize)) {
				g_print("set v buf size failed\n");
				return (void *)status;
			}


			if (media_packet_get_buffer_size(vid_pkt, &ns)) {
				g_print("unable to set the v buffer size actual =%d, fixed %d\n", size, (int)ns);
				return (void *)status;
			}
			g_print(" fixed size %d\n", (int)ns);

			if (media_packet_set_pts(vid_pkt, pts_vid)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}
			if (media_packet_set_dts(vid_pkt, dts_vid)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}
			if (media_packet_set_duration(vid_pkt, duration_vid)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}

			if (media_packet_set_flags(vid_pkt, flg_vid)) {
				g_print("unable to set the flag size\n");
				return (void *)status;
			}
			if (media_packet_set_codec_data(vid_pkt, vid_caps, vcap_size)) {
				g_print("unable to set the flag size\n");
				return (void *)status;
			}


			g_print("V write sample call. packet add:%x\n", (unsigned int)vid_pkt);
			mediamuxer_write_sample(myMuxer, track_index_vid, vid_pkt);

			media_packet_destroy(vid_pkt);
		} else {
			g_print("\nVideo while done in the test suite");
			mediamuxer_close_track(myMuxer, track_index_vid);
		}
	}
	g_print("\n\n\n ******* Out of while loop ****** \n\n\n");
	fclose(pvFile);
	fclose(pvFileInfo);
	*status = 0;
	return (void *)status;
}

void *_write_audio_data()
{
	FILE *paFile;
	FILE *paFileInfo;
	unsigned int size;
	unsigned int is_audio_readable = 1;
	unsigned int is_audio_pts_readable;
	unsigned int is_audio_dts_readable;
	unsigned int is_audio_duration_readable;
	unsigned int is_audio_flag_readable;
	unsigned int is_audio_key_readable;
	unsigned char *ptr1;
	unsigned long long int pts;
	unsigned long long int dts;
	unsigned long long int duration;
	int flg;

	int key;
	int acount = 0;
	int track_index_aud = 2; /* track_index=2 for audio */
	char *aud_caps;
	int ret_scan;
	media_packet_h aud_pkt;
	media_format_h audfmt;
	unsigned int acap_size;
	int *status = (int *)g_malloc(sizeof(int) * 1);
	*status = -1;

	paFileInfo = fopen(AAC_INFO, "rt");
	paFile = fopen(AAC_FILE, "rb");

	if (paFile == NULL || paFileInfo == NULL) {
		g_print("\nOne of the files (info/data) cant be loaded...\n");
		return (void *)status;
	}

	ret_scan = fscanf(paFileInfo, "%d\n", &size);
	aud_caps = (char *)malloc(1 + size);
	ret_scan = fscanf(paFileInfo, "%[^\n]s\n", aud_caps);
	g_print("\nA_Caps = %s\n", aud_caps);
	acap_size = size + 1;

	if (!ret_scan) { /* ToDo: repeat the same for every scanf */
		g_print("\nscan failed");
		return (void *)status;
	}

	while (is_audio_readable == 1) {

		/* Read encoded audio data */
		is_audio_readable = fscanf(paFileInfo, "%d\n", &size);
		is_audio_pts_readable = fscanf(paFileInfo, "%llu\n", &pts);
		is_audio_dts_readable = fscanf(paFileInfo, "%llu\n", &dts);
		is_audio_duration_readable = fscanf(paFileInfo, "%llu\n", &duration);
		is_audio_flag_readable = fscanf(paFileInfo, "%u\n", &flg);
		is_audio_key_readable = fscanf(paFileInfo, "%d\n", &key);

		if (is_audio_readable == 1 && is_audio_pts_readable == 1
		    && is_audio_dts_readable == 1 && is_audio_duration_readable == 1
		    && is_audio_flag_readable == 1 && is_audio_key_readable == 1) {
			g_print("\na%d: ", ++acount);
			g_print("\nSize: %d, a_pts: %llu, a_dts: %llu, a_duration: %llu, a_flag:%d, a_key:%d",
			        size, pts, dts, duration, flg, key);

			if (media_format_create(&audfmt)) {
				g_print("media_format_create failed\n");
				return (void *)status;
			}
			if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AAC)) {
				g_print("media_format_set_audio_mime failed\n");
				return (void *)status;
			}

			ptr1 = g_malloc(size);
			g_assert(ptr1);

			size = fread(ptr1, 1, size, paFile);

			/* To create media_pkt */
			uint64_t ns;
			guint8 *dptr;

			if (media_packet_create(audfmt, NULL, NULL, &aud_pkt)) {
				g_print("create audio media_packet failed\n");
				return (void *)status;
			}

			if (media_packet_alloc(aud_pkt)) {
				g_print("audio media_packet alloc failed\n");
				return (void *)status;
			}
			media_packet_get_buffer_data_ptr(aud_pkt, (void **)&dptr);
			memcpy((char *)dptr, ptr1, size);

			if (media_packet_set_buffer_size(aud_pkt, (uint64_t)size)) {
				g_print("audio set_buffer_size failed\n");
				return (void *)status;
			}

			if (media_packet_get_buffer_size(aud_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %d\n", size, (int)&ns);
				return (void *)status;
			}

			g_print(" fixed size %d\n", (int)ns);

			if (media_packet_set_pts(aud_pkt, pts)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}

			if (media_packet_set_dts(aud_pkt, dts)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}

			if (media_packet_set_duration(aud_pkt, duration)) {
				g_print("unable to set the pts\n");
				return (void *)status;
			}

			if (media_packet_set_flags(aud_pkt, key)) {
				g_print("unable to set the flag size\n");
				return (void *)status;
			}
			if (media_packet_set_codec_data(aud_pkt, aud_caps, acap_size)) {
				g_print("unable to set the audio codec data e\n");
				return (void *)status;
			}


			g_print("A write sample call. packet add:%x\n", (unsigned int)aud_pkt);
			mediamuxer_write_sample(myMuxer, track_index_aud, aud_pkt);

			media_packet_destroy(aud_pkt);
		} else {
			g_print("\nAudio while done in the test suite");
			mediamuxer_close_track(myMuxer, track_index_aud);
		}

	}
	g_print("\n\n\n ******* Out of while loop ****** \n\n\n");

	fclose(paFile);
	fclose(paFileInfo);
	*status = 0;
	return (void *)status;
}


int test_mediamuxer_write_sample()
{
	pthread_t thread[2];
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	g_print("In main: creating thread  for audio\n");
	pthread_create(&thread[0], &attr, _write_video_data, NULL);
	pthread_create(&thread[1], &attr, _write_audio_data, NULL);
	pthread_attr_destroy(&attr);
	return 0;
}

int test_mediamuxer_stop()
{
	g_print("test_mediamuxer_stop\n");
	mediamuxer_stop(myMuxer);
	media_format_unref(media_format_a);
	media_format_unref(media_format);
	return 0;
}

int test_mediamuxer_pause()
{
	g_print("test_mediamuxer_pause\n");
	mediamuxer_state_e state;
	if (mediamuxer_get_state(myMuxer, &state) == MEDIAMUXER_ERROR_NONE) {
		g_print("\nMediamuxer_state=%d",state);
		if (state == MEDIAMUXER_STATE_MUXING)
			mediamuxer_pause(myMuxer);
	}
	return 0;
}

int test_mediamuxer_resume()
{
	g_print("test_mediamuxer_resume\n");
	mediamuxer_resume(myMuxer);
	return 0;
}

int test_mediamuxer_add_track()
{
	g_print("test_mediamuxer_add_track\n");
	return 0;
}

void quit_testApp(void)
{
	/* To Do: Replace exit(0) with smooth exit */
	exit(0);
}

enum {
	CURRENT_STATUS_MAINMENU,
	CURRENT_STATUS_AUDIO_FILENAME,
	CURRENT_STATUS_AUDIO_INFONAME,
	CURRENT_STATUS_VIDEO_FILENAME,
	CURRENT_STATUS_VIDEO_INFONAME,
};

int g_menu_state = CURRENT_STATUS_MAINMENU;
static void display_sub_basic();

void reset_menu_state()
{
	g_menu_state = CURRENT_STATUS_MAINMENU;
	return;
}

static void input_filepath(char *filename)
{
	g_print("Opening file %s\n", filename);
	return;
}

void _interpret_main_menu(char *cmd)
{
	int len = strlen(cmd);
	if (len == 1) {
		if (strncmp(cmd, "c", 1) == 0) {
			test_mediamuxer_create();
		} else if (strncmp(cmd, "o", 1) == 0) {
			test_mediamuxer_set_data_sink();
		} else if (strncmp(cmd, "d", 1) == 0) {
			test_mediamuxer_destroy();
		} else if (strncmp(cmd, "s", 1) == 0) {
			test_mediamuxer_start();
		} else if (strncmp(cmd, "a", 1) == 0) {
			g_menu_state = CURRENT_STATUS_AUDIO_FILENAME;
		} else if (strncmp(cmd, "v", 1) == 0) {
			g_menu_state = CURRENT_STATUS_VIDEO_FILENAME;
		} else if (strncmp(cmd, "m", 1) == 0) {
			test_mediamuxer_write_sample();
		} else if (strncmp(cmd, "e", 1) == 0) {
			test_mediamuxer_stop();
		} else if (strncmp(cmd, "p", 1) == 0) {
			test_mediamuxer_pause();
		} else if (strncmp(cmd, "r", 1) == 0) {
			test_mediamuxer_resume();
		} else if (strncmp(cmd, "b", 1) == 0) {
			test_mediamuxer_set_error_cb();
		} else if (strncmp(cmd, "q", 1) == 0) {
			quit_testApp();
		} else {
			g_print("unknown menu command. Please try again\n");
		}
	} else {
		g_print("unknown menu command. Please try again\n");
	}

	return;
}

static void displaymenu(void)
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU) {
		display_sub_basic();
	} else if (g_menu_state == CURRENT_STATUS_AUDIO_FILENAME) {
		g_print("*** input encoded audio_data path:\n");
		g_print("[This is the raw encoded audio file to be muxed]:");
	} else if (g_menu_state == CURRENT_STATUS_AUDIO_INFONAME) {
		g_print("*** input encoded audio info (extra data) path\n");
		g_print("[This is the extra-information needed to mux.");
		g_print("This includes gst-caps too]:");
	} else if (g_menu_state == CURRENT_STATUS_VIDEO_FILENAME) {
		g_print("*** input encoded video path\n");
		g_print("[This is the raw encoded video file to be muxed]:");
	} else if (g_menu_state == CURRENT_STATUS_VIDEO_INFONAME) {
		g_print("*** input encoded video info (extra data) path\n");
		g_print("[This is the extra-information needed to mux.");
		g_print("This includes gst-caps too]:");
	} else {
		g_print("*** unknown status.\n");
		exit(0);
	}
	g_print(" >>> ");
}

gboolean timeout_menu_display(void *data)
{
	displaymenu();
	return FALSE;
}

static void interpret(char *cmd)
{

	switch (g_menu_state) {
		case CURRENT_STATUS_MAINMENU: {
				_interpret_main_menu(cmd);
				break;
			}
		case CURRENT_STATUS_AUDIO_FILENAME: {
				input_filepath(cmd);
				strcpy(audio_data, cmd);
				g_menu_state = CURRENT_STATUS_AUDIO_INFONAME;
				break;
			}
		case CURRENT_STATUS_AUDIO_INFONAME: {
				input_filepath(cmd);
				strcpy(audio_extra_info, cmd);
				test_mediamuxer_add_track_audio();
				g_menu_state = CURRENT_STATUS_MAINMENU;

				break;
			}
		case CURRENT_STATUS_VIDEO_FILENAME: {
				input_filepath(cmd);
				strcpy(video_data, cmd);
				g_menu_state = CURRENT_STATUS_VIDEO_INFONAME;
				break;
			}
		case CURRENT_STATUS_VIDEO_INFONAME: {
				input_filepath(cmd);
				strcpy(video_extra_info, cmd);
				test_mediamuxer_add_track_video();
				g_menu_state = CURRENT_STATUS_MAINMENU;
				break;
			}
		default:
			break;
	}
	g_timeout_add(100, timeout_menu_display, 0);
}

static void display_sub_basic()
{
	g_print("\n");
	g_print("==========================================================\n");
	g_print("                    media muxer test\n");
	g_print("----------------------------------------------------------\n");
	g_print("c. Create \t");
	g_print("o. Set Data Sink \t");
	g_print("a. AddAudioTrack \t");
	g_print("v. AddVideoTrack \t");
	g_print("s. Start \t");
	g_print("m. StartMuxing \t");
	g_print("p. PauseMuxing \t");
	g_print("r. ResumeMuxing \t");
	g_print("b. set error callback \t");
	g_print("e. Stop (eos) \n");
	g_print("d. destroy \t");
	g_print("q. quit \t");
	g_print("\n");
	g_print("==========================================================\n");
}

/**
 * This function is to execute command.
 *
 * @param	channel [in]    1st parameter
 *
 * @return	This function returns TRUE/FALSE
 * @remark
 * @see
 */
gboolean input(GIOChannel *channel)
{
	gchar buf[MAX_STRING_LEN];
	gsize read;
	GError *error = NULL;
	g_io_channel_read_chars(channel, buf, MAX_STRING_LEN, &read, &error);
	buf[read] = '\0';
	g_strstrip(buf);
	interpret(buf);
	return TRUE;
}

/**
 * This function is the example main function for mediamuxer API.
 *
 * @param
 *
 * @return      This function returns 0.
 * @remark
 * @see         other functions
 */
int main(int argc, char *argv[])
{
	GIOChannel *stdin_channel;
	GMainLoop *loop = g_main_loop_new(NULL, 0);
	stdin_channel = g_io_channel_unix_new(0);
	g_io_channel_set_flags(stdin_channel, G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc) input, NULL);

	displaymenu();
	/* g_print("RUN main loop\n"); */
	g_main_loop_run(loop);
	g_print("STOP main loop\n");

	g_main_loop_unref(loop);
	return 0;
}
