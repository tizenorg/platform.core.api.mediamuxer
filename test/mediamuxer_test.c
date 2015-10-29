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
#include <gst/gst.h>
#include <inttypes.h>
#include <mm_error.h>
#include <mm_debug.h>
#include "../include/mediamuxer_port.h"
#include <mediamuxer.h>
#include <mediamuxer_private.h>
#include <media_packet_internal.h>

#include <media_codec.h>
#define DUMP_OUTBUF 1
#define MAX_INPUT_BUF_NUM 20
#define USE_INPUT_QUEUE 1
/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                       |
-----------------------------------------------------------------------*/
#define PACKAGE "mediamuxer_test"
#define TEST_FILE_SIZE	(10 * 1024 * 1024)	/*10M - test case */
#define MAX_STRING_LEN 256
#define MAX_HANDLE 10
#define DEFAULT_OUT_BUF_WIDTH 640
#define DEFAULT_OUT_BUF_HEIGHT 480
#define OUTBUF_SIZE (DEFAULT_OUT_BUF_WIDTH * DEFAULT_OUT_BUF_HEIGHT * 3 / 2)

#define DEFAULT_SAMPPLERATE 44100
#define DEFAULT_CHANNEL	    2
#define DEFAULT_BIT	    16
#define DEFAULT_BITRATE     128
#define DEFAULT_SAMPLEBYTE  1024
#define ADTS_HEADER_SIZE    7

#define CHECK_BIT(x, y) (((x) >> (y)) & 0x01)
#define GET_IS_ENCODER(x) CHECK_BIT(x, 0)
mediamuxer_h myMuxer = 0;
media_format_h media_format = NULL;
media_format_h media_format_a = NULL;

int g_handle_num = 1;
char audio_extra_info[1000];
char audio_data[1000];
char video_extra_info[1000];
char video_data[1000];
static mediacodec_h g_media_codec[MAX_HANDLE] = {0};
media_format_h input_fmt = NULL;
#if USE_INPUT_QUEUE
media_packet_h input_buf[MAX_INPUT_BUF_NUM];
#else
media_packet_h in_buf = NULL;
#endif

#if DUMP_OUTBUF
int g_OutFileCtr;
FILE *fp_in = NULL;
bool validate_dump = false;
#endif

GQueue input_available;
int use_video = 1;   /* 1 to test video with codec,   0 for audio */
int use_encoder = 1;
int enc_vide_pkt_available = 0;
char g_uri[MAX_STRING_LEN];
int g_len, bMultipleFiles;
media_format_mimetype_e mimetype;
static int width = DEFAULT_OUT_BUF_WIDTH;
static int height = DEFAULT_OUT_BUF_HEIGHT;
static float fps = 0;
static int target_bits = 0;

static int samplerate = DEFAULT_SAMPPLERATE;
static int channel = DEFAULT_CHANNEL;
static int bit = DEFAULT_BIT;
unsigned char buf_adts[ADTS_HEADER_SIZE];
int frame_count = 0;
FILE *fp_src = NULL;
uint64_t pts = 0;
static int bitrate = DEFAULT_BITRATE;
static int samplebyte = DEFAULT_SAMPLEBYTE;
int iseos_codec = 0;
bool validate_with_codec = false;
bool validate_multitrack = false;

bool aud_eos = 0;
bool vid_eos = 0;
char *aud_caps, *vid_caps;
char file_mp4[1000];
bool have_mp4 = false;
bool have_vid_track = false;
bool have_aud_track = false;
int track_index_vid;
int track_index_aud;
int track_index_aud2;

/* demuxer sturcture for _demux_mp4() */
typedef struct _CustomData {
	GstElement *pipeline;
	GstElement *source;
	GstElement *demuxer;
	GstElement *audioqueue;
	GstElement *videoqueue;

	GstElement *audio_appsink;	/* o/p of demuxer */
	GstElement *video_appsink;
	GstElement *dummysink;

	char *saveLocation_audio;	/* aac stream */
	char *saveLocation_video;	/* h264 stream */
	GMainLoop *loop;

	GTimer *timer;
} CustomData;


/*-----------------------------------------------------------------------
|    HELPER  FUNCTION                                                                 |
-----------------------------------------------------------------------*/
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
static void _input_raw_filepath(char *filename)
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
void _mediacodec_set_aenc_info(int samplerate, int chnnel, int bit, int bitrate)
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
void _mediacodec_set_venc_info(int width, int height, float fps, int target_bits)
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
void _mediacodec_prepare(void)
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
			g_print("setting codec callbacks");
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
void _mediacodec_set_codec(int codecid, int flag)
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
			sprintf(g_uri+g_len, "%05d", frame_count);
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
		printf("use_encoder=%d\n", use_encoder);

		if (use_encoder) {
			if (use_video) {
				/*  Video Encoder */
				g_print("4 Video Encoder\n");
				if (bMultipleFiles) {
					buf_size = __extract_input_per_frame(fp_src, data);
					fclose(fp_src);
					sprintf(g_uri+g_len, "%05d", frame_count+1);
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
				printf("audio buf_size=%d\n", buf_size);
				/* pts is not needed for muxer, if adts header is present*/
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
			if (use_video && buf_size == -1) {
				g_print("%s - END : input_buf size = %d  frame_count : %d\n", __func__, buf_size, frame_count);
				return MEDIACODEC_ERROR_INVALID_INBUFFER;
			}
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
void _mediacodec_process_all(void)
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

/*-----------------------------------------------------------------------
|    HELPER  FUNCTION                                                                 |
-----------------------------------------------------------------------*/
/* Demuxer audio-appsink buffer receive callback*/
static void __audio_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h audfmt;
	media_packet_h aud_pkt;
	guint8 *dptr;
	static int count = 0;
	GstSample *sample;
	uint64_t ns;
	int key;
	GstMapInfo map;

	if (count == 0)
		g_print("Called __audio_app_sink_callback\n");

	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);
	if (buffer) {
		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
				/* g_print( "  Key Frame  \n"); */
				key = 1;
			} else {
				/* g_print( "  NOT a Key Frame  \n"); */
				key = 0;
			}

			if (media_format_create(&audfmt)) {
				g_print("media_format_create failed\n");
				return;
			}

			if (media_format_set_audio_mime(audfmt, MEDIA_FORMAT_AAC)) {
				g_print("media_format_set_audio_mime failed\n");
				return;
			}

			if (media_packet_create(audfmt, NULL, NULL, &aud_pkt)) {
				g_print("create audio media_packet failed\n");
				return;
			}

			if (media_packet_alloc(aud_pkt)) {
				g_print("audio media_packet alloc failed\n");
				return;
			}

			media_packet_get_buffer_data_ptr(aud_pkt, (void **)&dptr);
			memcpy((char*)dptr, map.data, map.size);

			if (media_packet_set_buffer_size(aud_pkt, (uint64_t)(map.size))) {
				g_print("audio set_buffer_size failed\n");
				return;
			}

			if (media_packet_get_buffer_size(aud_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %"PRIu64"\n",
					(unsigned int)map.size, ns);
				return;
			}

			if (media_packet_set_pts(aud_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(aud_pkt, buffer->dts)) {
				g_print("unable to set the dts\n");
				return;
			}

			if (media_packet_set_duration(aud_pkt, buffer->duration)) {
				g_print("unable to set the duration\n");
				return;
			}

			if (media_packet_set_flags(aud_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}

			if (media_packet_set_extra(aud_pkt, aud_caps)) {
				g_print("unable to set the audio codec data e\n");
				return;
			}

			/* Print count and size to indicate a received buffer */
			g_print("Received audio buffer count : %4d (size : %5"PRIu64", pts : %12"PRIu64")\n",
					++count, ns, buffer->pts);
			mediamuxer_write_sample(myMuxer, track_index_aud, aud_pkt);
			if (validate_multitrack)
				mediamuxer_write_sample(myMuxer, track_index_aud2, aud_pkt);

			media_packet_destroy(aud_pkt);
		}
	}
}

/* Demuxer video-appsink buffer receive callback*/
static void __video_app_sink_callback(GstElement *sink, CustomData *data)
{
	GstBuffer *buffer;
	media_format_h vidfmt;
	media_packet_h vid_pkt;
	uint64_t ns;
	static int count = 0;
	unsigned int vsize;
	int key;
	guint8 *dptr;
	GstMapInfo map;
	GstSample *sample;

	if (count == 0)
		g_print("Called __video_app_sink_callback\n");

	g_signal_emit_by_name(sink, "pull-sample", &sample);
	buffer = gst_sample_get_buffer(sample);

	if (buffer) {
		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			if (media_format_create(&vidfmt)) {
				g_print("media_format_create failed\n");
				return;
			}

			if (media_format_set_video_mime(vidfmt, MEDIA_FORMAT_H264_SP)) {
				g_print("media_format_set_vidio_mime failed\n");
				return;
			}

			if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
				/* g_print("Key Frame\n"); */
				key = 1;
			} else {
				/* g_print("NOT a Key Frame\n"); */
				key = 0;
			}

			vsize = map.size;
			media_format_set_video_width(vidfmt, vsize/2+1);
			media_format_set_video_height(vidfmt, vsize/2+1);
			/*frame rate is came from the caps filter of demuxer*/
			if (media_format_set_video_frame_rate(vidfmt, 30)) {
				g_print("media_format_set_video_frame_rate failed\n");
				return;
			}

			if (media_packet_create(vidfmt, NULL, NULL, &vid_pkt)) {
				g_print("create video media_packet failed\n");
				return;
			}

			if (media_packet_alloc(vid_pkt)) {
				g_print("video media_packet alloc failed\n");
				return;
			}

			media_packet_get_buffer_data_ptr(vid_pkt, (void**)&dptr);
			memcpy((char*)dptr, map.data, map.size);

			if (media_packet_set_buffer_size(vid_pkt, (uint64_t)(map.size))) {
				g_print("video set_buffer_size failed\n");
				return;
			}

			if (media_packet_get_buffer_size(vid_pkt, &ns)) {
				g_print("unable to set the buffer size actual =%d, fixed %"PRIu64"\n",
					(unsigned int)map.size, ns);
				return;
			}

			if (media_packet_set_pts(vid_pkt, buffer->pts)) {
				g_print("unable to set the pts\n");
				return;
			}

			if (media_packet_set_dts(vid_pkt, buffer->dts)) {
				g_print("unable to set the dts\n");
				return;
			}

			if (media_packet_set_duration(vid_pkt, buffer->duration)) {
				g_print("unable to set the duration\n");
				return;
			}

			if (media_packet_set_flags(vid_pkt, key)) {
				g_print("unable to set the flag size\n");
				return;
			}
			if (media_packet_set_extra(vid_pkt, vid_caps)) {
				g_print("unable to set the video codec data e\n");
				return;
			}

			/* Print count and size to indicate a received buffer */
			g_print("Received video buffer count : %4d (size : %5"PRIu64", pts : %12"PRIu64")\n",
					++count, ns, buffer->pts);
			mediamuxer_write_sample(myMuxer, track_index_vid, vid_pkt);
			media_packet_destroy(vid_pkt);
		}
	}

}

/* demuxer audio appsink eos callback */
static void __audio_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	mediamuxer_close_track(myMuxer, track_index_aud);
	if (validate_multitrack)
		mediamuxer_close_track(myMuxer, track_index_aud2);
	g_print("audio (AAC) EOS cb reached \n");
	aud_eos = 1;
	if (!have_vid_track || vid_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer video appsink eos callback */
static void __video_app_sink_eos_callback(GstElement *sink, CustomData *data)
{
	mediamuxer_close_track(myMuxer, track_index_vid);
	g_print("video (h264) EOS cb reached \n");
	vid_eos = 1;
	if (!have_aud_track || aud_eos == 1)
		g_main_loop_quit(data->loop);
}

/* demuxer on_pad callback */
static void __on_pad_added(GstElement *element, GstPad *pad, CustomData *data)
{
	GstPadLinkReturn ret;
	GstPad *sink_pad_audioqueue = gst_element_get_static_pad(data->audioqueue, "sink");
	GstPad *sink_pad_videoqueue = gst_element_get_static_pad(data->videoqueue, "sink");
	GstCaps *new_pad_aud_caps = NULL;
	GstCaps *new_pad_vid_caps = NULL;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;
	char *caps;
	g_print("Received new pad '%s' from '%s' :\n", GST_PAD_NAME(pad), GST_ELEMENT_NAME(element));

	new_pad_caps = gst_pad_get_current_caps(pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	if (have_aud_track && g_str_has_prefix(new_pad_type, "audio/mpeg")) {
		new_pad_aud_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_aud_caps);
		g_print("   Audio caps :%s\n", caps);
		aud_caps = caps;

		/* Link demuxer to audioqueue */
		ret = gst_pad_link(pad, sink_pad_audioqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print("   Type is but link failed.\n %s", new_pad_type);
		else
			g_print("   Link succeeded (type '%s').\n", new_pad_type);

		gst_element_link(data->audioqueue, data->audio_appsink);
		g_object_set(data->audio_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->audio_appsink, "new-sample", G_CALLBACK(__audio_app_sink_callback), data);
		g_signal_connect(data->audio_appsink, "eos", G_CALLBACK(__audio_app_sink_eos_callback), data);
		/* Link audioqueue->audio_appsink and save/Give to appsrc of muxer */
		gst_element_set_state(data->audio_appsink, GST_STATE_PLAYING);
		/* one has to set the newly added element to the same state as the rest of the elements. */
	} else if (have_vid_track && g_str_has_prefix(new_pad_type, "video/x-h264")) {
		new_pad_vid_caps = gst_pad_get_current_caps(pad);
		caps = gst_caps_to_string(new_pad_vid_caps);
		g_print("   Video caps :%s\n", caps);
		vid_caps = caps;

		/* Link demuxer with videoqueue */
		ret = gst_pad_link(pad, sink_pad_videoqueue);
		if (GST_PAD_LINK_FAILED(ret))
			g_print("   Type is '%s' but link failed.\n", new_pad_type);
		else
			g_print("   Link succeeded (type '%s').\n", new_pad_type);

		gst_element_link(data->videoqueue, data->video_appsink);
		g_object_set(data->video_appsink, "emit-signals", TRUE, NULL);
		g_signal_connect(data->video_appsink, "new-sample", G_CALLBACK(__video_app_sink_callback), data);
		g_signal_connect(data->video_appsink, "eos", G_CALLBACK(__video_app_sink_eos_callback), data);
		/* Link videoqueue->audio_appsink and save/Give to appsrc of muxer */
		gst_element_set_state(data->video_appsink, GST_STATE_PLAYING);
		/* one has to set the newly added element to the same state as the rest of the elements. */
	} else {
		g_print(" It has type '%s' which is not raw A/V. Ignoring.\n", new_pad_type);
		goto exit;
	}

exit:
	if (new_pad_caps != NULL)
		gst_caps_unref(new_pad_caps);

	gst_object_unref(sink_pad_audioqueue);
	gst_object_unref(sink_pad_videoqueue);
}

/* Demuxer bus_call */
static gboolean __bus_call(GstBus *bus, GstMessage *mesg, gpointer data)
{
	GMainLoop *dmxr_loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE(mesg)) {
	case GST_MESSAGE_EOS:
		g_print("Demuxer: End of stream\n");
		g_main_loop_quit(dmxr_loop);
		break;

	case GST_MESSAGE_ERROR:
	{
		gchar *dbg;
		GError *err;
		gst_message_parse_error(mesg, &err, &dbg);
		g_free(dbg);
		g_printerr("Demuxer-Error: %s\n", err->message);
		g_error_free(err);
		g_main_loop_quit(dmxr_loop);
		break;
	}
	default:
		break;
	}
	return TRUE;
}

/* Demux an mp4 file and generate encoded streams and extra data  */
int _demux_mp4()
{
	CustomData data;
	GMainLoop *loop_dmx;
	GstBus *bus;
	guint watch_id_for_bus;

	g_print("Start of _demux_mp4()\n");

	if (access(file_mp4, F_OK) == -1) {
		/* mp4 file doesn't exist */
		g_print("mp4 Invalid file path.");
		return -1;
	}

	gst_init(NULL, NULL);
	loop_dmx = g_main_loop_new(NULL, FALSE);
	data.loop = loop_dmx;

	/* Create gstreamer elements for demuxer*/
	data.pipeline = gst_pipeline_new("DemuxerPipeline");
	data.source = gst_element_factory_make("filesrc", "file-source");
	data.demuxer = gst_element_factory_make("qtdemux", "mp4-demuxer");
	data.audioqueue = gst_element_factory_make("queue", "audio-queue");
	data.videoqueue = gst_element_factory_make("queue", "video-queue");

	data.dummysink = gst_element_factory_make("fakesink", "fakesink");
	data.video_appsink = gst_element_factory_make("appsink", "video (h264) appsink");
	data.audio_appsink = gst_element_factory_make("appsink", "audio (AAC) appsink");

	if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue
		|| !data.dummysink || !data.videoqueue || !data.audio_appsink || !data.video_appsink) {
		g_print("Test-Suite: One gst-element can't be created. Exiting\n");
		return -1;
	}

	/* Add msg-handler */
	bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
	watch_id_for_bus = gst_bus_add_watch(bus, __bus_call, loop_dmx);
	gst_object_unref(bus);

	/* Add gstreamer-elements into gst-pipeline */
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.demuxer, data.dummysink, \
		data.audioqueue, data.videoqueue, data.audio_appsink, data.video_appsink, NULL);

	/* we set the input filename to the source element */
	g_object_set(G_OBJECT(data.source), "location", file_mp4, NULL);

	/* we link the elements together */
	gst_element_link(data.source, data.demuxer);

	/* Register demuxer callback */
	g_signal_connect(data.demuxer, "pad-added", G_CALLBACK(__on_pad_added), &data);

	/* Play the pipeline */
	g_print("Now playing : %s\n", file_mp4);
	gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

	/* Run the loop till quit */
	g_print("gst-pipeline -Running...\n");
	g_main_loop_run(loop_dmx);

	/* Done with gst-loop. Free resources */
	gst_element_set_state(data.pipeline, GST_STATE_NULL);

	g_print("gst-pipeline - Unreferencing...\n");
	gst_object_unref(GST_OBJECT(data.pipeline));
	g_source_remove(watch_id_for_bus);
	g_main_loop_unref(loop_dmx);
	g_print("End of _demux_mp4()\n");
	return 0;
}



/*-----------------------------------------------------------------------
|    LOCAL FUNCTION                                                                 |
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
	char *op_uri = "MuxTest.mp4";
	int ret = 0;
	g_print("test_mediamuxer_set_data_sink\n");

	/* Set data source after creating */
	ret = mediamuxer_set_data_sink(myMuxer, op_uri, MEDIAMUXER_CONTAINER_FORMAT_MP4);
	if (ret != MEDIAMUXER_ERROR_NONE)
		g_print("mediamuxer_set_data_sink is failed\n");
	return 0;
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
	media_format_set_video_mime(media_format, MEDIA_FORMAT_H264_SP);

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

	/* MEDIA_FORMAT_AAC_LC  MEDIA_FORMAT_AAC_HE  MEDIA_FORMAT_AAC_HE_PS */
	if (media_format_set_audio_mime(media_format_a, MEDIA_FORMAT_AAC_LC) == MEDIA_FORMAT_ERROR_INVALID_OPERATION)
		g_print("Problem during media_format_set_audio_mime operation\n");

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
		media_format_set_audio_bit(media_format_a, 32);
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
		_mediacodec_process_all();
	} else {
		_demux_mp4();
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
			test_mediamuxer_set_data_sink();
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
			g_print("*** input mp4 file path:\n");
			g_print("[This is the file from where demuxed data is fed to muxer]:");
		}
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
			strcpy(file_mp4, cmd);
			g_menu_state = CURRENT_STATUS_MAINMENU;
			break;
		}
	case CURRENT_STATUS_RAW_VIDEO_FILENAME: {	/* "cv" */
			use_video = 1;
			static int codecid = 0;
			_input_raw_filepath(cmd);
			use_encoder = 1;
			codecid = 0x2030; /* video */
			_mediacodec_set_codec(codecid, 5);
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
			_mediacodec_set_venc_info(width, height, fps, target_bits);
			 _mediacodec_prepare();
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
			_input_raw_filepath(cmd);
			use_encoder = 1;
			codecid = 0x1060; /* audio */
			_mediacodec_set_codec(codecid, 9);
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
				_mediacodec_set_aenc_info(samplerate, channel, bit, bitrate);
				 _mediacodec_prepare();
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
