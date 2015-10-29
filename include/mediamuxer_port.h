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

#ifndef __TIZEN_MEDIAMUXER_PORT_H__
#define __TIZEN_MEDIAMUXER_PORT_H__

/*=============================================================================
|                                                                              |
|  INCLUDE FILES                                                               |
|                                                                              |
==============================================================================*/

#include <glib.h>
#include <mm_types.h>
#include <mm_message.h>
#include <mediamuxer_util.h>
#include <mediamuxer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
    @addtogroup MEDIAMUXER
    @{

    @par
    This part describes APIs used for playback of multimedia contents.
    All multimedia contents are created by a media muxer through handle of
    playback. In creating a muxer, it displays the muxer's status or information
    by registering callback function.

    @par
    In case of streaming playback, network has to be opend by using datanetwork
    API. If proxy, cookies and the other attributes for streaming playback are
    needed, set those attributes using mx_set_attribute() before create muxer.

    @par
    The subtitle for local video playback is supported. Set "subtitle_uri"
    attribute using mx_set_attribute() before the application creates the muxer.
    Then the application could receive MMMessageParamType which includes
    subtitle string and duration.

    @par
    MediaMuxer can have 5 states, and each state can be changed by calling
    described functions on "Figure1. State of MediaMuxer".

    @par
    @image html	 muxer_state.jpg "Figure1. State of MediaMuxer"	width=12cm
    @image latex muxer_state.jpg "Figure1. State of MediaMuxer"	width=12cm

    @par
    Most of functions which change muxer state work as synchronous. But,
    mx_prepare() should be used asynchronously. Both mx_pause() and mx__resume()
    should also be used asynchronously in the case of streaming data.
    So, application have to confirm the result of those APIs through message
    callback function.

    @par
    Note that "None" and Null" state could be reached from any state
    by calling mx_destroy() and mx_unrealize().

    @par
    <div><table>
    <tr>
    <td><B>FUNCTION</B></td>
    <td><B>PRE-STATE</B></td>
    <td><B>POST-STATE</B></td>
    <td><B>SYNC TYPE</B></td>
    </tr>
    <tr>
    <td>mx_create()</td>
    <td>NONE</td>
    <td>NULL</td>
    <td>SYNC</td>
    </tr>
    <tr>
    <td>mx_destroy()</td>
    <td>NULL</td>
    <td>NONE</td>
    <td>SYNC</td>
    </tr>
    <tr>
    <td>mx_set_data_sink()</td>
    <td>NULL</td>
    <td>READY</td>
    <td>SYNC</td>
    </tr>
    </table></div>

    @par
    Following are the attributes supported in muxer which may be set after
    initialization. Those are handled as a string.

    @par
    <div><table>
    <tr>
    <td>PROPERTY</td>
    <td>TYPE</td>
    <td>VALID TYPE</td>
    </tr>
    <tr>
    <td>"profile_uri"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_duration"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_video_width"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_video_height"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"profile_user_param"</td>
    <td>data</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"profile_play_count"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_type"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_udp_timeout"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_user_agent"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_wap_profile"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_network_bandwidth"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"streaming_cookie"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_proxy_ip"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"streaming_proxy_port"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"subtitle_uri"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    </table></div>

    @par
    Following attributes are supported for playing stream data. Those value can
    be readable only and valid after starting playback.
    Please use mm_fileinfo for local playback.

    @par
    <div><table>
    <tr>
    <td>PROPERTY</td>
    <td>TYPE</td>
    <td>VALID TYPE</td>
    </tr>
    <tr>
    <td>"content_video_found"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_video_codec"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_video_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_audio_found"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_audio_codec"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"content_audio_bitrate"</td>
    <td>int</td>
    <td>array</td>
    </tr>
    <tr>
    <td>"content_audio_channels"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_audio_samplerate"</td>
    <td>int</td>
    <td>array</td>
    </tr>
    <tr>
    <td>"content_audio_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"content_text_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    <tr>
    <td>"tag_artist"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_title"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_album"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_genre"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_author"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_copyright"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_date"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_description"</td>
    <td>string</td>
    <td>N/A</td>
    </tr>
    <tr>
    <td>"tag_track_num"</td>
    <td>int</td>
    <td>range</td>
    </tr>
    </table></div>

 */

/*=============================================================================|
|                                                                              |
|  GLOBAL DEFINITIONS AND DECLARATIONS                                         |
|                                                                              |
==============================================================================*/
/**
 * @brief Called when error occurs in media muxer.
 * @details Following error codes can be delivered.
 *          #MEDIAMUXER_ERROR_INVALID_OPERATION,
 *          #MEDIAMUXER_ERROR_NOT_SUPPORTED,
 *          #MEDIAMUXER_ERROR_INVALID_PATH,
 *          #MEDIAMUXER_ERROR_RESOURCE_LIMIT
 * @since_tizen 3.0
 * @param[in] error  The error that occurred in media muxer
 * @param[in] user_data   The user data passed from the code where
 *                         mediamuxer_set_error_cb() was invoked
 *                         This data will be accessible from @a mediamuxer_error_cb
 * @pre Create media muxer handle by calling mediamuxer_create() function.
 * @see mediamuxer_set_error_cb()
 * @see mediamuxer_unset_error_cb()
 */
typedef void (*mx_error_cb)(mediamuxer_error_e error, void *user_data);

/**
 * Attribute validity structure
 */
typedef struct _media_port_muxer_ops {
	unsigned int n_size;
	int (*init)(MMHandleType *pHandle);
	/* Add new ops at the end of structure, no order change */
	int (*set_data_sink)(MMHandleType pHandle, char *uri, mediamuxer_output_format_e format);
	int (*add_track)(MMHandleType pHandle, media_format_h media_format, int *track_index);
	int (*prepare)(MMHandleType pHandle);
	int (*start)(MMHandleType pHandle);
	int (*write_sample)(MMHandleType pHandle, int track_index, media_packet_h inbuf);
	int (*close_track)(MMHandleType pHandle, int track_index);
	int (*pause)(MMHandleType pHandle);
	int (*resume)(MMHandleType pHandle);
	int (*unprepare)(MMHandleType pHandle);
	int (*stop)(MMHandleType pHandle);
	int (*destroy)(MMHandleType pHandle);
	int (*set_error_cb)(MMHandleType pHandle, mx_error_cb callback, void* user_data);
} media_port_muxer_ops;

/*=============================================================================
|                                                                              |
|  GLOBAL FUNCTION PROTOTYPES                                                  |
|                                                                              |
==============================================================================*/

/**
 * This function creates a muxer object for parsing multimedia contents. \n
 * The attributes of muxer are created to get/set some values with application.
 * And, proper port is selected to do the actual parsing of the mdia.
 *
 * @param   muxer [out]   Handle of muxer
 * @param   op_uri        uri at which muxed file should be stored
 * @param   format        container format of the muxed data
 *
 * @return  This function returns zero on success, or negative value with error
 *	     code. Please refer 'mx_error.h' to know it in detail.
 *
 * @par Example
 * @code
  mx_create(&muxer, char *op_uri, mediamuxer_output_format_e format);
  ...
  mx_destroy(&muxer);
 * @endcode
 */
int mx_create(MMHandleType *muxer);

/**
 * This function sets the input data source to parse. \n
 * The source can be a local file or remote
 *
 * @param   muxer	Handle of muxer
 * @param   uri	uri at which muxed file should be stored
 * @param   format	container format of the muxed data
 *
 * @return  This function returns zero on success, or negative value with error
 *	     code. Please refer 'mx_error.h' to know it in detail.
 *
 * @par Example
 * @code
    if (mx_set_data_sink(muxer, uri, format) != MM_ERROR_NONE)
	{
		MX_E("failed to set the source \n");
	}
 * @endcode
 */
int mx_set_data_sink(MMHandleType muxer, char *uri, mediamuxer_output_format_e format);

/**
 * This function releases muxer object and all resources which were created by
 *	mx_create(). And, muxer handle will also be destroyed.
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
 *		code.
 * @see     mx_destroy
 *
 * @par Example
 * @code
if (mx_destroy(g_muxer) != MM_ERROR_NONE)
{
    MX_E("failed to destroy muxer\n");
}
 * @endcode
 */
int mx_destroy(MMHandleType muxer);

/**
 * This function starts/prepares the muxer object. \n
 * For GST-port, this function creates necessary gst elemetns
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_create
 *
 * @par Example
 * @code
if (mx_prepare(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to prepare muxer\n");
}
 * @endcode
 */
int mx_prepare(MMHandleType muxer);

/**
 * This function starts the muxer object. \n
 * For GST-port, this function sets the pipeline to playing
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_stop
 *
 * @par Example
 * @code
if (mx_start(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to start muxer\n");
}
 * @endcode
 */
int mx_start(MMHandleType muxer);

/**
 * This function adds Audio/Vidoe/Subtitle track in muxer handle \n
 *
 * @param   muxer     [in]    Handle of muxer
 *          media_format  [in]   media format of A/V/S
 *          track_index  [out]   index of the media track
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_create
 *
 * @par Example
 * @code
if (mx_add_track(g_muxer,media_format,&track_index) != MX_ERROR_NONE)
{
    MX_E("failed to add track to muxer handle\n");
}
 * @endcode
 */
int mx_add_track(MMHandleType muxer, media_format_h media_format, int *track_index);

/**
 * This function writes Audio/Vidoe/Subtitle track to file using muxer handle \n
 *
 * @param   muxer     [in]    Handle of muxer
 *          track_index       index of the media track
 *	    inbuf            buffer to be muxed
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_write_sample
 *
 * @par Example
 * @code
if (mx_write_sample(g_muxer,track_id, inbuf) != MX_ERROR_NONE)
{
    MX_E("failed to write sample\n");
}
 * @endcode
 */
int mx_write_sample(MMHandleType mediamuxer, int track_index, media_packet_h inbuf);

/**
 * This function close the selected track. \n
 * For GST-port, this function send the eos to the specific appsrc element
 *
 * @param   muxer     [in]    Handle of muxer
 * @param   track_index [in]  selected track
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_unprepare
 *
 * @par Example
 * @code
if (mx_close_track(g_muxer,1) != MX_ERROR_NONE)
{
    MX_E("failed to close audio track\n");
}
 * @endcode
 */
int mx_close_track(MMHandleType mediamuxer, int track_index);

/**
 * This function stops the muxer object. \n
 * For GST-port, this function sets the pipeline to ready
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_start
 *
 * @par Example
 * @code
if (mx_stop(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to stop muxer\n");
}
 * @endcode
 */
int mx_stop(MMHandleType mediamuxer);

/**
 * This function stops/un-prepares the muxer object. \n
 * For GST-port, this function unrefs necessary gst elemetns
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_unprepare
 *
 * @par Example
 * @code
if (mx_unprepare(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to unprepare muxer\n");
}
 * @endcode
 */
int mx_unprepare(MMHandleType mediamuxer);

/**
 * This function pauses the muxing operation. \n
 * For GST-port, this function pauses the pipeline
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_resume
 *
 * @par Example
 * @code
if (mx_pause(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to pause muxer\n");
}
 * @endcode
 */
int mx_pause(MMHandleType mediamuxer);

/**
 * This function resumes the muxing operation. \n
 * For GST-port, this function sets the pipeline back to playing
 *
 * @param   muxer     [in]    Handle of muxer
 *
 * @return  This function returns zero on success, or negative value with error
		code.
 * @see     mx_pause
 *
 * @par Example
 * @code
if (mx_resume(g_muxer) != MX_ERROR_NONE)
{
    MX_E("failed to resume muxer\n");
}
 * @endcode
 */
int mx_resume(MMHandleType mediamuxer);

/**
 * This function is to set error call back function
 *
 * @param   muxer       [in]    Handle of muxer
 * @param   callback    [in]    call back function pointer
 * @param   user_data   [in]    user specific data pointer
 *
 * @return  This function returns zero on success, or negative value with error code.
 */
int mx_set_error_cb(MMHandleType muxer,	mediamuxer_error_cb callback, void* user_data);

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIAMUXER_PORT_H__ */
