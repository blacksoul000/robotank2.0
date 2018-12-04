#pragma once
// MESSAGE BLUETOOTH_DEVICES PACKING

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES 154

MAVPACKED(
typedef struct __mavlink_bluetooth_devices_t {
 int8_t total_count; /*<  total device count*/
 int8_t first_index; /*<  index of first element*/
 int8_t device_list[253]; /*<  serialized DeviceInfo list*/
}) mavlink_bluetooth_devices_t;

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN 255
#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN 255
#define MAVLINK_MSG_ID_154_LEN 255
#define MAVLINK_MSG_ID_154_MIN_LEN 255

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC 47
#define MAVLINK_MSG_ID_154_CRC 47

#define MAVLINK_MSG_BLUETOOTH_DEVICES_FIELD_DEVICE_LIST_LEN 253

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_BLUETOOTH_DEVICES { \
    154, \
    "BLUETOOTH_DEVICES", \
    3, \
    {  { "total_count", NULL, MAVLINK_TYPE_INT8_T, 0, 0, offsetof(mavlink_bluetooth_devices_t, total_count) }, \
         { "first_index", NULL, MAVLINK_TYPE_INT8_T, 0, 1, offsetof(mavlink_bluetooth_devices_t, first_index) }, \
         { "device_list", NULL, MAVLINK_TYPE_INT8_T, 253, 2, offsetof(mavlink_bluetooth_devices_t, device_list) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_BLUETOOTH_DEVICES { \
    "BLUETOOTH_DEVICES", \
    3, \
    {  { "total_count", NULL, MAVLINK_TYPE_INT8_T, 0, 0, offsetof(mavlink_bluetooth_devices_t, total_count) }, \
         { "first_index", NULL, MAVLINK_TYPE_INT8_T, 0, 1, offsetof(mavlink_bluetooth_devices_t, first_index) }, \
         { "device_list", NULL, MAVLINK_TYPE_INT8_T, 253, 2, offsetof(mavlink_bluetooth_devices_t, device_list) }, \
         } \
}
#endif

/**
 * @brief Pack a bluetooth_devices message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param total_count  total device count
 * @param first_index  index of first element
 * @param device_list  serialized DeviceInfo list
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bluetooth_devices_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               int8_t total_count, int8_t first_index, const int8_t *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_int8_t(buf, 0, total_count);
    _mav_put_int8_t(buf, 1, first_index);
    _mav_put_int8_t_array(buf, 2, device_list, 253);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    mav_array_memcpy(packet.device_list, device_list, sizeof(int8_t)*253);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BLUETOOTH_DEVICES;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
}

/**
 * @brief Pack a bluetooth_devices message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param total_count  total device count
 * @param first_index  index of first element
 * @param device_list  serialized DeviceInfo list
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bluetooth_devices_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   int8_t total_count,int8_t first_index,const int8_t *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_int8_t(buf, 0, total_count);
    _mav_put_int8_t(buf, 1, first_index);
    _mav_put_int8_t_array(buf, 2, device_list, 253);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    mav_array_memcpy(packet.device_list, device_list, sizeof(int8_t)*253);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BLUETOOTH_DEVICES;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
}

/**
 * @brief Encode a bluetooth_devices struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param bluetooth_devices C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_bluetooth_devices_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_bluetooth_devices_t* bluetooth_devices)
{
    return mavlink_msg_bluetooth_devices_pack(system_id, component_id, msg, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->device_list);
}

/**
 * @brief Encode a bluetooth_devices struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param bluetooth_devices C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_bluetooth_devices_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_bluetooth_devices_t* bluetooth_devices)
{
    return mavlink_msg_bluetooth_devices_pack_chan(system_id, component_id, chan, msg, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->device_list);
}

/**
 * @brief Send a bluetooth_devices message
 * @param chan MAVLink channel to send the message
 *
 * @param total_count  total device count
 * @param first_index  index of first element
 * @param device_list  serialized DeviceInfo list
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_bluetooth_devices_send(mavlink_channel_t chan, int8_t total_count, int8_t first_index, const int8_t *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_int8_t(buf, 0, total_count);
    _mav_put_int8_t(buf, 1, first_index);
    _mav_put_int8_t_array(buf, 2, device_list, 253);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    mav_array_memcpy(packet.device_list, device_list, sizeof(int8_t)*253);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, (const char *)&packet, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#endif
}

/**
 * @brief Send a bluetooth_devices message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_bluetooth_devices_send_struct(mavlink_channel_t chan, const mavlink_bluetooth_devices_t* bluetooth_devices)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_bluetooth_devices_send(chan, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->device_list);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, (const char *)bluetooth_devices, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#endif
}

#if MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_bluetooth_devices_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  int8_t total_count, int8_t first_index, const int8_t *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_int8_t(buf, 0, total_count);
    _mav_put_int8_t(buf, 1, first_index);
    _mav_put_int8_t_array(buf, 2, device_list, 253);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#else
    mavlink_bluetooth_devices_t *packet = (mavlink_bluetooth_devices_t *)msgbuf;
    packet->total_count = total_count;
    packet->first_index = first_index;
    mav_array_memcpy(packet->device_list, device_list, sizeof(int8_t)*253);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, (const char *)packet, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#endif
}
#endif

#endif

// MESSAGE BLUETOOTH_DEVICES UNPACKING


/**
 * @brief Get field total_count from bluetooth_devices message
 *
 * @return  total device count
 */
static inline int8_t mavlink_msg_bluetooth_devices_get_total_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  0);
}

/**
 * @brief Get field first_index from bluetooth_devices message
 *
 * @return  index of first element
 */
static inline int8_t mavlink_msg_bluetooth_devices_get_first_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  1);
}

/**
 * @brief Get field device_list from bluetooth_devices message
 *
 * @return  serialized DeviceInfo list
 */
static inline uint16_t mavlink_msg_bluetooth_devices_get_device_list(const mavlink_message_t* msg, int8_t *device_list)
{
    return _MAV_RETURN_int8_t_array(msg, device_list, 253,  2);
}

/**
 * @brief Decode a bluetooth_devices message into a struct
 *
 * @param msg The message to decode
 * @param bluetooth_devices C-struct to decode the message contents into
 */
static inline void mavlink_msg_bluetooth_devices_decode(const mavlink_message_t* msg, mavlink_bluetooth_devices_t* bluetooth_devices)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    bluetooth_devices->total_count = mavlink_msg_bluetooth_devices_get_total_count(msg);
    bluetooth_devices->first_index = mavlink_msg_bluetooth_devices_get_first_index(msg);
    mavlink_msg_bluetooth_devices_get_device_list(msg, bluetooth_devices->device_list);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN? msg->len : MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN;
        memset(bluetooth_devices, 0, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
    memcpy(bluetooth_devices, _MAV_PAYLOAD(msg), len);
#endif
}
