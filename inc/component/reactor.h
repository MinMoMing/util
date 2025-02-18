//
// Created by hujianzhe on 2019-07-14.
//

#ifndef	UTIL_C_COMPONENT_REACTOR_H
#define	UTIL_C_COMPONENT_REACTOR_H

#include "../sysapi/io.h"
#include "../sysapi/ipc.h"
#include "../sysapi/process.h"
#include "../sysapi/socket.h"
#include "../datastruct/list.h"
#include "../datastruct/hashtable.h"
#include "../datastruct/transport_ctx.h"

enum {
	REACTOR_OBJECT_REG_CMD = 1,
	REACTOR_OBJECT_FREE_CMD,
	REACTOR_CHANNEL_FREE_CMD,

	REACTOR_STREAM_SENDFIN_CMD,

	REACTOR_SEND_PACKET_CMD,

	REACTOR_USER_CMD
};
enum {
	REACTOR_REG_ERR = 1,
	REACTOR_IO_ERR,
	REACTOR_CONNECT_ERR,
	REACTOR_ZOMBIE_ERR,
	REACTOR_ONREAD_ERR
};
enum {
	CHANNEL_FLAG_CLIENT = 1 << 0,
	CHANNEL_FLAG_SERVER = 1 << 1,
	CHANNEL_FLAG_LISTEN = 1 << 2,
	CHANNEL_FLAG_STREAM = 1 << 3,
	CHANNEL_FLAG_DGRAM	= 1 << 4
};

typedef struct Reactor_t {
	/* private */
	unsigned char m_runthreadhasbind;
	long long m_event_msec;
	Thread_t m_runthread;
	Nio_t m_nio;
	CriticalSection_t m_cmdlistlock;
	List_t m_cmdlist;
	List_t m_invalidlist;
	List_t m_connect_endlist;
	Hashtable_t m_objht;
	HashtableNode_t* m_objht_bulks[2048];
} Reactor_t;

typedef struct ReactorCmd_t {
	struct ListNode_t _;
	int type;
} ReactorCmd_t;
typedef struct ReactorObject_t {
/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	int detach_error;
	int detach_timeout_msec;
	unsigned int read_fragment_size;
	unsigned int write_fragment_size;
	struct {
		Sockaddr_t m_connect_addr;
		char m_connected;
		char m_listened;
		short max_connect_timeout_sec;
		long long m_connect_end_msec;
		ListNode_t m_connect_endnode;
	} stream;
	ReactorCmd_t regcmd;
	ReactorCmd_t freecmd;
/* private */
	List_t m_channel_list;
	HashtableNode_t m_hashnode;
	ListNode_t m_invalidnode;
	Atom8_t m_reghaspost;
	char m_valid;
	char m_has_inserted;
	char m_has_detached;
	char m_readol_has_commit;
	char m_writeol_has_commit;
	void* m_readol;
	void* m_writeol;
	long long m_invalid_msec;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
	int m_inbufsize;
} ReactorObject_t;

struct ReactorPacket_t;
typedef struct ChannelBase_t {
	ReactorCmd_t regcmd;
	ReactorCmd_t freecmd;
	Atom32_t refcnt;
	ReactorObject_t* o;
	Reactor_t* reactor;
	Sockaddr_t to_addr;
	union {
		Sockaddr_t listen_addr;
		Sockaddr_t connect_addr;
	};
	union {
		StreamTransportCtx_t stream_ctx;
		DgramTransportCtx_t dgram_ctx;
	};
	struct {
		ReactorCmd_t stream_sendfincmd;
		Atom8_t m_stream_has_sendfincmd;
		char m_stream_sendfinwait;
	};
	char has_recvfin;
	char has_sendfin;
	unsigned int connected_times; /* client use */
	char disable_send;
	char valid;
	unsigned short flag;
	int detach_error;
	long long event_msec;
	unsigned int write_fragment_size;
	unsigned int readcache_max_size;

	union {
		void(*on_ack_halfconn)(struct ChannelBase_t* self, FD_t newfd, const struct sockaddr* peer_addr, long long ts_msec); /* listener use */
		void(*on_syn_ack)(struct ChannelBase_t* self, long long ts_msec); /* client use */
	};
	void(*on_reg)(struct ChannelBase_t* self, long long timestamp_msec);
	void(*on_exec)(struct ChannelBase_t* self, long long timestamp_msec);
	int(*on_read)(struct ChannelBase_t* self, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const struct sockaddr* from_addr);
	int(*on_pre_send)(struct ChannelBase_t* self, struct ReactorPacket_t* packet, long long timestamp_msec);
	void(*on_detach)(struct ChannelBase_t* self);
	void(*on_free)(struct ChannelBase_t* self);
/* private */
	char m_has_detached;
	List_t m_cache_packet_list;
} ChannelBase_t;

typedef struct ReactorPacket_t {
	ReactorCmd_t cmd;
	ChannelBase_t* channel;
	NetPacket_t _;
} ReactorPacket_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Reactor_t* reactorInit(Reactor_t* reactor);
__declspec_dll void reactorWake(Reactor_t* reactor);
__declspec_dll void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode);
__declspec_dll int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, long long timestamp_msec, int wait_msec);
__declspec_dll void reactorDestroy(Reactor_t* reactor);

__declspec_dll ReactorObject_t* reactorobjectOpen(FD_t fd, int domain, int socktype, int protocol);

__declspec_dll ReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen);
__declspec_dll void reactorpacketFree(ReactorPacket_t* pkg);
__declspec_dll void reactorpacketFreeList(List_t* pkglist);

__declspec_dll ChannelBase_t* channelbaseOpen(size_t sz, unsigned short flag, ReactorObject_t* o, const struct sockaddr* addr);
__declspec_dll ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel);
__declspec_dll void channelbaseSendPacket(ChannelBase_t* channel, ReactorPacket_t* packet);
__declspec_dll void channelbaseSendPacketList(ChannelBase_t* channel, List_t* packetlist);

#ifdef	__cplusplus
}
#endif

#endif
