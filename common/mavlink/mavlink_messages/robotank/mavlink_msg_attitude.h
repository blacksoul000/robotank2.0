#pragma once
// MESSAGE ATTITUDE PACKING

#define MAVLINK_MSG_ID_ATTITUDE 152

MAVPACKED(
typedef struct __mavlink_attitude_t {
 int16_t yaw; /*<  yaw*/
 int16_t pitch; /*<  pitch*/
 int16_t roll; /*<  roll*/
 int16_t gunH; /*<  gun horizontal position 0..360*/
 int16_t gunV; /*<  gun lift angle*/
}) mavlink_attitude_t;

#define MAVLINK_MSG_ID_ATTITUDE_LEN 10
#define MAVLINK_MSG_ID_ATTITUDE_MIN_LEN 10
#define MAVLINK_MSG_ID_152_LEN 10
#define MAVLINK_MSG_ID_152_MIN_LEN 10

#define MAVLINK_MSG_ID_ATTITUDE_CRC 111
#define MAVLINK_MSG_ID_152_CRC 111



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ATTITUDE { \
    152, \
    "ATTITUDE", \
    5, \
    {  { "yaw", NULL, MAVLINK_TYPE_INT16_T, 0, 0, offsetof(mavlink_attitude_t, yaw) }, \
         { "pitch", NULL, MAVLINK_TYPE_INT16_T, 0, 2, offsetof(mavlink_attitude_t, pitch) }, \
         { "roll", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_attitude_t, roll) }, \
         { "gunH", NULL, MAVLINK_TYPE_INT16_T, 0, 6, offsetof(mavlink_attitude_t, gunH) }, \
         { "gunV", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_attitude_t, gunV) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ATTITUDE { \
    "ATTITUDE", \
    5, \
    {  { "yaw", NULL, MAVLINK_TYPE_INT16_T, 0, 0, offsetof(mavlink_attitude_t, yaw) }, \
         { "pitch", NULL, MAVLINK_TYPE_INT16_T, 0, 2, offsetof(mavlink_attitude_t, pitch) }, \
         { "roll", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_attitude_t, roll) }, \
         { "gunH", NULL, MAVLINK_TYPE_INT16_T, 0, 6, offsetof(mavlink_attitude_t, gunH) }, \
         { "gunV", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_attitude_t, gunV) }, \
         } \
}
#endif

/**
 * @brief Pack a attitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param yaw  yaw
 * @param pitch  pitch
 * @param roll  roll
 * @param gunH  gun horizontal position 0..360
 * @param gunV  gun lift angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               int16_t yaw, int16_t pitch, int16_t roll, int16_t gunH, int16_t gunV)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ATTITUDE_LEN];
    _mav_put_int16_t(buf, 0, yaw);
    _mav_put_int16_t(buf, 2, pitch);
    _mav_put_int16_t(buf, 4, roll);
    _mav_put_int16_t(buf, 6, gunH);
    _mav_put_int16_t(buf, 8, gunV);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ATTITUDE_LEN);
#else
    mavlink_attitude_t packet;
    packet.yaw = yaw;
    packet.pitch = pitch;
    packet.roll = roll;
    packet.gunH = gunH;
    packet.gunV = gunV;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ATTITUDE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ATTITUDE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
}

/**
 * @brief Pack a attitude message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param yaw  yaw
 * @param pitch  pitch
 * @param roll  roll
 * @param gunH  gun horizontal position 0..360
 * @param gunV  gun lift angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   int16_t yaw,int16_t pitch,int16_t roll,int16_t gunH,int16_t gunV)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ATTITUDE_LEN];
    _mav_put_int16_t(buf, 0, yaw);
    _mav_put_int16_t(buf, 2, pitch);
    _mav_put_int16_t(buf, 4, roll);
    _mav_put_int16_t(buf, 6, gunH);
    _mav_put_int16_t(buf, 8, gunV);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ATTITUDE_LEN);
#else
    mavlink_attitude_t packet;
    packet.yaw = yaw;
    packet.pitch = pitch;
    packet.roll = roll;
    packet.gunH = gunH;
    packet.gunV = gunV;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ATTITUDE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ATTITUDE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
}

/**
 * @brief Encode a attitude struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param attitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_attitude_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_attitude_t* attitude)
{
    return mavlink_msg_attitude_pack(system_id, component_id, msg, attitude->yaw, attitude->pitch, attitude->roll, attitude->gunH, attitude->gunV);
}

/**
 * @brief Encode a attitude struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param attitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_attitude_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_attitude_t* attitude)
{
    return mavlink_msg_attitude_pack_chan(system_id, component_id, chan, msg, attitude->yaw, attitude->pitch, attitude->roll, attitude->gunH, attitude->gunV);
}

/**
 * @brief Send a attitude message
 * @param chan MAVLink channel to send the message
 *
 * @param yaw  yaw
 * @param pitch  pitch
 * @param roll  roll
 * @param gunH  gun horizontal position 0..360
 * @param gunV  gun lift angle
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_attitude_send(mavlink_channel_t chan, int16_t yaw, int16_t pitch, int16_t roll, int16_t gunH, int16_t gunV)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ATTITUDE_LEN];
    _mav_put_int16_t(buf, 0, yaw);
    _mav_put_int16_t(buf, 2, pitch);
    _mav_put_int16_t(buf, 4, roll);
    _mav_put_int16_t(buf, 6, gunH);
    _mav_put_int16_t(buf, 8, gunV);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ATTITUDE, buf, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
#else
    mavlink_attitude_t packet;
    packet.yaw = yaw;
    packet.pitch = pitch;
    packet.roll = roll;
    packet.gunH = gunH;
    packet.gunV = gunV;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ATTITUDE, (const char *)&packet, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
#endif
}

/**
 * @brief Send a attitude message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_attitude_send_struct(mavlink_channel_t chan, const mavlink_attitude_t* attitude)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_attitude_send(chan, attitude->yaw, attitude->pitch, attitude->roll, attitude->gunH, attitude->gunV);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ATTITUDE, (const char *)attitude, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
#endif
}

#if MAVLINK_MSG_ID_ATTITUDE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_attitude_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  int16_t yaw, int16_t pitch, int16_t roll, int16_t gunH, int16_t gunV)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_int16_t(buf, 0, yaw);
    _mav_put_int16_t(buf, 2, pitch);
    _mav_put_int16_t(buf, 4, roll);
    _mav_put_int16_t(buf, 6, gunH);
    _mav_put_int16_t(buf, 8, gunV);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ATTITUDE, buf, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
#else
    mavlink_attitude_t *packet = (mavlink_attitude_t *)msgbuf;
    packet->yaw = yaw;
    packet->pitch = pitch;
    packet->roll = roll;
    packet->gunH = gunH;
    packet->gunV = gunV;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ATTITUDE, (const char *)packet, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, MAVLINK_MSG_ID_ATTITUDE_CRC);
#endif
}
#endif

#endif

// MESSAGE ATTITUDE UNPACKING


/**
 * @brief Get field yaw from attitude message
 *
 * @return  yaw
 */
static inline int16_t mavlink_msg_attitude_get_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  0);
}

/**
 * @brief Get field pitch from attitude message
 *
 * @return  pitch
 */
static inline int16_t mavlink_msg_attitude_get_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  2);
}

/**
 * @brief Get field roll from attitude message
 *
 * @return  roll
 */
static inline int16_t mavlink_msg_attitude_get_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  4);
}

/**
 * @brief Get field gunH from attitude message
 *
 * @return  gun horizontal position 0..360
 */
static inline int16_t mavlink_msg_attitude_get_gunH(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  6);
}

/**
 * @brief Get field gunV from attitude message
 *
 * @return  gun lift angle
 */
static inline int16_t mavlink_msg_attitude_get_gunV(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  8);
}

/**
 * @brief Decode a attitude message into a struct
 *
 * @param msg The message to decode
 * @param attitude C-struct to decode the message contents into
 */
static inline void mavlink_msg_attitude_decode(const mavlink_message_t* msg, mavlink_attitude_t* attitude)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    attitude->yaw = mavlink_msg_attitude_get_yaw(msg);
    attitude->pitch = mavlink_msg_attitude_get_pitch(msg);
    attitude->roll = mavlink_msg_attitude_get_roll(msg);
    attitude->gunH = mavlink_msg_attitude_get_gunH(msg);
    attitude->gunV = mavlink_msg_attitude_get_gunV(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ATTITUDE_LEN? msg->len : MAVLINK_MSG_ID_ATTITUDE_LEN;
        memset(attitude, 0, MAVLINK_MSG_ID_ATTITUDE_LEN);
    memcpy(attitude, _MAV_PAYLOAD(msg), len);
#endif
}
