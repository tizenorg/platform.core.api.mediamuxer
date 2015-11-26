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

#ifndef __TIZEN_MEDIAMUXER_H__
#define __TIZEN_MEDIAMUXER_H__

#include <tizen.h>
#include <stdint.h>
#include <media_format.h>
#include <media_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIZEN_ERROR_MEDIA_MUXER
#define TIZEN_ERROR_MEDIA_MUXER -0x05000000
#endif

/**
* @file mediamuxer.h
* @brief This file contains the capi media muxer API.
*/

/**
* @addtogroup CAPI_MEDIAMUXER_MODULE
* @
*/

/**
 * @brief Media Muxer handle type
 * @since_tizen 3.0
 */
typedef struct mediamuxer_s *mediamuxer_h;

/**
 * @brief Enumeration for media muxer state
 * @since_tizen 3.0
 */
typedef enum {
	MEDIAMUXER_STATE_NONE,		/**< The mediamuxer is not created */
	MEDIAMUXER_STATE_IDLE,		/**< The mediamuxer is created, but not prepared */
	MEDIAMUXER_STATE_READY,		/**< The mediamuxer is ready to mux media */
	MEDIAMUXER_STATE_MUXING,	/**< The mediamuxer is muxing media */
	MEDIAMUXER_STATE_PAUSED		/**< The mediamuxer is paused while muxing media */
} mediamuxer_state_e;

/**
 * @brief Enumeration for media muxer error
 * @since_tizen 3.0
 */
typedef enum {
	MEDIAMUXER_ERROR_NONE = TIZEN_ERROR_NONE,					/**< Successful */
	MEDIAMUXER_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY,
	MEDIAMUXER_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER,		/**< Invalid parameter */
	MEDIAMUXER_ERROR_INVALID_OPERATION = TIZEN_ERROR_INVALID_OPERATION,		/**< Invalid operation */
	MEDIAMUXER_ERROR_NOT_SUPPORTED  = TIZEN_ERROR_NOT_SUPPORTED,                     /**< Not supported */
	MEDIAMUXER_ERROR_PERMISSION_DENIED  = TIZEN_ERROR_PERMISSION_DENIED,		/**< Permission denied */
	MEDIAMUXER_ERROR_INVALID_STATE = TIZEN_ERROR_MEDIA_MUXER | 0x01,		/**< Invalid state */
	MEDIAMUXER_ERROR_INVALID_PATH = TIZEN_ERROR_MEDIA_MUXER | 0x02,			/**< Invalid path */
	MEDIAMUXER_ERROR_RESOURCE_LIMIT = TIZEN_ERROR_MEDIA_MUXER | 0x03		/**< Resource limit */
} mediamuxer_error_e;

/**
 * @brief Enumeration for media muxer output format
 * @since_tizen 3.0
 */
typedef enum {
	MEDIAMUXER_CONTAINER_FORMAT_MP4 = MEDIA_FORMAT_CONTAINER_MP4,	/**< The mediamuxer output format is MP4 container */
	MEDIAMUXER_CONTAINER_FORMAT_3GP = MEDIA_FORMAT_CONTAINER_3GP,	/**< The mediamuxer output format is 3GP container */
	MEDIAMUXER_CONTAINER_FORMAT_WAV = MEDIA_FORMAT_CONTAINER_WAV	/**< The mediamuxer output format is WAV container */
} mediamuxer_output_format_e;

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
typedef void (*mediamuxer_error_cb)(mediamuxer_error_e error, void *user_data);

/**
 * @brief Creates a media muxer handle for muxing.
 * @since_tizen 3.0
 * @param[out] muxer  A new handle to media muxer
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @post The media muxer state will be #MEDIAMUXER_STATE_IDLE.
 * @see mediamuxer_destroy()
 */
int mediamuxer_create(mediamuxer_h *muxer);

/**
 * @brief Sets the sink path of output stream.
 * @since_tizen 3.0
 * @remarks The mediastorage privilege(http://tizen.org/privilege/mediastorage) should be added if any video/audio files are to be saved in the internal storage.
 * @remarks The externalstorage privilege(http://tizen.org/privilege/externalstorage) should be added if any video/audio files are to be saved in the external storage.
 * @param[in] muxer  A new handle to media muxer
 * @param[in] path  The location of the output media file, such as the file path
		This is the path at which the muxed file should be saved.
 * @param[in] format  The format of the output media file
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @retval #MEDIAMUXER_ERROR_PERMISSION_DENIED Permission denied
 * @retval #MEDIAMUXER_ERROR_INVALID_PATH Invalid path
 * @pre The media muxer state will be #MEDIAMUXER_STATE_IDLE by calling mediamuxer_create
 * @see #mediamuxer_output_format_e
 */
int mediamuxer_set_data_sink(mediamuxer_h muxer, char *path, mediamuxer_output_format_e format);

/**
 * @brief Adds the media track of interest to the muxer handle.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @param[in] media_format  The format of media muxer
 * @param[out] track_index  The index of the media track
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_IDLE.
 * @see #media_format_h
 * @see mediamuxer_create()
 * @see mediamuxer_prepare()
 * */
int mediamuxer_add_track(mediamuxer_h muxer, media_format_h media_format, int *track_index);

/**
 * @brief Prepares the media muxer.
 * @remarks Initiates the necessary parameters.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_IDLE.
 * @post The media muxer state will be #MEDIAMUXER_STATE_READY.
 * @see mediamuxer_create()
 * @see mediamuxer_unprepare()
 * */
int mediamuxer_prepare(mediamuxer_h muxer);

/**
 * @brief Starts the media muxer.
 * @remarks Keeps the muxer ready for writing data.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_READY.
 * @post The media muxer state will be #MEDIAMUXER_STATE_MUXING.
 * @see mediamuxer_prepare()
 * @see mediamuxer_stop()
 * */
int mediamuxer_start(mediamuxer_h muxer);

/**
 * @brief Writes the media packet of interest to the muxer handle.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @param[in] track_index  The index of the media track
 * @param[in] inbuf  The packet of media muxer
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_READY by calling mediamuxer_prepare() or
 *      set to #MEDIAMUXER_STATE_PAUSED by calling mediamuxer_pause().
 * @post The media muxer state will be #MEDIAMUXER_STATE_MUXING.
 * @see mediamuxer_prepare()
 * @see mediamuxer_close_track()
 * @see mediamuxer_pause()
 * @see #media_packet_h
 * */
int mediamuxer_write_sample(mediamuxer_h muxer, int track_index, media_packet_h inbuf);

/**
 * @brief Closes the track from further writing of data.
 * @remarks For each added track, user needs to call this API to indicate the end of stream.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @param[in] track_index the selected track index
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_MUXING.
 * @see mediamuxer_write_sample()
 * @see mediamuxer_pause()
 * @see mediamuxer_unprepare()
 * @see #mediamuxer_error_e
 * */
int mediamuxer_close_track(mediamuxer_h muxer, int track_index);

/**
 * @brief Pauses the media muxer.
 * @remarks To temporarily disable writing data for muxing. This API pauses a playing muxer
		If the prior state of the muxer is not in PLAYING, no action will be taken.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_MUXING.
 * @post The media muxer state will be #MEDIAMUXER_STATE_PAUSED.
 * @see mediamuxer_write_sample()
 * @see mediamuxer_resume()
 * */
int mediamuxer_pause(mediamuxer_h muxer);

/**
 * @brief Resumes the media muxer.
 * @remarks Make it ready for any further writing. This API will resume a paused muxer.
		If the prior state of the muxer is not playing, no action will be taken.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_PAUSED.
 * @post The media muxer state will be #MEDIAMUXER_STATE_MUXING.
 * @see mediamuxer_pause()
 * */
int mediamuxer_resume(mediamuxer_h muxer);

/**
 * @brief Stops the media muxer.
 * @remarks Keeps the muxer ready for writing data.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_MUXING
 *	or #MEDIAMUXER_STATE_PAUSED.
 * @post The media muxer state will be #MEDIAMUXER_STATE_READY.
 * @see mediamuxer_start()
 * @see mediamuxer_unprepare()
 * */
int mediamuxer_stop(mediamuxer_h muxer);

/**
 * @brief Unprepares the media muxer.
 * @remarks Unrefs the variables created after calling mediamuxer_prepare().
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre The media muxer state must be set to #MEDIAMUXER_STATE_READY or
 *      set to #MEDIAMUXER_STATE_PAUSED by calling mediamuxer_pause().
 * @post The media muxer state will be #MEDIAMUXER_STATE_IDLE.
 * @see mediamuxer_write_sample()
 * @see mediamuxer_pause()
 * @see mediamuxer_destroy()
 * */
int mediamuxer_unprepare(mediamuxer_h muxer);

/**
 * @brief Removes the instance of media muxer and clear all its context memory.
 * @since_tizen 3.0
 * @param[in] muxer     The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_STATE Invalid state
 * @pre Create a media muxer handle by calling mediamuxer_create() function.
 * @post The media muxer state will be #MEDIAMUXER_STATE_NONE.
 * @see mediamuxer_create()
 * */
int mediamuxer_destroy(mediamuxer_h muxer);

/**
 * @brief Gets media muxer state.
 * @since_tizen 3.0
 * @param[in] muxer   The media muxer handle
 * @param[out] state   The media muxer sate
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #MEDIAMUXER_ERROR_INVALID_OPERATION Invalid operation
 * @pre Create a media muxer handle by calling mediamuxer_create() function.
 * @see #mediamuxer_state_e
 * */
int mediamuxer_get_state(mediamuxer_h muxer, mediamuxer_state_e *state);

/**
 * @brief Registers a error callback function to be invoked when an error occurs.
 * @details Following error codes can be delivered.
 *          #MEDIAMUXER_ERROR_INVALID_OPERATION,
 *          #MEDIAMUXER_ERROR_NOT_SUPPORTED,
 *          #MEDIAMUXER_ERROR_INVALID_PATH,
 *          #MEDIAMUXER_ERROR_RESOURCE_LIMIT
 * @since_tizen 3.0
 * @param[in] muxer   The media muxer handle
 * @param[in] callback  Callback function pointer
 * @param[in] user_data   The user data passed from the code where
 *                         mediamuxer_set_error_cb() was invoked
 *                         This data will be accessible from @a mediamuxer_error_cb
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @pre Create a media muxer handle by calling mediamuxer_create() function.
 * @post mediamuxer_error_cb() will be invoked.
 * @see mediamuxer_unset_error_cb()
 * @see mediamuxer_error_cb()
 * */
int mediamuxer_set_error_cb(mediamuxer_h muxer, mediamuxer_error_cb callback, void* user_data);

/**
 * @brief Unregisters the error callback function.
 * @since_tizen 3.0
 * @param[in] muxer   The media muxer handle
 * @return @c 0 on success, otherwise a negative error value
 * @retval #MEDIAMUXER_ERROR_NONE Successful
 * @retval #MEDIAMUXER_ERROR_INVALID_PARAMETER Invalid parameter
 * @see mediamuxer_error_cb()
 * */
int mediamuxer_unset_error_cb(mediamuxer_h muxer);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIAMUXER_H__ */
