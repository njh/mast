

u_int32_t rtp_packet_count=0;
u_int32_t rtp_octet_count=0;



struct {
	rtcp_common_t sr_common;
	struct {
		u_int32_t ssrc;     /* sender generating this report */
		u_int32_t ntp_sec;  /* NTP timestamp */
		u_int32_t ntp_frac;
		u_int32_t rtp_ts;   /* RTP timestamp */
		u_int32_t psent;    /* packets sent */
		u_int32_t osent;    /* octets sent */ 
		//rtcp_rr_t rr[1];  /* variable-length list */
	} sr;
	
	
	rtcp_common_t sdes_common;
	u_int32_t sdes_ssrc;      /* first SSRC/CSRC */
	rtcp_sdes_item_t sdes_cname;	
	rtcp_sdes_item_t sdes_name;	
	
} rtcp_packet;





static void
send_rtcp_packet( udp_socket_t* rtcp_socket ) {
	int packet_len = sizeof(rtcp_packet.sr_common) + sizeof(rtcp_packet.sr) +
	                 sizeof(rtcp_packet.sdes_common) +
	                 4 + sizeof(rtcp_packet.sdes_cname) + sizeof(rtcp_packet.sdes_name);
	int sr_plen = sizeof(rtcp_packet.sr)/4;
	int sdes_plen = (4+sizeof(rtcp_packet.sdes_cname)+sizeof(rtcp_packet.sdes_name))/4;
	struct timeval now;
	
	fprintf(stderr, "  Sending RTCP packet: len=%d srlen=%d sdes_plen=%d\n", 
				packet_len, (int)sizeof(rtcp_packet.sr), 4+sizeof(rtcp_packet.sdes_cname));

	/* The SR */
	rtcp_packet.sr_common.version = RTP_VERSION;
	rtcp_packet.sr_common.p = 0;
	rtcp_packet.sr_common.count = 0;
	rtcp_packet.sr_common.pt = RTCP_SR;
	rtcp_packet.sr_common.length = sr_plen;
	rtcp_packet.sr.ssrc = htonl(config.ssrc);
	
	/* set the NTP fields */
	gettimeofday(&now, NULL);
	rtcp_packet.sr.ntp_sec = htonl(now.tv_sec + NTP_EPOCH_OFFSET);
	rtcp_packet.sr.ntp_frac = htonl((u_int32_t)(((double)(now.tv_usec)*(u_int32_t)(~0))/1000000.0));

	rtcp_packet.sr.rtp_ts = htonl(rtp_timestamp);	/* Supposed to be caculated from NTP ? */
	rtcp_packet.sr.psent = htonl(rtp_packet_count);
	rtcp_packet.sr.osent = htonl(rtp_octet_count);
	
	
	/* The SDES */
	rtcp_packet.sdes_common.version = RTP_VERSION;
	rtcp_packet.sdes_common.p = 0;
	rtcp_packet.sdes_common.count = 2;
	rtcp_packet.sdes_common.pt = RTCP_SDES;
	rtcp_packet.sdes_common.length = sdes_plen;
	rtcp_packet.sdes_ssrc = htonl(config.ssrc);
	
	rtcp_packet.sdes_cname.type = RTCP_SDES_CNAME;
	rtcp_packet.sdes_cname.length = 30;
	memcpy(rtcp_packet.sdes_cname.data, "njh@hwickabab.ecs.soton.ac.uk.", 30);
	
	rtcp_packet.sdes_name.type = RTCP_SDES_NAME;
	rtcp_packet.sdes_name.length = 30;
	memcpy(rtcp_packet.sdes_name.data,  "Mr   Nicholas   James  Humfrey", 30);
	
	
	udp_socket_send( rtcp_socket, &rtcp_packet, packet_len );
}






		/* Send an RTCP packet every 5 seconds or so */
		octet_count += bytes_read;
		if (octet_count > 220500) {
			//send_rtcp_packet( rtcp_socket );
			octet_count = 0;
		}



