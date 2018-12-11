#pragma once
// MESSAGE COMMAND_BLUETOOTH_PAIR PACKING

#define MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR 76

MAVPACKED(
typedef struct __mavlink_command_bluetooth_pair_t {
 uint64_t address; /*<  Device address.*/
 uint16_t command; /*<  Command ID (of command to send).*/
 uint8_t target_system; /*<  System which should execute the command*/
 uint8_t target_component; /*<  Component which should execute the command, 0 for all components*/
 uint8_t confirmation; /*<  0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)*/
 uint8_t pair; /*<  1 - pair, 0 - unpair.*/
}) mavlink_command_bluetooth_pair_t;

#define MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN 14
#define MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN 14
#define MAVLINK_MSG_ID_76_LEN 14
#define MAVLINK_MSG_ID_76_MIN_LEN 14

#define MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC 81
#define MAVLINK_MSG_ID_76_CRC 81



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_COMMAND_BLUETOOTH_PAIR { \
    76, \
    "COMMAND_BLUETOOTH_PAIR", \
    6, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_command_bluetooth_pair_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_command_bluetooth_pair_t, target_component) }, \
         { "command", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_command_bluetooth_pair_t, command) }, \
         { "confirmation", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_command_bluetooth_pair_t, confirmation) }, \
         { "address", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_command_bluetooth_pair_t, address) }, \
         { "pair", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_command_bluetooth_pair_t, pair) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_COMMAND_BLUETOOTH_PAIR { \
    "COMMAND_BLUETOOTH_PAIR", \
    6, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_command_bluetooth_pair_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_command_bluetooth_pair_t, target_component) }, \
         { "command", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_command_bluetooth_pair_t, command) }, \
         { "confirmation", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_command_bluetooth_pair_t, confirmation) }, \
         { "address", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_command_bluetooth_pair_t, address) }, \
         { "pair", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_command_bluetooth_pair_t, pair) }, \
         } \
}
#endif

/**
 * @brief Pack a command_bluetooth_pair message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System which should execute the command
 * @param target_component  Component which should execute the command, 0 for all components
 * @param command  Command ID (of command to send).
 * @param confirmation  0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param address  Device address.
 * @param pair  1 - pair, 0 - unpair.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_bluetooth_pair_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint16_t command, uint8_t confirmation, uint64_t address, uint8_t pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN];
    _mav_put_uint64_t(buf, 0, address);
    _mav_put_uint16_t(buf, 8, command);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, confirmation);
    _mav_put_uint8_t(buf, 13, pair);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN);
#else
    mavlink_command_bluetooth_pair_t packet;
    packet.address = address;
    packet.command = command;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.confirmation = confirmation;
    packet.pair = pair;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
}

/**
 * @brief Pack a command_bluetooth_pair message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System which should execute the command
 * @param target_component  Component which should execute the command, 0 for all components
 * @param command  Command ID (of command to send).
 * @param confirmation  0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param address  Device address.
 * @param pair  1 - pair, 0 - unpair.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_bluetooth_pair_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint16_t command,uint8_t confirmation,uint64_t address,uint8_t pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN];
    _mav_put_uint64_t(buf, 0, address);
    _mav_put_uint16_t(buf, 8, command);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, confirmation);
    _mav_put_uint8_t(buf, 13, pair);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN);
#else
    mavlink_command_bluetooth_pair_t packet;
    packet.address = address;
    packet.command = command;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.confirmation = confirmation;
    packet.pair = pair;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
}

/**
 * @brief Encode a command_bluetooth_pair struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param command_bluetooth_pair C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_command_bluetooth_pair_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_command_bluetooth_pair_t* command_bluetooth_pair)
{
    return mavlink_msg_command_bluetooth_pair_pack(system_id, component_id, msg, command_bluetooth_pair->target_system, command_bluetooth_pair->target_component, command_bluetooth_pair->command, command_bluetooth_pair->confirmation, command_bluetooth_pair->address, command_bluetooth_pair->pair);
}

/**
 * @brief Encode a command_bluetooth_pair struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param command_bluetooth_pair C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_command_bluetooth_pair_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_command_bluetooth_pair_t* command_bluetooth_pair)
{
    return mavlink_msg_command_bluetooth_pair_pack_chan(system_id, component_id, chan, msg, command_bluetooth_pair->target_system, command_bluetooth_pair->target_component, command_bluetooth_pair->command, command_bluetooth_pair->confirmation, command_bluetooth_pair->address, command_bluetooth_pair->pair);
}

/**
 * @brief Send a command_bluetooth_pair message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System which should execute the command
 * @param target_component  Component which should execute the command, 0 for all components
 * @param command  Command ID (of command to send).
 * @param confirmation  0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param address  Device address.
 * @param pair  1 - pair, 0 - unpair.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_command_bluetooth_pair_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t command, uint8_t confirmation, uint64_t address, uint8_t pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN];
    _mav_put_uint64_t(buf, 0, address);
    _mav_put_uint16_t(buf, 8, command);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, confirmation);
    _mav_put_uint8_t(buf, 13, pair);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR, buf, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
#else
    mavlink_command_bluetooth_pair_t packet;
    packet.address = address;
    packet.command = command;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.confirmation = confirmation;
    packet.pair = pair;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR, (const char *)&packet, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
#endif
}

/**
 * @brief Send a command_bluetooth_pair message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_command_bluetooth_pair_send_struct(mavlink_channel_t chan, const mavlink_command_bluetooth_pair_t* command_bluetooth_pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_command_bluetooth_pair_send(chan, command_bluetooth_pair->target_system, command_bluetooth_pair->target_component, command_bluetooth_pair->command, command_bluetooth_pair->confirmation, command_bluetooth_pair->address, command_bluetooth_pair->pair);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR, (const char *)command_bluetooth_pair, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
#endif
}

#if MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_command_bluetooth_pair_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint16_t command, uint8_t confirmation, uint64_t address, uint8_t pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, address);
    _mav_put_uint16_t(buf, 8, command);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, confirmation);
    _mav_put_uint8_t(buf, 13, pair);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR, buf, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
#else
    mavlink_command_bluetooth_pair_t *packet = (mavlink_command_bluetooth_pair_t *)msgbuf;
    packet->address = address;
    packet->command = command;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->confirmation = confirmation;
    packet->pair = pair;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR, (const char *)packet, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_MIN_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_CRC);
#endif
}
#endif

#endif

// MESSAGE COMMAND_BLUETOOTH_PAIR UNPACKING


/**
 * @brief Get field target_system from command_bluetooth_pair message
 *
 * @return  System which should execute the command
 */
static inline uint8_t mavlink_msg_command_bluetooth_pair_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Get field target_component from command_bluetooth_pair message
 *
 * @return  Component which should execute the command, 0 for all components
 */
static inline uint8_t mavlink_msg_command_bluetooth_pair_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field command from command_bluetooth_pair message
 *
 * @return  Command ID (of command to send).
 */
static inline uint16_t mavlink_msg_command_bluetooth_pair_get_command(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field confirmation from command_bluetooth_pair message
 *
 * @return  0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 */
static inline uint8_t mavlink_msg_command_bluetooth_pair_get_confirmation(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field address from command_bluetooth_pair message
 *
 * @return  Device address.
 */
static inline uint64_t mavlink_msg_command_bluetooth_pair_get_address(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field pair from command_bluetooth_pair message
 *
 * @return  1 - pair, 0 - unpair.
 */
static inline uint8_t mavlink_msg_command_bluetooth_pair_get_pair(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Decode a command_bluetooth_pair message into a struct
 *
 * @param msg The message to decode
 * @param command_bluetooth_pair C-struct to decode the message contents into
 */
static inline void mavlink_msg_command_bluetooth_pair_decode(const mavlink_message_t* msg, mavlink_command_bluetooth_pair_t* command_bluetooth_pair)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    command_bluetooth_pair->address = mavlink_msg_command_bluetooth_pair_get_address(msg);
    command_bluetooth_pair->command = mavlink_msg_command_bluetooth_pair_get_command(msg);
    command_bluetooth_pair->target_system = mavlink_msg_command_bluetooth_pair_get_target_system(msg);
    command_bluetooth_pair->target_component = mavlink_msg_command_bluetooth_pair_get_target_component(msg);
    command_bluetooth_pair->confirmation = mavlink_msg_command_bluetooth_pair_get_confirmation(msg);
    command_bluetooth_pair->pair = mavlink_msg_command_bluetooth_pair_get_pair(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN? msg->len : MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN;
        memset(command_bluetooth_pair, 0, MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR_LEN);
    memcpy(command_bluetooth_pair, _MAV_PAYLOAD(msg), len);
#endif
}
