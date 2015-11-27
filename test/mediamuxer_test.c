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
#include <inttypes.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mediamuxer_port.h"
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <media_packet_internal.h>

#include <media_codec.h>
#define DUMP_OUTBUF 1
/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define PACKAGE "mediamuxer_test"
#define TEST_FILE_SIZE	(10 * 1024 * 1024)	/* 10M - test case */
#define MAX_STRING_LEN 256
#define DEFAULT_OUT_BUF_WIDTH 640
#define DEFAULT_OUT_BUF_HEIGHT 480
#define OUTBUF_SIZE (DEFAULT_OUT_BUF_WIDTH * DEFAULT_OUT_BUF_HEIGHT * 3 / 2)

#define DEFAULT_SAMPPLERATE 44100
#define DEFAULT_CHANNEL	    2
#define DEFAULT_BIT	    16
#define DEFAULT_BITRATE     128
#define ADTS_HEADER_SIZE    7

#ifndef GST_SECOND
#define GST_SECOND (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))
#endif

mediamuxer_h myMuxer = 0;
media_format_h media_format = NULL;
media_format_h media_format_a = NULL;

media_format_h input_fmt = NULL;

#if DUMP_OUTBUF
int g_OutFileCtr;
FILE *fp_in = NULL;
bool validate_dump = false;
#endif

GQueue input_available;
int use_video = 1;   /* 1 to test video with codec,   0 for audio */
int use_encoder = 1;
media_format_mimetype_e mimetype;
int width = DEFAULT_OUT_BUF_WIDTH;
int height = DEFAULT_OUT_BUF_HEIGHT;
float fps = 0;
int target_bits = 0;

int samplerate = DEFAULT_SAMPPLERATE;
int channel = DEFAULT_CHANNEL;
int bit = DEFAULT_BIT;
unsigned char buf_adts[ADTS_HEADER_SIZE];
uint64_t pts = 0;
static int bitrate = DEFAULT_BITRATE;
int iseos_codec = 0;
bool validate_with_codec = false;
bool validate_multitrack = false;
char media_file[2048];
char data_sink[2048];
bool have_mp4 = false;
bool have_vid_track = false;
bool have_aud_track = false;
int track_index_vid;
int track_index_aud;
int track_index_aud2;

int demux_mp4();
int demux_audio();
void mediacodec_process_all(void);
void input_raw_filepath(char *filename);
void mediacodec_config_set_codec(int codecid, int flag);
void mediacodec_config_set_venc_info(int width, int height, float fps, int target_bits);
void mediacodec_config_prepare(void);
void mediacodec_config_set_aenc_info(int samplerate, int chnnel, int bit, int bitrate);

/*-----------------------------------------------------------------------
|    LOCAL FUNCTION                                                     |
-----------------------------------------------------------------------*/
int test_mediamuxer_create()
{
	g_print("test_mediamuxer_create\n");
	g_print("myMuxer = %p\n", myMuxer);

	if (mediamuxer_create(&myMuxer) != MEDIAMUXER_ERROR_NONE)
		g_print("mediamuxer create is failed\n");

	g_print("Muxer->mx_handle created successfully with address = %p\n",
		(void *)((mediamuxer_s *)myMuxer)->mx_handle);
	g_print("Muxer handle created successfully with address = %p\n", myMuxer);

	return 0;
}

int test_mediamuxer_set_data_sink()
{
	char *op_uri = "\0";
	int ret = 0;
	g_print("test_mediamuxer_set_data_sink\n");

		/* Set data source after creating */
	g_print("\nData_sink choosen is: %s\n", data_sink);

	/* Set data sink after creating */
	if (strncmp(data_sink, "1", 1) == 0 || strncmp(data_sink, "mp4", 3) == 0) {
		op_uri = "MuxTest.mp4";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_MP4);
	} else if (strncmp(data_sink, "2", 1) == 0 || strncmp(data_sink, "3gp", 3) == 0) {
		op_uri = "MuxTest.3gp";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_3GP);
	} else if (strncmp(data_sink, "3", 1) == 0 || strncmp(data_sink, "4", 1) == 0) {
		op_uri = "MuxTest.3gp";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_3GP);
	} else if (strncmp(data_sink, "5", 1) == 0 || strncmp(data_sink, "wav", 3) == 0) {
		op_uri = "MuxTest.wav";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_WAV);
	} else if (strncmp(data_sink,"6",1) == 0 || strncmp(data_sink,"amr-nb",6) == 0) {
		op_uri = "MuxTest_nb.amr";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_AMR_NB);
	} else if (strncmp(data_sink,"7",1) == 0 || strncmp(data_sink,"amr-wb",6) == 0) {
		op_uri = "MuxTest_wb.amr";
		ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_AMR_WB);
	}

	g_print("\nFile will be saved to: %s\n",op_uri);

	if (ret != MEDIAMUXER_ERROR_NONE)
		g_print("mediamuxer_set_data_sink is failed\n");
	return ret;
}

int test_mediamuxer_add_track_video()
{
	media_format_mimetype_e mimetype;
	int width;
	int height;
	int avg_bps;
	int max_bps;

	g_print("test_mediamuxer_add_track_video\n");
	media_format_create(&media_format);

	/* MEDIA_FORMAT_H264_SP  MEDIA_FORMAT_H264_MP  MEDIA_FORMAT_H264_HP */
	if (strncmp(data_sink, "1", 1) == 0 || strncmp(data_sink, "mp4", 3) == 0) {
		if (media_format_set_video_mime(media_format, MEDIA_FORMAT_H264_SP) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation\n");
	} else if (strncmp(data_sink, "2", 1) == 0 || strncmp(data_sink, "3gp", 3) == 0) {
		if (media_format_set_video_mime(media_format, MEDIA_FORMAT_H264_SP) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation\n");
	} else if (strncmp(data_sink, "3", 1) == 0) {
		if (media_format_set_video_mime(media_format, MEDIA_FORMAT_H263) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation\n");
	} else if (strncmp(data_sink, "4", 1) == 0) {
		if (media_format_set_video_mime(media_format, MEDIA_FORMAT_H263) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation\n");
	} else if (strncmp(data_sink, "5", 1) == 0
		|| strncmp(data_sink,"6",1) == 0 || strncmp(data_sink,"7",1) == 0) {
		g_print("Add video track is invalid for wav/amr\n");
		return 1;
	}

	if (validate_with_codec) {
		media_format_set_video_width(media_format, width);
		media_format_set_video_height(media_format, height);
	} else {
		media_format_set_video_width(media_format, 640);
		media_format_set_video_height(media_format, 480);
	}
	media_format_set_video_avg_bps(media_format, 256000);
	media_format_set_video_max_bps(media_format, 256000);

	media_format_get_video_info(media_format, &mimetype, &width, &height, &avg_bps, &max_bps);

	g_print("Video Mimetype trying to set: %x (H264 : %x)\n", (int)(mimetype), (int)(MEDIA_FORMAT_H264_SP));
	g_print("Video param trying to set: (width, height, avg_bps, max_bps): %d %d %d %d  \n",
			width, height, avg_bps, max_bps);

	/* To add video track */
	mediamuxer_add_track(myMuxer, media_format, &track_index_vid);
	g_print("Video Track index is returned : %d\n", track_index_vid);
	return 0;
}

int test_mediamuxer_add_track_audio()
{
	media_format_mimetype_e mimetype;
	int channel;
	int samplerate;
	int bit;
	int avg_bps;

	g_print("test_mediamuxer_add_track_audio\n");
	media_format_create(&media_format_a);

	if (strncmp(data_sink, "1", 1) == 0 || strncmp(data_sink, "mp4", 3) == 0) {
		/* MEDIA_FORMAT_AAC_LC  MEDIA_FORMAT_AAC_HE  MEDIA_FORMAT_AAC_HE_PS */
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AAC_LC) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation\n");
	} else if (strncmp(data_sink, "2", 1) == 0 || strncmp(data_sink, "3gp", 3) == 0
			|| strncmp(data_sink, "3", 1) == 0) {
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AAC_LC) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation, for AAC in 3GP\n");
	} else if (strncmp(data_sink, "4", 1) == 0) {
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AMR_NB) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation for AMR_NB in 3GP\n");
	} else if (strncmp(data_sink, "5", 1) == 0) {
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_PCM) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation for PCM in WAV\n");
	} else if (strncmp(data_sink,"6",1) == 0) {
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AMR_NB) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation for amr-nb audio\n");
	} else if (strncmp(data_sink,"7",1) == 0) {
		if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AMR_WB) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_mime operation for amr-wb audio\n");
	}



	if (validate_with_codec) {
		if (media_format_set_audio_channel(media_format_a, channel) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_channel operation\n");
		media_format_set_audio_samplerate(media_format_a, samplerate);
		media_format_set_audio_bit(media_format_a, bit);
		media_format_set_audio_avg_bps(media_format_a, bitrate);
	} else {
		if (media_format_set_audio_channel(media_format_a, 2) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
			g_print("Problem during media_format_set_audio_channel operation\n");
		media_format_set_audio_samplerate(media_format_a, 44100);
		media_format_set_audio_bit(media_format_a, 16);
		media_format_set_audio_avg_bps(media_format_a, 128000);
	}

	media_format_set_audio_aac_type(media_format_a, true);

	media_format_get_audio_info(media_format_a, &mimetype, &channel, &samplerate, &bit, &avg_bps);

	g_print("Audio Mimetype trying to set: %x (AAC : %x)\n", (int)(mimetype), (int)(MEDIA_FORMAT_AAC_LC));
	g_print("Audio Param trying to set: (ch, samplert, bt, avg_bps) %d %d %d %d \n",
		channel, samplerate, bit, avg_bps);

	/* To add audio track */
	mediamuxer_add_track(myMuxer, media_format_a, &track_index_aud);

	if (validate_multitrack) {
		mediamuxer_add_track(myMuxer, media_format_a, &track_index_aud2);
		g_print("Audio Track index is returned : %d\n", track_index_aud2);
	}

	g_print("Audio Track index is returned : %d\n", track_index_aud);
	return 0;
}

int test_mediamuxer_prepare()
{
	g_print("test_mediamuxer_prepare\n");
	mediamuxer_prepare(myMuxer);
	return 0;
}

int test_mediamuxer_start()
{
	g_print("test_mediamuxer_start\n");
	mediamuxer_start(myMuxer);
	return 0;
}

int test_mediamuxer_write_sample()
{
	if (validate_with_codec) {
		/* Test muxer with codec */
		mediacodec_process_all();
	} else if (strncmp(data_sink,"5",1) == 0 || strncmp(data_sink,"wav",3) == 0
		|| strncmp(data_sink,"6",1) == 0
		|| strncmp(data_sink,"7",1) == 0 || strncmp(data_sink,"amr",3) == 0) {
		demux_audio();
	} else {

		demux_mp4();
	}
	return 0;
}

int test_mediamuxer_pause()
{
	g_print("test_mediamuxer_pause\n");
	mediamuxer_state_e state;
	if (mediamuxer_get_state(myMuxer, &state) == MEDIAMUXER_ERROR_NONE) {
		g_print("Mediamuxer_state=%d\n", state);
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

int test_mediamuxer_stop()
{
	g_print("test_mediamuxer_stop\n");
	mediamuxer_stop(myMuxer);
	return 0;
}

int test_mediamuxer_unprepare()
{
	g_print("test_mediamuxer_unprepare\n");
	mediamuxer_unprepare(myMuxer);
	media_format_unref(media_format_a);
	media_format_unref(media_format);
	return 0;
}

int test_mediamuxer_destroy()
{
	int ret = 0;
	g_print("test_mediamuxer_destroy\n");
	ret = mediamuxer_destroy(myMuxer);
	myMuxer = NULL;
	g_print("Destroy operation returned: %d\n", ret);
	return ret;
}

void app_err_cb(mediamuxer_error_e error, void *user_data)
{
	printf("Got Error %d from mediamuxer\n", error);
}

int test_mediamuxer_set_error_cb()
{
	int ret = 0;
	g_print("test_mediamuxer_set_error_cb\n");
	ret = mediamuxer_set_error_cb(myMuxer, app_err_cb, myMuxer);
	return ret;
}

/*-----------------------------------------------------------------------
|    TEST  FUNCTION                                                                 |
-----------------------------------------------------------------------*/
void quit_testApp(void)
{
	/* To Do: Replace exit(0) with smooth exit */
	exit(0);
}

enum {
	CURRENT_STATUS_MAINMENU,
	CURRENT_STATUS_MP4_FILENAME,
	CURRENT_STATUS_DATA_SINK,
	CURRENT_STATUS_RAW_VIDEO_FILENAME,
	CURRENT_STATUS_RAW_AUDIO_FILENAME,
	CURRENT_STATUS_SET_VENC_INFO,
	CURRENT_STATUS_SET_AENC_INFO,
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
	g_print("Opening file : %s\n", filename);
	return;
}

void _interpret_main_menu(char *cmd)
{
	int len = strlen(cmd);
	if (len == 1) {
		if (strncmp(cmd, "c", 1) == 0) {
			test_mediamuxer_create();
		} else if (strncmp(cmd, "o", 1) == 0) {
			g_menu_state = CURRENT_STATUS_DATA_SINK;
		} else if (strncmp(cmd, "d", 1) == 0) {
			test_mediamuxer_destroy();
		} else if (strncmp(cmd, "e", 1) == 0) {
			test_mediamuxer_prepare();
		} else if (strncmp(cmd, "s", 1) == 0) {
			test_mediamuxer_start();
		} else if (strncmp(cmd, "a", 1) == 0) {
			if (!validate_with_codec) {
				have_aud_track = true;
				if (have_mp4 == false) {
					g_menu_state = CURRENT_STATUS_MP4_FILENAME;
					have_mp4 = true;
				}
			}
			test_mediamuxer_add_track_audio();
		} else if (strncmp(cmd, "v", 1) == 0) {
			if (!validate_with_codec) {
				have_vid_track = true;
				if (have_mp4 == false) {
					g_menu_state = CURRENT_STATUS_MP4_FILENAME;
					have_mp4 = true;
				}
			}
			test_mediamuxer_add_track_video();
		} else if (strncmp(cmd, "m", 1) == 0) {
			test_mediamuxer_write_sample();
		} else if (strncmp(cmd, "t", 1) == 0) {
			test_mediamuxer_stop();
		} else if (strncmp(cmd, "u", 1) == 0) {
			test_mediamuxer_unprepare();
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
	} else if (len == 2 && validate_with_codec) {
		if (strncmp(cmd, "cv", 2) == 0)
		    g_menu_state = CURRENT_STATUS_RAW_VIDEO_FILENAME;
		else if (strncmp(cmd, "ve", 2) == 0)
		    g_menu_state = CURRENT_STATUS_SET_VENC_INFO;
		else if (strncmp(cmd, "ca", 2) == 0)
		    g_menu_state = CURRENT_STATUS_RAW_AUDIO_FILENAME;
		else if (strncmp(cmd, "ae", 2) == 0)
		    g_menu_state = CURRENT_STATUS_SET_AENC_INFO;
	} else {
		g_print("unknown menu command. Please try again\n");
	}

	return;
}

static void displaymenu(void)
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU) {
		display_sub_basic();
	} else if (g_menu_state == CURRENT_STATUS_MP4_FILENAME) {
		if (!validate_with_codec) {
			g_print("*** input video file path:\n");
			g_print("[This is the file from where demuxed data is fed to muxer]:");
		}
	} else if (g_menu_state == CURRENT_STATUS_DATA_SINK) {
		g_print("*** input the datasink container format:\n");
		g_print("(1) mp4 \n(2) 3gp (h264 + AAC) \n(3) 3gp (h263 + AAC) \n(4) 3gp (h263 + AMR) \n(5) wav \n(6) amr-nb \n(7) amr-wb \n");
	} else if (g_menu_state == CURRENT_STATUS_RAW_VIDEO_FILENAME) {
		g_print("*** input raw video file name");
	} else if (g_menu_state == CURRENT_STATUS_SET_VENC_INFO) {
		g_print("*** input video encode configure.(width, height, fps, target_bits)\n");
	} else if (g_menu_state == CURRENT_STATUS_RAW_AUDIO_FILENAME) {
		g_print("*** input raw audio file name");
	} else if (g_menu_state == CURRENT_STATUS_SET_AENC_INFO) {
		g_print("*** input audio encode configure.(samplerate, channel, bit, bitrate (e.g. 44100,  1, 32, 128000))\n");
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
	case CURRENT_STATUS_MP4_FILENAME: {
			input_filepath(cmd);
			strcpy(media_file, cmd);
			g_menu_state = CURRENT_STATUS_MAINMENU;
			break;
		}
	case CURRENT_STATUS_DATA_SINK: {
			strcpy(data_sink, cmd);
			test_mediamuxer_set_data_sink();
			g_menu_state = CURRENT_STATUS_MAINMENU;
			break;
		}
	case CURRENT_STATUS_RAW_VIDEO_FILENAME: {	/* "cv" */
			use_video = 1;
			static int codecid = 0;
			input_raw_filepath(cmd);
			use_encoder = 1;
			codecid = 0x2030; /* video */
			mediacodec_config_set_codec(codecid, 5);
			reset_menu_state();
			g_menu_state = CURRENT_STATUS_MAINMENU;
			break;
		}
	case CURRENT_STATUS_SET_VENC_INFO:	/* "ve" */
	{
		static int cnt = 0;
		switch (cnt) {
		case 0:
			width = atoi(cmd);
			cnt++;
			break;
		case 1:
			height = atoi(cmd);
			cnt++;
			break;
		case 2:
			fps = atol(cmd);
			cnt++;
			break;
		case 3:
			target_bits = atoi(cmd);
			g_print("width = %d, height = %d, fps = %f, target_bits = %d\n", width, height, fps, target_bits);
			mediacodec_config_set_venc_info(width, height, fps, target_bits);
			 mediacodec_config_prepare();
			reset_menu_state();
			cnt = 0;
			break;
		default:
			break;
		}
	}
	break;

	case CURRENT_STATUS_RAW_AUDIO_FILENAME:
		{	/* "ca" */
			use_video = 0;
			static int codecid = 0;
			input_raw_filepath(cmd);
			use_encoder = 1;
			codecid = 0x1060; /* audio */
			mediacodec_config_set_codec(codecid, 9);
			reset_menu_state();
			g_menu_state = CURRENT_STATUS_MAINMENU;
			break;
		}

	case CURRENT_STATUS_SET_AENC_INFO:  /* ae */
		{
			static int cnt = 0;
			switch (cnt) {
			case 0:
				samplerate = atoi(cmd);
				cnt++;
				break;
			case 1:
				channel = atoi(cmd);
				cnt++;
				break;
			case 2:
				bit = atoi(cmd);
				cnt++;
				break;
			case 3:
				bitrate = atoi(cmd);
				mediacodec_config_set_aenc_info(samplerate, channel, bit, bitrate);
				mediacodec_config_prepare();
				reset_menu_state();
				cnt = 0;
				break;
			default:
				break;
			}
		}
		break;
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
	if (validate_with_codec) {
		g_print("--To test Muxer along with Media Codec. --\n");
		g_print("cv. Create Media Codec for Video\t");
		g_print("ve. Set venc info \n");
		g_print("ca. Create Media Codec for Audio\t");
		g_print("ae. Set aenc info \n");
	}
	g_print("c. Create \t");
	g_print("o. Set Data Sink \n");
	g_print("a. AddAudioTrack \t");
	g_print("v. AddVideoTrack \n");
	g_print("e. Prepare \t");
	g_print("s. Start \n");
	g_print("m. StartMuxing \n");
	g_print("p. PauseMuxing \t");
	g_print("r. ResumeMuxing \t");
	g_print("b. Set error callback \n");
	g_print("t. Stop \t");
	g_print("u. UnPrepare \t");
	g_print("d. Destroy \n");
	g_print("q. Quit \n");
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

	if (argc > 1) {
		/* Check whether validation with media codec is required */
		if (argv[1][0] == '-' && argv[1][1] == 'c')
			validate_with_codec = true;
		if (argv[1][0] == '-' && argv[1][1] == 'm')
			validate_multitrack = true;
	}

	displaymenu();
	/* g_print("RUN main loop\n"); */
	g_main_loop_run(loop);
	g_print("STOP main loop\n");

	g_main_loop_unref(loop);
	return 0;
}
