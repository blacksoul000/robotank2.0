#pragma once
// MESSAGE SYS_STATUS PACKING

#define MAVLINK_MSG_ID_SYS_STATUS 151

MAVPACKED(
typedef struct __mavlink_sys_status_t {
 uint16_t voltage; /*<  chassis voltage*/
 uint16_t gamepad_buttons; /*<  gamepad buttons state*/
 uint8_t system_state; /*<  system state*/
 uint8_t gamepad_capacity; /*<  gamepad capacity*/
 uint8_t bluetooth_devices_count; /*<  bluetooth devices count*/
}) mavlink_sys_status_t;

#define MAVLINK_MSG_ID_SYS_STATUS_LEN 7
#define MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN 7
#define MAVLINK_MSG_ID_151_LEN 7
#define MAVLINK_MSG_ID_151_MIN_LEN 7

#define MAVLINK_MSG_ID_SYS_STATUS_CRC 233
#define MAVLINK_MSG_ID_151_CRC 233



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_SYS_STATUS { \
    151, \
    "SYS_STATUS", \
    5, \
    {  { "system_state", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_sys_status_t, system_state) }, \
         { "voltage", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_sys_status_t, voltage) }, \
         { "gamepad_capacity", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_sys_status_t, gamepad_capacity) }, \
         { "gamepad_buttons", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_sys_status_t, gamepad_buttons) }, \
         { "bluetooth_devices_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_sys_status_t, bluetooth_devices_count) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_SYS_STATUS { \
    "SYS_STATUS", \
    5, \
    {  { "system_state", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_sys_status_t, system_state) }, \
         { "voltage", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_sys_status_t, voltage) }, \
         { "gamepad_capacity", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_sys_status_t, gamepad_capacity) }, \
         { "gamepad_buttons", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_sys_status_t, gamepad_buttons) }, \
         { "bluetooth_devices_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_sys_status_t, bluetooth_devices_count) }, \
         } \
}
#endif

/**
 * @brief Pack a sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param system_state  system state
 * @param voltage  chassis voltage
 * @param gamepad_capacity  gamepad capacity
 * @param gamepad_buttons  gamepad buttons state
 * @param bluetooth_devices_count  bluetooth devices count
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t system_state, uint16_t voltage, uint8_t gamepad_capacity, uint16_t gamepad_buttons, uint8_t bluetooth_devices_count)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SYS_STATUS_LEN];
    _mav_put_uint16_t(buf, 0, voltage);
    _mav_put_uint16_t(buf, 2, gamepad_buttons);
    _mav_put_uint8_t(buf, 4, system_state);
    _mav_put_uint8_t(buf, 5, gamepad_capacity);
    _mav_put_uint8_t(buf, 6, bluetooth_devices_count);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SYS_STATUS_LEN);
#else
    mavlink_sys_status_t packet;
    packet.voltage = voltage;
    packet.gamepad_buttons = gamepad_buttons;
    packet.system_state = system_state;
    packet.gamepad_capacity = gamepad_capacity;
    packet.bluetooth_devices_count = bluetooth_devices_count;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SYS_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
}

/**
 * @brief Pack a sys_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param system_state  system state
 * @param voltage  chassis voltage
 * @param gamepad_capacity  gamepad capacity
 * @param gamepad_buttons  gamepad buttons state
 * @param bluetooth_devices_count  bluetooth devices count
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t system_state,uint16_t voltage,uint8_t gamepad_capacity,uint16_t gamepad_buttons,uint8_t bluetooth_devices_count)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SYS_STATUS_LEN];
    _mav_put_uint16_t(buf, 0, voltage);
    _mav_put_uint16_t(buf, 2, gamepad_buttons);
    _mav_put_uint8_t(buf, 4, system_state);
    _mav_put_uint8_t(buf, 5, gamepad_capacity);
    _mav_put_uint8_t(buf, 6, bluetooth_devices_count);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SYS_STATUS_LEN);
#else
    mavlink_sys_status_t packet;
    packet.voltage = voltage;
    packet.gamepad_buttons = gamepad_buttons;
    packet.system_state = system_state;
    packet.gamepad_capacity = gamepad_capacity;
    packet.bluetooth_devices_count = bluetooth_devices_count;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SYS_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
}

/**
 * @brief Encode a sys_status struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sys_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sys_status_t* sys_status)
{
    return mavlink_msg_sys_status_pack(system_id, component_id, msg, sys_status->system_state, sys_status->voltage, sys_status->gamepad_capacity, sys_status->gamepad_buttons, sys_status->bluetooth_devices_count);
}

/**
 * @brief Encode a sys_status struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param sys_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_status_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_sys_status_t* sys_status)
{
    return mavlink_msg_sys_status_pack_chan(system_id, component_id, chan, msg, sys_status->system_state, sys_status->voltage, sys_status->gamepad_capacity, sys_status->gamepad_buttons, sys_status->bluetooth_devices_count);
}

/**
 * @brief Send a sys_status message
 * @param chan MAVLink channel to send the message
 *
 * @param system_state  system state
 * @param voltage  chassis voltage
 * @param gamepad_capacity  gamepad capacity
 * @param gamepad_buttons  gamepad buttons state
 * @param bluetooth_devices_count  bluetooth devices count
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sys_status_send(mavlink_channel_t chan, uint8_t system_state, uint16_t voltage, uint8_t gamepad_capacity, uint16_t gamepad_buttons, uint8_t bluetooth_devices_count)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SYS_STATUS_LEN];
    _mav_put_uint16_t(buf, 0, voltage);
    _mav_put_uint16_t(buf, 2, gamepad_buttons);
    _mav_put_uint8_t(buf, 4, system_state);
    _mav_put_uint8_t(buf, 5, gamepad_capacity);
    _mav_put_uint8_t(buf, 6, bluetooth_devices_count);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_STATUS, buf, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
#else
    mavlink_sys_status_t packet;
    packet.voltage = voltage;
    packet.gamepad_buttons = gamepad_buttons;
    packet.system_state = system_state;
    packet.gamepad_capacity = gamepad_capacity;
    packet.bluetooth_devices_count = bluetooth_devices_count;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_STATUS, (const char *)&packet, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
#endif
}

/**
 * @brief Send a sys_status message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_sys_status_send_struct(mavlink_channel_t chan, const mavlink_sys_status_t* sys_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_sys_status_send(chan, sys_status->system_state, sys_status->voltage, sys_status->gamepad_capacity, sys_status->gamepad_buttons, sys_status->bluetooth_devices_count);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_STATUS, (const char *)sys_status, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
#endif
}

#if MAVLINK_MSG_ID_SYS_STATUS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_sys_status_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t system_state, uint16_t voltage, uint8_t gamepad_capacity, uint16_t gamepad_buttons, uint8_t bluetooth_devices_count)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, voltage);
    _mav_put_uint16_t(buf, 2, gamepad_buttons);
    _mav_put_uint8_t(buf, 4, system_state);
    _mav_put_uint8_t(buf, 5, gamepad_capacity);
    _mav_put_uint8_t(buf, 6, bluetooth_devices_count);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_STATUS, buf, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
#else
    mavlink_sys_status_t *packet = (mavlink_sys_status_t *)msgbuf;
    packet->voltage = voltage;
    packet->gamepad_buttons = gamepad_buttons;
    packet->system_state = system_state;
    packet->gamepad_capacity = gamepad_capacity;
    packet->bluetooth_devices_count = bluetooth_devices_count;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SYS_STATUS, (const char *)packet, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, MAVLINK_MSG_ID_SYS_STATUS_CRC);
#endif
}
#endif

#endif

// MESSAGE SYS_STATUS UNPACKING


/**
 * @brief Get field system_state from sys_status message
 *
 * @return  system state
 */
static inline uint8_t mavlink_msg_sys_status_get_system_state(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field voltage from sys_status message
 *
 * @return  chassis voltage
 */
static inline uint16_t mavlink_msg_sys_status_get_voltage(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field gamepad_capacity from sys_status message
 *
 * @return  gamepad capacity
 */
static inline uint8_t mavlink_msg_sys_status_get_gamepad_capacity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field gamepad_buttons from sys_status message
 *
 * @return  gamepad buttons state
 */
static inline uint16_t mavlink_msg_sys_status_get_gamepad_buttons(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field bluetooth_devices_count from sys_status message
 *
 * @return  bluetooth devices count
 */
static inline uint8_t mavlink_msg_sys_status_get_bluetooth_devices_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Decode a sys_status message into a struct
 *
 * @param msg The message to decode
 * @param sys_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_status_decode(const mavlink_message_t* msg, mavlink_sys_status_t* sys_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    sys_status->voltage = mavlink_msg_sys_status_get_voltage(msg);
    sys_status->gamepad_buttons = mavlink_msg_sys_status_get_gamepad_buttons(msg);
    sys_status->system_state = mavlink_msg_sys_status_get_system_state(msg);
    sys_status->gamepad_capacity = mavlink_msg_sys_status_get_gamepad_capacity(msg);
    sys_status->bluetooth_devices_count = mavlink_msg_sys_status_get_bluetooth_devices_count(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_SYS_STATUS_LEN? msg->len : MAVLINK_MSG_ID_SYS_STATUS_LEN;
        memset(sys_status, 0, MAVLINK_MSG_ID_SYS_STATUS_LEN);
    memcpy(sys_status, _MAV_PAYLOAD(msg), len);
#endif
}
