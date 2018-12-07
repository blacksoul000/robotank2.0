#pragma once
// MESSAGE TRACKING_TARGET_RECT PACKING

#define MAVLINK_MSG_ID_TRACKING_TARGET_RECT 153

MAVPACKED(
typedef struct __mavlink_tracking_target_rect_t {
 float x; /*<  x*/
 float y; /*<  y*/
 float width; /*<  width*/
 float height; /*<  height*/
}) mavlink_tracking_target_rect_t;

#define MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN 16
#define MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN 16
#define MAVLINK_MSG_ID_153_LEN 16
#define MAVLINK_MSG_ID_153_MIN_LEN 16

#define MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC 247
#define MAVLINK_MSG_ID_153_CRC 247



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_TRACKING_TARGET_RECT { \
    153, \
    "TRACKING_TARGET_RECT", \
    4, \
    {  { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_tracking_target_rect_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_tracking_target_rect_t, y) }, \
         { "width", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_tracking_target_rect_t, width) }, \
         { "height", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_tracking_target_rect_t, height) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_TRACKING_TARGET_RECT { \
    "TRACKING_TARGET_RECT", \
    4, \
    {  { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_tracking_target_rect_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_tracking_target_rect_t, y) }, \
         { "width", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_tracking_target_rect_t, width) }, \
         { "height", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_tracking_target_rect_t, height) }, \
         } \
}
#endif

/**
 * @brief Pack a tracking_target_rect message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param x  x
 * @param y  y
 * @param width  width
 * @param height  height
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_tracking_target_rect_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               float x, float y, float width, float height)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN];
    _mav_put_float(buf, 0, x);
    _mav_put_float(buf, 4, y);
    _mav_put_float(buf, 8, width);
    _mav_put_float(buf, 12, height);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN);
#else
    mavlink_tracking_target_rect_t packet;
    packet.x = x;
    packet.y = y;
    packet.width = width;
    packet.height = height;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TRACKING_TARGET_RECT;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
}

/**
 * @brief Pack a tracking_target_rect message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param x  x
 * @param y  y
 * @param width  width
 * @param height  height
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_tracking_target_rect_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   float x,float y,float width,float height)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN];
    _mav_put_float(buf, 0, x);
    _mav_put_float(buf, 4, y);
    _mav_put_float(buf, 8, width);
    _mav_put_float(buf, 12, height);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN);
#else
    mavlink_tracking_target_rect_t packet;
    packet.x = x;
    packet.y = y;
    packet.width = width;
    packet.height = height;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TRACKING_TARGET_RECT;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
}

/**
 * @brief Encode a tracking_target_rect struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param tracking_target_rect C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_tracking_target_rect_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_tracking_target_rect_t* tracking_target_rect)
{
    return mavlink_msg_tracking_target_rect_pack(system_id, component_id, msg, tracking_target_rect->x, tracking_target_rect->y, tracking_target_rect->width, tracking_target_rect->height);
}

/**
 * @brief Encode a tracking_target_rect struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param tracking_target_rect C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_tracking_target_rect_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_tracking_target_rect_t* tracking_target_rect)
{
    return mavlink_msg_tracking_target_rect_pack_chan(system_id, component_id, chan, msg, tracking_target_rect->x, tracking_target_rect->y, tracking_target_rect->width, tracking_target_rect->height);
}

/**
 * @brief Send a tracking_target_rect message
 * @param chan MAVLink channel to send the message
 *
 * @param x  x
 * @param y  y
 * @param width  width
 * @param height  height
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_tracking_target_rect_send(mavlink_channel_t chan, float x, float y, float width, float height)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN];
    _mav_put_float(buf, 0, x);
    _mav_put_float(buf, 4, y);
    _mav_put_float(buf, 8, width);
    _mav_put_float(buf, 12, height);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT, buf, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
#else
    mavlink_tracking_target_rect_t packet;
    packet.x = x;
    packet.y = y;
    packet.width = width;
    packet.height = height;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT, (const char *)&packet, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
#endif
}

/**
 * @brief Send a tracking_target_rect message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_tracking_target_rect_send_struct(mavlink_channel_t chan, const mavlink_tracking_target_rect_t* tracking_target_rect)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_tracking_target_rect_send(chan, tracking_target_rect->x, tracking_target_rect->y, tracking_target_rect->width, tracking_target_rect->height);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT, (const char *)tracking_target_rect, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
#endif
}

#if MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_tracking_target_rect_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  float x, float y, float width, float height)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_float(buf, 0, x);
    _mav_put_float(buf, 4, y);
    _mav_put_float(buf, 8, width);
    _mav_put_float(buf, 12, height);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT, buf, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
#else
    mavlink_tracking_target_rect_t *packet = (mavlink_tracking_target_rect_t *)msgbuf;
    packet->x = x;
    packet->y = y;
    packet->width = width;
    packet->height = height;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TRACKING_TARGET_RECT, (const char *)packet, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_MIN_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_CRC);
#endif
}
#endif

#endif

// MESSAGE TRACKING_TARGET_RECT UNPACKING


/**
 * @brief Get field x from tracking_target_rect message
 *
 * @return  x
 */
static inline float mavlink_msg_tracking_target_rect_get_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field y from tracking_target_rect message
 *
 * @return  y
 */
static inline float mavlink_msg_tracking_target_rect_get_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field width from tracking_target_rect message
 *
 * @return  width
 */
static inline float mavlink_msg_tracking_target_rect_get_width(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field height from tracking_target_rect message
 *
 * @return  height
 */
static inline float mavlink_msg_tracking_target_rect_get_height(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Decode a tracking_target_rect message into a struct
 *
 * @param msg The message to decode
 * @param tracking_target_rect C-struct to decode the message contents into
 */
static inline void mavlink_msg_tracking_target_rect_decode(const mavlink_message_t* msg, mavlink_tracking_target_rect_t* tracking_target_rect)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    tracking_target_rect->x = mavlink_msg_tracking_target_rect_get_x(msg);
    tracking_target_rect->y = mavlink_msg_tracking_target_rect_get_y(msg);
    tracking_target_rect->width = mavlink_msg_tracking_target_rect_get_width(msg);
    tracking_target_rect->height = mavlink_msg_tracking_target_rect_get_height(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN? msg->len : MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN;
        memset(tracking_target_rect, 0, MAVLINK_MSG_ID_TRACKING_TARGET_RECT_LEN);
    memcpy(tracking_target_rect, _MAV_PAYLOAD(msg), len);
#endif
}
