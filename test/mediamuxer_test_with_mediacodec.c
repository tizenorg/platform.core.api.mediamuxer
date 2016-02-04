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

/* ============================================================================
|                                                                              |
|  INCLUDE FILES                                                               |
|                                                                              |
============================================================================= */
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
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <media_packet_internal.h>
#include <media_codec.h>

#define MAX_STRING_LEN 256
#define MAX_HANDLE 10
#define DEFAULT_SAMPLEBYTE  1024

#ifndef GST_SECOND
#define GST_SECOND (G_USEC_PER_SEC * G_GINT64_CONSTANT(1000))
#endif

#define USE_INPUT_QUEUE 1
#define MAX_INPUT_BUF_NUM 20
#define CHECK_BIT(x, y) (((x) >> (y)) & 0x01)
#define GET_IS_ENCODER(x) CHECK_BIT(x, 0)

/* ----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
---------------------------------------------------------------------- */
extern int use_video;   /* 1 to test video with codec,   0 for audio */
extern mediamuxer_h myMuxer;
extern media_format_h input_fmt;
extern int use_encoder;
extern int bitrate;
extern int bitrate;
extern int samplerate;
extern int channel;
extern media_format_mimetype_e mimetype;
extern int bit;
extern uint64_t pts;
extern int width;
extern int height;
extern float fps;
extern int target_bits;

int frame_count = 0;
int g_handle_num = 1;
char g_uri[MAX_STRING_LEN];
int g_len;
int bMultipleFiles;
#if USE_INPUT_QUEUE
media_packet_h input_buf[MAX_INPUT_BUF_NUM];
GQueue input_available;
#else
media_packet_h in_buf;
#endif
static mediacodec_h g_media_codec[MAX_HANDLE] = {0};
static int samplebyte = DEFAULT_SAMPLEBYTE;
int enc_vide_pkt_available = 0;
FILE *fp_src = NULL;

/* ----------------------------------------------------------------------
|    HELPER  FUNCTION                                                   |
---------------------------------------------------------------------- */
/* To aid muxer-codec test */
static void _mediacodec_eos_cb(void *user_data)
{
	g_print("\n\n\n\n event : eos_cb \n\n\n");
	int track_index_vid = 0;
	int track_index_aud = 1;
	if (use_video == 1)
		mediamuxer_close_track(myMuxer, track_index_vid);
	else if (use_video == 0)
		mediamuxer_close_track(myMuxer, track_index_aud);
}

/* To aid muxer-codec test */
void _allocate_buf(void)
{
#if USE_INPUT_QUEUE
	int i = 0;

	/* !!!! remove dependency on internal headers. */
	/* input_avaliable = mc_async_queue_new(); */
	/* input_buf = (media_packet_h *)malloc(sizeof(media_packet_h)*MAX_INPUT_BUF_NUM); */

	for (i = 0; i < MAX_INPUT_BUF_NUM; i++) {
		media_packet_create_alloc(input_fmt, NULL, NULL, &input_buf[i]);
		g_print("if-input queue buf = %p\n", input_buf[i]);
		g_queue_push_tail(&input_available, input_buf[i]);
	}
#else
	media_packet_create_alloc(input_fmt, NULL, NULL, &in_buf);
	g_print("else-input queue buf = %p\n", in_buf);
#endif
	return;
}

/* To aid muxer-codec test */
void input_raw_filepath(char *filename)
{
	int len = strlen(filename);
	int i = 0;

	if (len < 0 || len > MAX_STRING_LEN)
		return;

	for (i = 0; i < g_handle_num; i++) {
		if (g_media_codec[i] != NULL) {
			mediacodec_unprepare(g_media_codec[i]);
			mediacodec_destroy(g_media_codec[i]);
			g_media_codec[i] = NULL;
		}

		if (mediacodec_create(&g_media_codec[i]) != MEDIACODEC_ERROR_NONE)
			g_print("mediacodec create is failed\n");
	}

	media_format_create(&input_fmt);

	strncpy(g_uri, filename, len);
	g_len = len;

	bMultipleFiles = 0;
	if (g_uri[g_len-1] == '/')
		bMultipleFiles = 1;

	return;
}


/* To aid muxer-codec test */
/**
 *  Add ADTS header at the beginning of each and every AAC packet.
 *  This is needed as MediaCodec encoder generates a packet of raw AAC data.
 *  Note the packetLen must count in the ADTS header itself.
 **/
void _add_adts_header_for_aacenc(unsigned char *buffer, int packetLen)
{
	int profile = 2;    /* AAC LC (0x01) */
	int freqIdx = 3;    /* 48KHz (0x03) */
	int chanCfg = 2;    /* CPE (0x02) */

	if (samplerate == 96000)
		freqIdx = 0;
	else if (samplerate == 88200)
		freqIdx = 1;
	else if (samplerate == 64000)
		freqIdx = 2;
	else if (samplerate == 48000)
		freqIdx = 3;
	else if (samplerate == 44100)
		freqIdx = 4;
	else if (samplerate == 32000)
		freqIdx = 5;
	else if (samplerate == 24000)
		freqIdx = 6;
	else if (samplerate == 22050)
		freqIdx = 7;
	else if (samplerate == 16000)
		freqIdx = 8;
	else if (samplerate == 12000)
		freqIdx = 9;
	else if (samplerate == 11025)
		freqIdx = 10;
	else if (samplerate == 8000)
		freqIdx = 11;

	if ((channel == 1) || (channel == 2))
		chanCfg = channel;

	/* fill in ADTS data */
	buffer[0] = (char)0xFF;
	buffer[1] = (char)0xF1;
	buffer[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
	buffer[3] = (char)(((chanCfg&3)<<6) + (packetLen>>11));
	buffer[4] = (char)((packetLen&0x7FF) >> 3);
	buffer[5] = (char)(((packetLen&7)<<5) + 0x1F);
	buffer[6] = (char)0xFC;
}

/* To aid muxer-codec test */
static void _mediacodec_fill_buffer_cb(media_packet_h pkt, void *user_data)
{
	g_print("**** Entered _mediacodec_fill_buffer_cb *****\n");
	int track_index_aud = 1;
	static int count = -1;
	enc_vide_pkt_available = 1;
	uint64_t pts, dts, duration, size;
	media_buffer_flags_e flags;
	void *codec_data;
	void *pkt_data;
	unsigned int codec_data_size;

	g_print("A%d write sample call. packet add:%p\n", ++count, pkt);

	if (media_packet_get_buffer_size(pkt, &size)) {
		g_print("unable to get the buffer size actual =%"PRIu64"\n", size);
		return;
	}

	media_packet_get_pts(pkt, &pts);
	media_packet_get_dts(pkt, &dts);
	media_packet_get_duration(pkt, &duration);
	/* offset */
	media_packet_get_flags(pkt, &flags);
	media_packet_get_extra(pkt, (void**)&codec_data);
	codec_data_size = strlen(codec_data) + 1;

	g_print("***pkt attributes before writing *** Size=%"PRIu64", pts=%"PRIu64", dts=%"PRIu64", duration=%"PRIu64", flags=%d\n", size, pts, dts, duration, (int)flags);
	g_print("Codec_data=%s\ncodec_data_size = %d\n", (char*)codec_data, codec_data_size);

#if DUMP_OUTBUF
	if (count == 0) {
		fp_in = fopen("/opt/usr/codec_dump_mxr.aac", "wb");
		if (fp_in != NULL)
			validate_dump = true;
		else
			g_print("Error - Cannot open file for file dump, Please chek root\n");
	}
#endif

	/* EOS hack for mediacodec */
	if (count == 1758) {  /* Last buffer for SampleAAC.aac. should be replaced with a valid eos. */
		g_print("Last Buffer Reached\n");
#if DUMP_OUTBUF
		fclose(fp_in);
#endif
		_mediacodec_eos_cb(user_data);
		goto Finalize;
	}

	media_packet_get_buffer_data_ptr(pkt, &pkt_data);
#if DUMP_OUTBUF
	if (validate_dump)
		fwrite(pkt_data, 1, size, fp_in);
#endif
	mediamuxer_write_sample(myMuxer, track_index_aud, pkt);

Finalize:
	return;
}

/* To aid muxer-codec test */
void mediacodec_config_set_aenc_info(int samplerate, int chnnel, int bit, int bitrate)
{
	g_print("_mediacodec_set_aenc_info\n");
	g_print("samplerate = %d, channel = %d, bit = %d, bitrate = %d\n", samplerate, chnnel, bit, bitrate);
	if (g_media_codec[0] != NULL)
		mediacodec_set_aenc_info(g_media_codec[0], samplerate, chnnel, bit, bitrate);
	else
		g_print("mediacodec handle is not created\n");
	return;
}

/* To aid muxer-codec test */
void mediacodec_config_set_venc_info(int width, int height, float fps, int target_bits)
{
	g_print("_mediacodec_set_venc_info\n");
	if (g_media_codec[0] != NULL)
		mediacodec_set_venc_info(g_media_codec[0], width, height, fps, target_bits);
	else
		g_print("mediacodec handle is not created\n");

	return;
}

/* To aid muxer-codec test */
static void _mediacodec_empty_buffer_cb(media_packet_h pkt, void *user_data)
{
	if (pkt != NULL) {
#if USE_INPUT_QUEUE
	media_packet_unset_flags(pkt, MEDIA_PACKET_CODEC_CONFIG);
	g_queue_push_tail(&input_available, pkt);
	g_print("availablebuf = %p\n", pkt);
#else
	g_print("Used input buffer = %p\n", pkt);
	media_packet_destroy(pkt);
#endif
	}
	return;
}

/* To aid muxer-codec test */
static bool _mcdiacodec_supported_cb(mediacodec_codec_type_e type, void *user_data)
{
	if (type != -1)
		g_print("type : %x\n", type);
	return true;
}

/* To aid muxer-codec test */
void mediacodec_config_prepare(void)
{
	int i = 0;
	int err = 0;
	int ret;
	if (use_video) {
		g_print(" [video test] mimetype (0x%x), width(%d), height (%d), target_bits (%d)\n", mimetype, width, height, target_bits);
		media_format_set_video_mime(input_fmt, mimetype);
		media_format_set_video_width(input_fmt, width);
		media_format_set_video_height(input_fmt, height);
		media_format_set_video_avg_bps(input_fmt, target_bits);
	} else {
		g_print(" [audio test] mimetype (0x%x), channel(%d), samplerate (%d), bit (%d)\n", mimetype, channel, samplerate, bit);

		if (!(mimetype & MEDIA_FORMAT_AUDIO))
			g_print("\n*****Not supported******\n");

		if (input_fmt == NULL)
			g_print("\n*****NULL******\n");

		ret = media_format_set_audio_mime(input_fmt, mimetype);
		if (ret != MEDIA_FORMAT_ERROR_NONE)
			g_print("->media_format_set_audio_mime failed mime: %d\n", ret);
		media_format_set_audio_channel(input_fmt, channel);
		media_format_set_audio_samplerate(input_fmt, samplerate);
		media_format_set_audio_bit(input_fmt, bit);
	}

	for (i = 0; i < g_handle_num; i++) {
		if (g_media_codec[i] != NULL) {
			g_print("setting codec callbacks\n");
			mediacodec_set_input_buffer_used_cb(g_media_codec[i], _mediacodec_empty_buffer_cb, g_media_codec[i]);
			mediacodec_set_output_buffer_available_cb(g_media_codec[i], _mediacodec_fill_buffer_cb, g_media_codec[i]);
			mediacodec_set_eos_cb(g_media_codec[i], _mediacodec_eos_cb, g_media_codec[i]);

			mediacodec_foreach_supported_codec(g_media_codec[i], _mcdiacodec_supported_cb, g_media_codec[i]);

			err = mediacodec_prepare(g_media_codec[i]);

			if (err != MEDIACODEC_ERROR_NONE)
				g_print("mediacodec_prepare failed error = %d \n", err);
#if USE_INPUT_QUEUE
			_allocate_buf();
#endif
		} else {
			g_print("mediacodec handle is not created\n");
		}
	}
	frame_count = 0;
	enc_vide_pkt_available = 1;

	return;
}

/* To aid muxer-codec Test */
void mediacodec_config_set_codec(int codecid, int flag)
{
	int encoder = 0;
	int ret;
	g_print("_mediacodec_configure\n");
	g_print("codecid = 0x%x, flag = %d\n", codecid, flag);
	g_print("MyTag; codecid = %d, flag = %d\n", codecid, flag);
	if (g_media_codec[0] != NULL) {
		ret = mediacodec_set_codec(g_media_codec[0], (mediacodec_codec_type_e)codecid, flag);
		if (ret !=  MEDIACODEC_ERROR_NONE) {
			g_print("mediacodec set codec is failed, ret = %d, ret_hex=%x\n", ret, ret);
			return;
		}

		encoder = GET_IS_ENCODER(flag) ? 1 : 0;
		if (use_video) {
			if (encoder) {
				mimetype |= MEDIA_FORMAT_NV12;
				mimetype |= MEDIA_FORMAT_RAW;
			} else  {
				if (codecid == MEDIACODEC_MPEG4)
					mimetype |= MEDIA_FORMAT_MPEG4_SP;
				else if (codecid == MEDIACODEC_H263)
					mimetype |= MEDIA_FORMAT_H263;
				else
					mimetype |= MEDIA_FORMAT_H264_SP;
			}
			mimetype |= MEDIA_FORMAT_VIDEO;
		} else {
			if (encoder) {
				mimetype |= MEDIA_FORMAT_RAW;
				mimetype |= MEDIA_FORMAT_PCM;
			}
		mimetype |= MEDIA_FORMAT_AUDIO;
		g_print("[audio test] mimetype (0x%x)\n", mimetype);
		}
	} else {
		g_print("mediacodec handle is not created\n");
	}
	return;
}

/*-----------------------------------------------------------------------
|    HELPER  FUNCTION                                                                 |
-----------------------------------------------------------------------*/
/* To aid muxer-codec test */
int __extract_input_per_frame(FILE *fp, unsigned char *rawdata)
{
	int readsize = 0;
	while (!feof(fp))
		readsize += fread(rawdata+readsize, 1, 1, fp);
	return readsize;
}

/* To aid muxer-codec test */
unsigned int __bytestream2yuv420(FILE *fd, unsigned char* yuv)
{
	size_t result;
	int read_size;
	unsigned char buffer[1500000];

	if (feof(fd))
		return 0;

	read_size = width*height*3/2;

	result = fread(buffer, 1, read_size, fd);
	g_print("3\n");

	if (result != read_size)
		return -1;

	memcpy(yuv, buffer, width*height*3/2);

	return width*height*3/2;
}

/* To aid muxer-codec test */
/** Extract Input data for AAC encoder **/
unsigned int __extract_input_aacenc(FILE *fd, unsigned char* rawdata)
{
	int readsize;
	size_t result;
	unsigned char buffer[1000000];

	if (feof(fd))
		return 0;

	readsize =  ((samplebyte*channel)*(bit/8));
	result = fread(buffer, 1, readsize, fd);
	if (result != readsize)
		return -1;

	memcpy(rawdata, buffer, readsize);

	return readsize;
}

/* To aid muxer-codec test */
int __mediacodec_process_input(void)
{
	int buf_size = 0;
#if USE_INPUT_QUEUE
	media_packet_h in_buf = NULL;
#endif
	void *data = NULL;
	int ret = 0;
	/* int err; */

	if (g_media_codec[0] == NULL) {
		g_print("mediacodec handle is not created\n");
		return MEDIACODEC_ERROR_INVALID_PARAMETER;
	}

	if (fp_src == NULL) {
		if (bMultipleFiles) {
			if (g_uri[g_len-1] != '/')
				g_uri[g_len++] = '/';
			snprintf(g_uri+g_len, MAX_STRING_LEN - g_len, "%05d", frame_count);
		}
		fp_src = fopen(g_uri, "r");
		if (fp_src == NULL) {
			g_print("%s file open failed\n", g_uri);
			return MEDIACODEC_ERROR_INVALID_PARAMETER;
		}
	}

#if USE_INPUT_QUEUE
	in_buf = g_queue_pop_head(&input_available);
#else
	_allocate_buf();
#endif

	if (in_buf != NULL) {
		media_packet_get_buffer_data_ptr(in_buf, &data);
		if (data == NULL)
			return MEDIACODEC_ERROR_INVALID_PARAMETER;
		g_print("use_encoder=%d\n", use_encoder);

		if (use_encoder) {
			if (use_video) {
				/* Video Encoder */
				g_print("4 Video Encoder\n");
				if (bMultipleFiles) {
					buf_size = __extract_input_per_frame(fp_src, data);
					fclose(fp_src);
					snprintf(g_uri+g_len, MAX_STRING_LEN-g_len, "%05d", frame_count+1);
					fp_src = fopen(g_uri, "r");
					if (fp_src == NULL) {
						media_packet_set_flags(in_buf, MEDIA_PACKET_END_OF_STREAM);
						/* err = MEDIACODEC_ERROR_INVALID_INBUFFER; */
					}
				} else {
					buf_size = __bytestream2yuv420(fp_src, data);
					if (buf_size == -1)
						g_print("codec EOS reaced\n");
				}

				g_print("input pts = %"PRIu64"\n", pts);
				media_packet_set_pts(in_buf, pts);

				if (fps != 0)
				    pts += (GST_SECOND / fps);
			} else {
				/* Audio Encoder - AAC */
				buf_size = __extract_input_aacenc(fp_src, data);
				g_print("audio buf_size=%d\n", buf_size);
				/* pts is not needed for muxer, if adts header is present */
				/* media_packet_set_pts (in_buf, pts); */
				g_print("----pts calculation: input pts = %"PRIu64", buf_size=%d samplerate=%d, samplebyte=%d\n", pts, buf_size, samplerate, samplebyte);
				if (samplerate != 0)
					pts += ((GST_SECOND / samplerate) * samplebyte);
			}
		}

		if (buf_size >= 0) {
			if (use_video && buf_size == 4) {
				media_packet_set_flags(in_buf, MEDIA_PACKET_END_OF_STREAM);
				media_packet_set_buffer_size(in_buf, 4);
				mediacodec_process_input(g_media_codec[0], in_buf, 0);
				g_print("\n\nEOS packet is sent\n");

				return MEDIACODEC_ERROR_INVALID_INBUFFER;
			}
			media_packet_set_buffer_size(in_buf, buf_size);
			g_print("%s - input_buf size = %4d  (0x%x) at %4d frame, %p\n", __func__, buf_size, buf_size, frame_count, in_buf);

			ret = mediacodec_process_input(g_media_codec[0], in_buf, 0);
		} else {
			g_print("%s - [WARN] Check to input buf_size = %4d  at %4d frame, %p\n", __func__, buf_size, frame_count, in_buf);
			return MEDIACODEC_ERROR_INVALID_INBUFFER;
		}

		frame_count++;
		g_print("returning, ret=%d, expected ret=%d\n", ret, MEDIACODEC_ERROR_NONE);
		return ret;
	}

	return MEDIACODEC_ERROR_NONE;
}

/* To aid muxer-codec test */
void mediacodec_process_all(void)
{
	int ret = MEDIACODEC_ERROR_NONE;
	g_print("Start of _mediacodec_process_all()\n");

	while (1) {
		ret = __mediacodec_process_input();

		if (ret != MEDIACODEC_ERROR_NONE) {
			g_print ("__mediacodec_process_input ret = %d\n", ret);
			break;
		}
	}

	g_print("End of _mediacodec_process_all()\n");
	return;
}
