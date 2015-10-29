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

#ifndef __TIZEN_MEDIAMUXER_ERROR_H__
#define __TIZEN_MEDIAMUXER_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif
/**< Definition of number describing error group */
#define MX_ERROR_CLASS              0x80000000
/**< Category for describing common error group */
#define MX_ERROR_COMMON_CLASS       0x80000100
/**< Category for describing gst_port error group */
#define MX_ERROR_GST_PORT_CLASS     0x80000200
/**< Category for describing ffmpeg port error group */
#define MX_ERROR_FFMPEG_PORT_CLASS  0x80000300
/**< Category for describing custom error group */
#define MX_ERROR_CUSTOM_PORT_CLASS       0x80000400

/*
   MX_ERROR_CLASS
 */
/**< Unclassified error */
#define MX_ERROR_UNKNOWN            (MX_ERROR_CLASS | 0x00)
/**< Invalid argument */
#define MX_ERROR_INVALID_ARGUMENT       (MX_ERROR_CLASS | 0x01)
/**< Out of memory */
#define MX_ERROR_OUT_OF_MEMORY          (MX_ERROR_CLASS | 0x02)
/**< Out of storage */
#define MX_ERROR_OUT_OF_STORAGE         (MX_ERROR_CLASS | 0x03)
/**< Invalid handle */
#define MX_ERROR_INVALID_HANDLE         (MX_ERROR_CLASS | 0x04)
/**< Cannot find file */
#define MX_ERROR_FILE_NOT_FOUND         (MX_ERROR_CLASS | 0x05)
/**< Fail to read data from file */
#define MX_ERROR_FILE_READ          (MX_ERROR_CLASS | 0x06)
/**< Fail to write data to file */
#define MX_ERROR_FILE_WRITE         (MX_ERROR_CLASS | 0x07)
/**< End of file */
#define MX_ERROR_END_OF_FILE        (MX_ERROR_CLASS | 0x08)
/**< Not supported API */
#define MX_ERROR_NOT_SUPPORT_API        (MX_ERROR_CLASS | 0x09)
/**< port regitstration failed error */
#define MX_ERROR_PORT_REG_FAILED        (MX_ERROR_CLASS | 0x0a)

/*
   MX_ERROR_COMMON_CLASS
 */
/**< Invalid argument */
#define MX_ERROR_COMMON_INVALID_ARGUMENT    (MX_ERROR_COMMON_CLASS | 1)
/**< Out of storage */
#define MX_ERROR_COMMON_NO_FREE_SPACE       (MX_ERROR_COMMON_CLASS | 2)
/**< Out of memory */
#define MX_ERROR_COMMON_OUT_OF_MEMORY       (MX_ERROR_COMMON_CLASS | 3)
/**< Unknown error */
#define MX_ERROR_COMMON_UNKNOWN             (MX_ERROR_COMMON_CLASS | 4)
/**< Invalid argument */
#define MX_ERROR_COMMON_INVALID_ATTRTYPE    (MX_ERROR_COMMON_CLASS | 5)
/**< Invalid permission */
#define MX_ERROR_COMMON_INVALID_PERMISSION  (MX_ERROR_COMMON_CLASS | 6)
/**< Out of array */
#define MX_ERROR_COMMON_OUT_OF_ARRAY        (MX_ERROR_COMMON_CLASS | 7)
/**< Out of value range */
#define MX_ERROR_COMMON_OUT_OF_RANGE        (MX_ERROR_COMMON_CLASS | 8)
/**< Attribute doesn't exist. */
#define MX_ERROR_COMMON_ATTR_NOT_EXIST      (MX_ERROR_COMMON_CLASS | 9)

/*
 *      MX_ERROR_GST_PORT_CLASS
 */
/**< GST Port  instance is not initialized */
#define MX_ERROR_GST_PORT_NOT_INITIALIZED   (MX_ERROR_GST_PORT_CLASS | 0x01)

/*
   MX_ERROR_FFMPEG_PORT_CLASS
 */
/**< FFMPEG Port instance is not initialized */
#define MX_ERROR_FFMPEG_PORT_NOT_INITIALIZED (MX_ERROR_FFMPEG_PORT_CLASS | 0x01)

/*
   MX_ERROR_CUSTOM_PORT_CLASS
 */
/**< CUSTOM Port instance is not initialized */
#define MX_ERROR_CUSTOM_PORT_NOT_INITIALIZED  MX_ERROR_CUSTOM_PORT_CLASS | 0x01)

typedef enum {
	MX_ERROR_NONE = 0,
	MX_ERROR = -1,			/**< muxer happens error */
	MX_MEMORY_ERROR = -2,		/**< muxer memory is not enough */
	MX_PARAM_ERROR = -3,		/**< muxer parameter is error */
	MX_INVALID_ARG = -4,		/**< muxer has invalid arguments */
	MX_PERMISSION_DENIED = -5,
	MX_INVALID_STATUS = -6,		/**< muxer works at invalid status */
	MX_NOT_SUPPORTED = -7,		/**< muxer can't support this specific video format */
	MX_INVALID_IN_BUF = -8,
	MX_INVALID_OUT_BUF = -9,
	MX_INTERNAL_ERROR = -10,
	MX_HW_ERROR = -11,		/**< muxer happens hardware error */
	MX_NOT_INITIALIZED = -12,
	MX_INVALID_STREAM = -13,
	MX_OUTPUT_BUFFER_EMPTY = -14,
	MX_OUTPUT_BUFFER_OVERFLOW = -15,/**< muxer output buffer is overflow */
	MX_MEMORY_ALLOCED = -16,	/**< muxer has got memory and can decode one frame */
	MX_COURRPTED_INI = -17,		/**< value in the ini file is not valid */
} mx_ret_e;

#ifdef __cplusplus
}
#endif
#endif /* __TIZEN_MEDIAMUXER_ERROR_H__ */
