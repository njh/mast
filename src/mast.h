/*
  mast.h

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT
*/

#include "config.h"

#ifndef MAST_H
#define MAST_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <math.h>

#include <sndfile.h>


#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif


#define MAST_DEFAULT_PORT           "5004"
#define MAST_DEFAULT_SAMPLE_RATE    48000
#define MAST_DEFAULT_ENCODING       MAST_ENCODING_L24
#define MAST_DEFAULT_CHANNEL_COUNT  2
#define MAST_MAX_CHANNEL_COUNT      64



// ------- Network Sockets ---------

typedef struct
{
    int fd;
    int joined_group;
    unsigned int if_index;

    struct sockaddr_storage dest_addr;
    struct sockaddr_storage src_addr;

    union {
        struct ipv6_mreq imr6;
        struct ip_mreq imr;
    };

} mast_socket_t;


int mast_socket_open_recv(mast_socket_t* sock, const char* address, const char* port, const char *ifname);
int mast_socket_open_send(mast_socket_t* sock, const char* address, const char* port, const char *ifname);
int mast_socket_recv(mast_socket_t* sock, void* data, unsigned int len);
int mast_socket_send(mast_socket_t* sock, void* data, unsigned int len);
void mast_socket_close(mast_socket_t* sock);


// ------- Audio Peak measurement ---------

#define MAST_POWER_TO_DB(power)    (20.0f * log10f(power))

void mast_peak_init(int channels);
float mast_peak_read_and_reset(int channel);
float mast_peak_read_and_reset_all();
void mast_peak_process_l16(uint8_t* payload, int payload_length);
void mast_peak_process_l24(uint8_t* payload, int payload_length);


// ------- SAP packet handling ---------

#define MAST_SAP_ADDRESS_LOCAL  "239.255.255.255"
#define MAST_SAP_ADDRESS_ORG    "239.195.255.255"
#define MAST_SAP_ADDRESS_GLOBAL "224.2.127.254"
#define MAST_SAP_PORT           "9875"

#define MAST_SAP_MAX_HEADER     (36)
#define MAST_SAP_MAX_LEN        (MAST_SAP_MAX_HEADER + MAST_SDP_MAX_LEN)

enum
{
    MAST_SAP_MESSAGE_ANNOUNCE = 0,
    MAST_SAP_MESSAGE_DELETE = 1
};

typedef struct
{
    uint8_t message_type;
    uint16_t message_id_hash;
    char message_source[INET6_ADDRSTRLEN];

    char sdp[2048];
} mast_sap_t;


int mast_sap_parse(const uint8_t* data, size_t data_len, mast_sap_t* sap);
int mast_sap_generate(mast_socket_t *sock, const char* sdp, uint8_t message_type, uint8_t *buffer, size_t buffer_len);
int mast_sap_send_sdp_string(mast_socket_t *sock, const char* sdp, uint8_t message_type);


// ------- SDP handling ---------

#define MAST_SDP_MIME_TYPE      "application/sdp"
#define MAST_SDP_MAX_LEN        (2048)

typedef struct
{
    char address[NI_MAXHOST];  // c=
    char port[NI_MAXSERV];     // m=

    char session_id[256];      // o=
    char session_origin[256];  // o=
    char session_name[256];    // s=
    char information[256];     // i=

    int payload_type;          // m=
    int encoding;              // a=rtpmap
    int sample_size;           // Size of the samples based on the encoding (16/24)
    int sample_rate;           // a=rtpmap
    int channel_count;         // a=rtpmap
    double packet_duration;    // a=ptime

    char ptp_gmid[24];         // a=ts-refclk
    uint64_t clock_offset;     // a=mediaclk
} mast_sdp_t;


int mast_sdp_parse_string(const char* str, mast_sdp_t* sdp);
int mast_sdp_parse_file(const char* filename, mast_sdp_t* sdp);

void mast_sdp_set_defaults(mast_sdp_t *sdp);
int mast_sdp_is_valid(mast_sdp_t *sdp);
void mast_sdp_set_payload_type(mast_sdp_t *sdp, int payload_type);
void mast_sdp_set_port(mast_sdp_t *sdp, const char *port);
void mast_sdp_set_address(mast_sdp_t *sdp, const char *address);
void mast_sdp_set_encoding(mast_sdp_t *sdp, int encoding);
void mast_sdp_set_encoding_name(mast_sdp_t *sdp, const char* encoding_name);



// ------- RTP packet handling ---------

#define RTP_MAX_PAYLOAD     (1440)
#define RTP_HEADER_LENGTH   (12)

typedef struct
{
    uint8_t version;
    uint8_t padding;
    uint8_t extension;
    uint8_t csrc_count;
    uint8_t marker;
    uint8_t payload_type;

    uint16_t sequence;
    uint32_t timestamp;
    uint32_t ssrc;

    uint16_t payload_length;
    uint8_t *payload;

    uint16_t length;
    uint8_t buffer[1500];

} mast_rtp_packet_t;

int mast_rtp_parse( mast_rtp_packet_t* packet );
int mast_rtp_recv( mast_socket_t* socket, mast_rtp_packet_t* packet );



// ------- Audio File Writing ---------

SNDFILE *mast_writer_open(const char* path, mast_sdp_t *sdp);
void mast_writer_write(SNDFILE *file, uint8_t* payload, int payload_length);


// ------- Utilities ---------

enum {
    MAST_ENCODING_L8,
    MAST_ENCODING_L16,
    MAST_ENCODING_L24,
    MAST_ENCODING_PCMU,   // G.711 - uLaw
    MAST_ENCODING_PCMA,   // G.711 - aLaw
    MAST_ENCODING_G722,   // SB-ADPCM
    MAST_ENCODING_GSM,
    MAST_ENCODING_MAX
} mast_encodings;

void setup_signal_hander();
extern int running;
extern int exit_code;
extern int verbose;
extern int quiet;

typedef enum {
    mast_LOG_DEBUG,
    mast_LOG_INFO,
    mast_LOG_WARN,
    mast_LOG_ERROR
} mast_log_level;


void mast_log(mast_log_level level, const char *fmt, ...);

// Only display debug if verbose
#define mast_debug( ... ) \
		mast_log(mast_LOG_DEBUG, __VA_ARGS__ )

// Don't show info when quiet
#define mast_info( ... ) \
		mast_log(mast_LOG_INFO, __VA_ARGS__ )

#define mast_warn( ... ) \
		mast_log(mast_LOG_WARN, __VA_ARGS__ )

// All errors are fatal
#define mast_error( ... ) \
		mast_log(mast_LOG_ERROR, __VA_ARGS__ )

int mast_read_file_string(const char* filename, char* buffer, size_t buffer_len);

int mast_directory_exists(const char* path);

const char* mast_encoding_name(int encoding);
int mast_encoding_lookup(const char* name);

#endif
