#pragma once
// MESSAGE SETTINGS PACKING

#define MAVLINK_MSG_ID_SETTINGS 155

MAVPACKED(
typedef struct __mavlink_settings_t {
 uint8_t image_quality; /*<  image quality*/
 uint8_t image_contrast; /*<  image contrast*/
 uint8_t image_brightness; /*<  image_brightness*/
 uint8_t left_engine_power; /*<  left engine power*/
 uint8_t right_engine_power; /*<  right engine power*/
 uint8_t selected_tracker; /*<  selected tracker*/
 char video_source[249]; /*<  video source path*/
}) mavlink_settings_t;

#define MAVLINK_MSG_ID_SETTINGS_LEN 255
#define MAVLINK_MSG_ID_SETTINGS_MIN_LEN 255
#define MAVLINK_MSG_ID_155_LEN 255
#define MAVLINK_MSG_ID_155_MIN_LEN 255

#define MAVLINK_MSG_ID_SETTINGS_CRC 249
#define MAVLINK_MSG_ID_155_CRC 249

#define MAVLINK_MSG_SETTINGS_FIELD_VIDEO_SOURCE_LEN 249

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_SETTINGS { \
    155, \
    "SETTINGS", \
    7, \
    {  { "image_quality", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_settings_t, image_quality) }, \
         { "image_contrast", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_settings_t, image_contrast) }, \
         { "image_brightness", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_settings_t, image_brightness) }, \
         { "left_engine_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_settings_t, left_engine_power) }, \
         { "right_engine_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_settings_t, right_engine_power) }, \
         { "selected_tracker", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_settings_t, selected_tracker) }, \
         { "video_source", NULL, MAVLINK_TYPE_CHAR, 249, 6, offsetof(mavlink_settings_t, video_source) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_SETTINGS { \
    "SETTINGS", \
    7, \
    {  { "image_quality", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_settings_t, image_quality) }, \
         { "image_contrast", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_settings_t, image_contrast) }, \
         { "image_brightness", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_settings_t, image_brightness) }, \
         { "left_engine_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_settings_t, left_engine_power) }, \
         { "right_engine_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_settings_t, right_engine_power) }, \
         { "selected_tracker", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_settings_t, selected_tracker) }, \
         { "video_source", NULL, MAVLINK_TYPE_CHAR, 249, 6, offsetof(mavlink_settings_t, video_source) }, \
         } \
}
#endif

/**
 * @brief Pack a settings message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param image_quality  image quality
 * @param image_contrast  image contrast
 * @param image_brightness  image_brightness
 * @param left_engine_power  left engine power
 * @param right_engine_power  right engine power
 * @param selected_tracker  selected tracker
 * @param video_source  video source path
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_settings_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t image_quality, uint8_t image_contrast, uint8_t image_brightness, uint8_t left_engine_power, uint8_t right_engine_power, uint8_t selected_tracker, const char *video_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SETTINGS_LEN];
    _mav_put_uint8_t(buf, 0, image_quality);
    _mav_put_uint8_t(buf, 1, image_contrast);
    _mav_put_uint8_t(buf, 2, image_brightness);
    _mav_put_uint8_t(buf, 3, left_engine_power);
    _mav_put_uint8_t(buf, 4, right_engine_power);
    _mav_put_uint8_t(buf, 5, selected_tracker);
    _mav_put_char_array(buf, 6, video_source, 249);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SETTINGS_LEN);
#else
    mavlink_settings_t packet;
    packet.image_quality = image_quality;
    packet.image_contrast = image_contrast;
    packet.image_brightness = image_brightness;
    packet.left_engine_power = left_engine_power;
    packet.right_engine_power = right_engine_power;
    packet.selected_tracker = selected_tracker;
    mav_array_memcpy(packet.video_source, video_source, sizeof(char)*249);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SETTINGS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SETTINGS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
}

/**
 * @brief Pack a settings message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param image_quality  image quality
 * @param image_contrast  image contrast
 * @param image_brightness  image_brightness
 * @param left_engine_power  left engine power
 * @param right_engine_power  right engine power
 * @param selected_tracker  selected tracker
 * @param video_source  video source path
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_settings_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t image_quality,uint8_t image_contrast,uint8_t image_brightness,uint8_t left_engine_power,uint8_t right_engine_power,uint8_t selected_tracker,const char *video_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SETTINGS_LEN];
    _mav_put_uint8_t(buf, 0, image_quality);
    _mav_put_uint8_t(buf, 1, image_contrast);
    _mav_put_uint8_t(buf, 2, image_brightness);
    _mav_put_uint8_t(buf, 3, left_engine_power);
    _mav_put_uint8_t(buf, 4, right_engine_power);
    _mav_put_uint8_t(buf, 5, selected_tracker);
    _mav_put_char_array(buf, 6, video_source, 249);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SETTINGS_LEN);
#else
    mavlink_settings_t packet;
    packet.image_quality = image_quality;
    packet.image_contrast = image_contrast;
    packet.image_brightness = image_brightness;
    packet.left_engine_power = left_engine_power;
    packet.right_engine_power = right_engine_power;
    packet.selected_tracker = selected_tracker;
    mav_array_memcpy(packet.video_source, video_source, sizeof(char)*249);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SETTINGS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SETTINGS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
}

/**
 * @brief Encode a settings struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param settings C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_settings_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_settings_t* settings)
{
    return mavlink_msg_settings_pack(system_id, component_id, msg, settings->image_quality, settings->image_contrast, settings->image_brightness, settings->left_engine_power, settings->right_engine_power, settings->selected_tracker, settings->video_source);
}

/**
 * @brief Encode a settings struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param settings C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_settings_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_settings_t* settings)
{
    return mavlink_msg_settings_pack_chan(system_id, component_id, chan, msg, settings->image_quality, settings->image_contrast, settings->image_brightness, settings->left_engine_power, settings->right_engine_power, settings->selected_tracker, settings->video_source);
}

/**
 * @brief Send a settings message
 * @param chan MAVLink channel to send the message
 *
 * @param image_quality  image quality
 * @param image_contrast  image contrast
 * @param image_brightness  image_brightness
 * @param left_engine_power  left engine power
 * @param right_engine_power  right engine power
 * @param selected_tracker  selected tracker
 * @param video_source  video source path
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_settings_send(mavlink_channel_t chan, uint8_t image_quality, uint8_t image_contrast, uint8_t image_brightness, uint8_t left_engine_power, uint8_t right_engine_power, uint8_t selected_tracker, const char *video_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SETTINGS_LEN];
    _mav_put_uint8_t(buf, 0, image_quality);
    _mav_put_uint8_t(buf, 1, image_contrast);
    _mav_put_uint8_t(buf, 2, image_brightness);
    _mav_put_uint8_t(buf, 3, left_engine_power);
    _mav_put_uint8_t(buf, 4, right_engine_power);
    _mav_put_uint8_t(buf, 5, selected_tracker);
    _mav_put_char_array(buf, 6, video_source, 249);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SETTINGS, buf, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
#else
    mavlink_settings_t packet;
    packet.image_quality = image_quality;
    packet.image_contrast = image_contrast;
    packet.image_brightness = image_brightness;
    packet.left_engine_power = left_engine_power;
    packet.right_engine_power = right_engine_power;
    packet.selected_tracker = selected_tracker;
    mav_array_memcpy(packet.video_source, video_source, sizeof(char)*249);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SETTINGS, (const char *)&packet, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
#endif
}

/**
 * @brief Send a settings message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_settings_send_struct(mavlink_channel_t chan, const mavlink_settings_t* settings)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_settings_send(chan, settings->image_quality, settings->image_contrast, settings->image_brightness, settings->left_engine_power, settings->right_engine_power, settings->selected_tracker, settings->video_source);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SETTINGS, (const char *)settings, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
#endif
}

#if MAVLINK_MSG_ID_SETTINGS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_settings_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t image_quality, uint8_t image_contrast, uint8_t image_brightness, uint8_t left_engine_power, uint8_t right_engine_power, uint8_t selected_tracker, const char *video_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, image_quality);
    _mav_put_uint8_t(buf, 1, image_contrast);
    _mav_put_uint8_t(buf, 2, image_brightness);
    _mav_put_uint8_t(buf, 3, left_engine_power);
    _mav_put_uint8_t(buf, 4, right_engine_power);
    _mav_put_uint8_t(buf, 5, selected_tracker);
    _mav_put_char_array(buf, 6, video_source, 249);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SETTINGS, buf, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
#else
    mavlink_settings_t *packet = (mavlink_settings_t *)msgbuf;
    packet->image_quality = image_quality;
    packet->image_contrast = image_contrast;
    packet->image_brightness = image_brightness;
    packet->left_engine_power = left_engine_power;
    packet->right_engine_power = right_engine_power;
    packet->selected_tracker = selected_tracker;
    mav_array_memcpy(packet->video_source, video_source, sizeof(char)*249);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SETTINGS, (const char *)packet, MAVLINK_MSG_ID_SETTINGS_MIN_LEN, MAVLINK_MSG_ID_SETTINGS_LEN, MAVLINK_MSG_ID_SETTINGS_CRC);
#endif
}
#endif

#endif

// MESSAGE SETTINGS UNPACKING


/**
 * @brief Get field image_quality from settings message
 *
 * @return  image quality
 */
static inline uint8_t mavlink_msg_settings_get_image_quality(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field image_contrast from settings message
 *
 * @return  image contrast
 */
static inline uint8_t mavlink_msg_settings_get_image_contrast(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field image_brightness from settings message
 *
 * @return  image_brightness
 */
static inline uint8_t mavlink_msg_settings_get_image_brightness(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field left_engine_power from settings message
 *
 * @return  left engine power
 */
static inline uint8_t mavlink_msg_settings_get_left_engine_power(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field right_engine_power from settings message
 *
 * @return  right engine power
 */
static inline uint8_t mavlink_msg_settings_get_right_engine_power(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field selected_tracker from settings message
 *
 * @return  selected tracker
 */
static inline uint8_t mavlink_msg_settings_get_selected_tracker(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field video_source from settings message
 *
 * @return  video source path
 */
static inline uint16_t mavlink_msg_settings_get_video_source(const mavlink_message_t* msg, char *video_source)
{
    return _MAV_RETURN_char_array(msg, video_source, 249,  6);
}

/**
 * @brief Decode a settings message into a struct
 *
 * @param msg The message to decode
 * @param settings C-struct to decode the message contents into
 */
static inline void mavlink_msg_settings_decode(const mavlink_message_t* msg, mavlink_settings_t* settings)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    settings->image_quality = mavlink_msg_settings_get_image_quality(msg);
    settings->image_contrast = mavlink_msg_settings_get_image_contrast(msg);
    settings->image_brightness = mavlink_msg_settings_get_image_brightness(msg);
    settings->left_engine_power = mavlink_msg_settings_get_left_engine_power(msg);
    settings->right_engine_power = mavlink_msg_settings_get_right_engine_power(msg);
    settings->selected_tracker = mavlink_msg_settings_get_selected_tracker(msg);
    mavlink_msg_settings_get_video_source(msg, settings->video_source);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_SETTINGS_LEN? msg->len : MAVLINK_MSG_ID_SETTINGS_LEN;
        memset(settings, 0, MAVLINK_MSG_ID_SETTINGS_LEN);
    memcpy(settings, _MAV_PAYLOAD(msg), len);
#endif
}
