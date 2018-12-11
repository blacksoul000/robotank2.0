#pragma once
// MESSAGE BLUETOOTH_DEVICES PACKING

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES 154

MAVPACKED(
typedef struct __mavlink_bluetooth_devices_t {
 uint8_t total_count; /*<  total device count*/
 uint8_t first_index; /*<  index of first element*/
 uint8_t count; /*<  index of first element*/
 uint8_t data_length; /*<  size of device_list field*/
 char device_list[251]; /*<  serialized DeviceInfo list*/
}) mavlink_bluetooth_devices_t;

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN 255
#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN 255
#define MAVLINK_MSG_ID_154_LEN 255
#define MAVLINK_MSG_ID_154_MIN_LEN 255

#define MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC 84
#define MAVLINK_MSG_ID_154_CRC 84

#define MAVLINK_MSG_BLUETOOTH_DEVICES_FIELD_DEVICE_LIST_LEN 251

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_BLUETOOTH_DEVICES { \
    154, \
    "BLUETOOTH_DEVICES", \
    5, \
    {  { "total_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_bluetooth_devices_t, total_count) }, \
         { "first_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_bluetooth_devices_t, first_index) }, \
         { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_bluetooth_devices_t, count) }, \
         { "data_length", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_bluetooth_devices_t, data_length) }, \
         { "device_list", NULL, MAVLINK_TYPE_CHAR, 251, 4, offsetof(mavlink_bluetooth_devices_t, device_list) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_BLUETOOTH_DEVICES { \
    "BLUETOOTH_DEVICES", \
    5, \
    {  { "total_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_bluetooth_devices_t, total_count) }, \
         { "first_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_bluetooth_devices_t, first_index) }, \
         { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_bluetooth_devices_t, count) }, \
         { "data_length", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_bluetooth_devices_t, data_length) }, \
         { "device_list", NULL, MAVLINK_TYPE_CHAR, 251, 4, offsetof(mavlink_bluetooth_devices_t, device_list) }, \
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
 * @param count  index of first element
 * @param data_length  size of device_list field
 * @param device_list  serialized DeviceInfo list
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bluetooth_devices_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t total_count, uint8_t first_index, uint8_t count, uint8_t data_length, const char *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_uint8_t(buf, 0, total_count);
    _mav_put_uint8_t(buf, 1, first_index);
    _mav_put_uint8_t(buf, 2, count);
    _mav_put_uint8_t(buf, 3, data_length);
    _mav_put_char_array(buf, 4, device_list, 251);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    packet.count = count;
    packet.data_length = data_length;
    mav_array_memcpy(packet.device_list, device_list, sizeof(char)*251);
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
 * @param count  index of first element
 * @param data_length  size of device_list field
 * @param device_list  serialized DeviceInfo list
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bluetooth_devices_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t total_count,uint8_t first_index,uint8_t count,uint8_t data_length,const char *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_uint8_t(buf, 0, total_count);
    _mav_put_uint8_t(buf, 1, first_index);
    _mav_put_uint8_t(buf, 2, count);
    _mav_put_uint8_t(buf, 3, data_length);
    _mav_put_char_array(buf, 4, device_list, 251);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    packet.count = count;
    packet.data_length = data_length;
    mav_array_memcpy(packet.device_list, device_list, sizeof(char)*251);
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
    return mavlink_msg_bluetooth_devices_pack(system_id, component_id, msg, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->count, bluetooth_devices->data_length, bluetooth_devices->device_list);
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
    return mavlink_msg_bluetooth_devices_pack_chan(system_id, component_id, chan, msg, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->count, bluetooth_devices->data_length, bluetooth_devices->device_list);
}

/**
 * @brief Send a bluetooth_devices message
 * @param chan MAVLink channel to send the message
 *
 * @param total_count  total device count
 * @param first_index  index of first element
 * @param count  index of first element
 * @param data_length  size of device_list field
 * @param device_list  serialized DeviceInfo list
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_bluetooth_devices_send(mavlink_channel_t chan, uint8_t total_count, uint8_t first_index, uint8_t count, uint8_t data_length, const char *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN];
    _mav_put_uint8_t(buf, 0, total_count);
    _mav_put_uint8_t(buf, 1, first_index);
    _mav_put_uint8_t(buf, 2, count);
    _mav_put_uint8_t(buf, 3, data_length);
    _mav_put_char_array(buf, 4, device_list, 251);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#else
    mavlink_bluetooth_devices_t packet;
    packet.total_count = total_count;
    packet.first_index = first_index;
    packet.count = count;
    packet.data_length = data_length;
    mav_array_memcpy(packet.device_list, device_list, sizeof(char)*251);
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
    mavlink_msg_bluetooth_devices_send(chan, bluetooth_devices->total_count, bluetooth_devices->first_index, bluetooth_devices->count, bluetooth_devices->data_length, bluetooth_devices->device_list);
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
static inline void mavlink_msg_bluetooth_devices_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t total_count, uint8_t first_index, uint8_t count, uint8_t data_length, const char *device_list)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, total_count);
    _mav_put_uint8_t(buf, 1, first_index);
    _mav_put_uint8_t(buf, 2, count);
    _mav_put_uint8_t(buf, 3, data_length);
    _mav_put_char_array(buf, 4, device_list, 251);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BLUETOOTH_DEVICES, buf, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_MIN_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_CRC);
#else
    mavlink_bluetooth_devices_t *packet = (mavlink_bluetooth_devices_t *)msgbuf;
    packet->total_count = total_count;
    packet->first_index = first_index;
    packet->count = count;
    packet->data_length = data_length;
    mav_array_memcpy(packet->device_list, device_list, sizeof(char)*251);
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
static inline uint8_t mavlink_msg_bluetooth_devices_get_total_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field first_index from bluetooth_devices message
 *
 * @return  index of first element
 */
static inline uint8_t mavlink_msg_bluetooth_devices_get_first_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field count from bluetooth_devices message
 *
 * @return  index of first element
 */
static inline uint8_t mavlink_msg_bluetooth_devices_get_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field data_length from bluetooth_devices message
 *
 * @return  size of device_list field
 */
static inline uint8_t mavlink_msg_bluetooth_devices_get_data_length(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field device_list from bluetooth_devices message
 *
 * @return  serialized DeviceInfo list
 */
static inline uint16_t mavlink_msg_bluetooth_devices_get_device_list(const mavlink_message_t* msg, char *device_list)
{
    return _MAV_RETURN_char_array(msg, device_list, 251,  4);
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
    bluetooth_devices->count = mavlink_msg_bluetooth_devices_get_count(msg);
    bluetooth_devices->data_length = mavlink_msg_bluetooth_devices_get_data_length(msg);
    mavlink_msg_bluetooth_devices_get_device_list(msg, bluetooth_devices->device_list);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN? msg->len : MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN;
        memset(bluetooth_devices, 0, MAVLINK_MSG_ID_BLUETOOTH_DEVICES_LEN);
    memcpy(bluetooth_devices, _MAV_PAYLOAD(msg), len);
#endif
}
