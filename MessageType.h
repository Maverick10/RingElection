#ifndef MESSAGETYPE_H_
#define MESSAGETYPE_H_

/*
 * Message Formats:
 *
 * ChangeNext <SENDER> <TIMESTAMP> <NEXT>
 * -----------------------
 * PingRequest <SENDER> <TIMESTAMP>
 * -----------------------
 * PingReply <SENDER> <TIMESTAMP>
 * -----------------------
 * Heartbeat <SENDER> <TIMESTAMP>
 * -----------------------
 * ProcessDeath <SENDER> <TIMESTAMP> <DEADPROCESS> <ORIGINALSENDER>
 * -----------------------
 * Election <SENDER> <TIMESTAMP> <INITIATOR> <CURWINNER>
 * -----------------------
 * Victory <SENDER> <TIMESTAMP> <ORIGINALSENDER> <COORDINATOR>
 * -----------------------
 * Count <SENDER> <TIMESTAMP> <COORDINATOR> <COUNT>
 */

#define MSGTYPE_CHANGENEXT 1
#define MSGTYPE_PINGREQUEST 2
#define MSGTYPE_PINGREPLY 3
#define MSGTYPE_HEARTBEAT 4
#define MSGTYPE_PROCESSDEATH 5
#define MSGTYPE_ELECTION 6
#define MSGTYPE_VICTORY 7
#define MSGTYPE_COUNT 8
#define MSGTYPE_DATA 9
#define MSGTYPE_RESULT 10

#endif /* MESSAGETYPE_H_ */
