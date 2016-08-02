#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <inttypes.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mediamuxer_port.h"
#include <mediademuxer.h>
#include <mediamuxer.h>


mediademuxer_h demuxer = NULL;
media_format_mimetype_e v_mime;
media_format_mimetype_e a_mime;
media_format_mimetype_e t_mime;
media_format_text_type_e t_type;
int num_tracks = 0;
int aud_track = -1;
int vid_track = -1;
int txt_track = -1;
int w;
int h;
bool is_adts = 0;
bool vid_eos_track = 0;
bool aud_eos_track = 0;
bool text_eos_track = 0;

extern int track_index_vid;
extern int track_index_aud;

extern mediamuxer_h myMuxer;


void eos_cb(int track_index, void *user_data)
{
	printf("Got EOS for track -- %d from Mediademuxer\n", track_index);
	if (track_index == vid_track) {
		vid_eos_track = true;
		mediamuxer_close_track(myMuxer, track_index_vid);
	} else if (track_index == aud_track) {
		aud_eos_track = true;
		mediamuxer_close_track(myMuxer, track_index_aud);
	} else if (track_index == txt_track) {
		text_eos_track = true;
	} else {
		g_print("EOS for invalid track number\n");
	}
}

void err_cb(mediademuxer_error_e error, void *user_data)
{
	printf("Got Error %d from Mediademuxer\n", error);
}

int test_mediademuxer_create()
{
	int ret = 0;
	g_print("test_mediademuxer_create\n");
	ret = mediademuxer_create(&demuxer);
	return ret;
}

int test_mediademuxer_set_data_source(const char *path)
{
	int ret = 0;
	g_print("test_mediademuxer_set_data_source\n");

	ret = mediademuxer_set_data_source(demuxer, path);
	ret = mediademuxer_set_eos_cb(demuxer, eos_cb, demuxer);
	ret = mediademuxer_set_error_cb(demuxer, err_cb, demuxer);
	return ret;
}

int test_mediademuxer_prepare()
{
	int ret = 0;
	g_print("test_mediademuxer_prepare\n");
	ret = mediademuxer_prepare(demuxer);
	return ret;
}

int test_mediademuxer_get_track_count(int *track_num)
{
	int num_tracks = 0;
	g_print("test_mediademuxer_get_track_count\n");
	mediademuxer_get_track_count(demuxer, &num_tracks);
	g_print("Number of total tracks [%d]\n", num_tracks);
	*track_num = num_tracks;
	return 0;
}

int test_mediademuxer_select_track(int track_num)
{
	g_print("test_mediademuxer_select_track\n");
	if (mediademuxer_select_track(demuxer, track_num)) {
		g_print("mediademuxer_select_track index [%d] failed\n", track_num);
		return -1;
	}
	return 0;
}

int test_mediademuxer_start()
{
	int ret = 0;
	g_print("test_mediademuxer_start\n");
	ret = mediademuxer_start(demuxer);
	return ret;
}

int test_mediademuxer_get_track_info(int track_num, media_format_h *format, bool *is_video, bool *is_audio)
{
	int ret = 0;

	g_print("test_mediademuxer_get_track_info\n");

	media_format_h g_media_format;
	ret = mediademuxer_get_track_info(demuxer, track_num, &g_media_format);
	if (ret == 0) {
		media_format_mimetype_e v_mime;
		media_format_mimetype_e a_mime;
		int w;
		int h;
		int channel;
		int samplerate;
		int bit;
		if (media_format_get_video_info(g_media_format, &v_mime,
			&w, &h, NULL, NULL) == MEDIA_FORMAT_ERROR_NONE) {
			g_print("media_format_get_video_info is sucess!\n");
			g_print("\t\t[media_format_get_video]mime:%x, width :%d, height :%d\n",
						v_mime, w, h);
			*is_video = true;
			*is_audio = false;
			vid_track = track_num;
		} else if (media_format_get_audio_info(g_media_format, &a_mime,
					&channel, &samplerate, &bit, NULL) == MEDIA_FORMAT_ERROR_NONE) {
			g_print("media_format_get_audio_info is sucess!\n");
			g_print("\t\t[media_format_get_audio]mime:%x, channel :%d, samplerate :%d, bit :%d\n",
						a_mime, channel, samplerate, bit);
			*is_audio = true;
			*is_video = false;
			aud_track = track_num;
		} else if (media_format_get_text_info(g_media_format, NULL, NULL) == MEDIA_FORMAT_ERROR_NONE) {
				g_print("media_format_get_text_info is sucess!\n");
				g_print("\t\t[media_format_get_text]mime:%x, type:%x\n", t_mime, t_type);
		} else {
				g_print("Not Supported YET\n");
		}
	} else {
		g_print("Error while getting mediademuxer_get_track_info\n");
	}

	*format = g_media_format;

	return ret;
}

void *_fetch_video_data(void *ptr)
{
	int ret = MEDIADEMUXER_ERROR_NONE;
	int *status = (int *)g_malloc(sizeof(int) * 1);
	if (!status) {
		g_print("Fail malloc fetch video data retur status value\n");
		return NULL;
	}
	media_packet_h vidbuf;
	int count = 0;
	uint64_t buf_size = 0;
	void *data = NULL;

	*status = -1;
	g_print("Video Data function\n");

	while (1) {
		ret = mediademuxer_read_sample(demuxer, vid_track, &vidbuf);
		if (ret != MEDIADEMUXER_ERROR_NONE) {
			g_print("Error (%d) return of mediademuxer_read_sample()\n", ret);
			pthread_exit(NULL);
		}

		ret = mediamuxer_write_sample(myMuxer, track_index_vid, vidbuf);
		if (vid_eos_track)
			break;
		count++;
		media_packet_get_buffer_size(vidbuf, &buf_size);
		media_packet_get_buffer_data_ptr(vidbuf, &data);
		g_print("Video Read Count::[%4d] frame - get_buffer_size = %"PRIu64"\n", count, buf_size);

		media_packet_destroy(vidbuf);
		usleep(1000);
	}
	g_print("EOS return of mediademuxer_read_sample() for video\n");
	*status = 0;

	return (void *)status;
}

void *_fetch_audio_data(void *ptr)
{
	int ret = MEDIADEMUXER_ERROR_NONE;

	int *status = (int *)g_malloc(sizeof(int) * 1);
	if (!status) {
		g_print("Fail malloc fetch video data retur status value\n");
		return NULL;
	}

	media_packet_h audbuf;
	int count = 0;
	uint64_t buf_size = 0;
	void *data = NULL;

	*status = -1;
	g_print("Audio Data function\n");

	while (1) {
		ret = mediademuxer_read_sample(demuxer, aud_track, &audbuf);
		if (ret != MEDIADEMUXER_ERROR_NONE) {
			g_print("Error (%d) return of mediademuxer_read_sample()\n", ret);
			pthread_exit(NULL);
		}

		ret = mediamuxer_write_sample(myMuxer, track_index_aud, audbuf);

		if (aud_eos_track)
			break;
		count++;
		media_packet_get_buffer_size(audbuf, &buf_size);
		media_packet_get_buffer_data_ptr(audbuf, &data);
		g_print("Audio Read Count::[%4d] frame - get_buffer_size = %"PRIu64"\n", count, buf_size);

		media_packet_destroy(audbuf);
		usleep(1000);
	}

	g_print("EOS return of mediademuxer_read_sample() for audio\n");
	*status = 0;

	return (void *)status;
}

void test_mediademuxer_process_all(bool is_video, bool is_audio)
{
	pthread_t thread[2];

	/* Initialize and set thread detached attribute */
	if (is_video) {
		g_print("In main: creating thread  for video\n");
		pthread_create(&thread[0], NULL, _fetch_video_data, NULL);
	}
	if (is_audio) {
		g_print("In main: creating thread  for audio\n");
		pthread_create(&thread[1], NULL, _fetch_audio_data, NULL);
	}

	return;
}


