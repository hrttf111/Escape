/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "common.h"

#define MSG_KEYBOARD_READ			0

#define MSG_VIDEO_SET				0
#define MSG_VIDEO_SETSCREEN			1
#define MSG_VIDEO_SETCURSOR			2

#define MSG_SPEAKER_BEEP			0

#define MSG_ATA_READ_REQ			0
#define MSG_ATA_WRITE_REQ			1
#define MSG_ATA_READ_RESP			2

/* the header for all default-messages */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgDefHeader;

/* a message that will be send from the keyboard-service */
typedef struct {
	/* the keycode (see keycodes.h) */
	u8 keycode;
	/* wether the key was released */
	u8 isBreak;
} sMsgKbResponse;

/* the message-data for the speaker-beep-message */
typedef struct {
	u16 frequency;
	u16 duration;	/* in ms */
} sMsgDataSpeakerBeep;

/* the message-data for the video-set-message */
typedef struct {
	u8 col;
	u8 row;
	u8 color;
	s8 character;
} sMsgDataVidSet;

/* the message-data for the video-setcursor-message */
typedef struct {
	u8 col;
	u8 row;
} sMsgDataVidSetCursor;

/* the message-data for a ATA-read- and write-request.
 * A write-request transports the data directly behind this struct */
typedef struct {
	u8 drive;
	u8 partition;
	u64 lba;
	u16 secCount;
} sMsgDataATAReq;

/**
 * Creates a default-message with the given data
 *
 * @param id the message-id
 * @param length the length of the data to send
 * @param buf the data to send
 * @return the message or NULL if failed
 */
sMsgDefHeader *createDefMsg(u8 id,u32 length,void *buf);

/**
 * Frees the messages that has been build with createDefMsg().
 *
 * @param msg the message
 */
void freeDefMsg(sMsgDefHeader *msg);

#endif /* MESSAGES_H_ */
