/*
   Copyright (C) The Internet Society (2003).  All Rights Reserved.

   This document and translations of it may be copied and furnished to
   others, and derivative works that comment on or otherwise explain it
   or assist in its implementation may be prepared, copied, published
   and distributed, in whole or in part, without restriction of any
   kind, provided that the above copyright notice and this paragraph are
   included on all such copies and derivative works.  However, this
   document itself may not be modified in any way, such as by removing
   the copyright notice or references to the Internet Society or other
   Internet organizations, except as needed for the purpose of
   developing Internet standards in which case the procedures for
   copyrights defined in the Internet Standards process must be
   followed, or as required to translate it into languages other than
   English.

   The limited permissions granted above are perpetual and will not be
   revoked by the Internet Society or its successors or assigns.

   This document and the information contained herein is provided on an
   "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
   TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
   BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
   HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "mast.h"

void
mast_rtp_init_sequence(mast_rtp_sequence_state_t *s, uint16_t seq)
{
    s->base_seq = seq;
    s->max_seq = seq;
    s->bad_seq = RTP_SEQ_MOD + 1;
    s->cycles = 0;
    s->received = 0;
    s->received_prior = 0;
    s->expected_prior = 0;
}

int
mast_rtp_update_sequence(mast_rtp_sequence_state_t *s, uint16_t seq)
{
    uint16_t udelta = seq - s->max_seq;
#define MAX_DROPOUT 3000
#define MAX_MISORDER 100

    /*
     * Source is not valid until RTP_MIN_SEQUENTIAL packets with
     * sequential sequence numbers have been received.
     */
    if (s->probation) {
	/* packet is in sequence */
	if (seq == s->max_seq + 1) {
	    s->probation--;
	    s->max_seq = seq;
	    if (s->probation == 0) {
		mast_rtp_init_sequence(s, seq);
		s->received++;
		return TRUE;
	    }
	} else {
	    s->probation = RTP_MIN_SEQUENTIAL - 1;
	    s->max_seq = seq;
	}
	return FALSE;
    } else if (udelta < MAX_DROPOUT) {
	/* in order, with permissible gap */
	if (seq < s->max_seq) {
	    /*
	     * Sequence number wrapped - count another 64K cycle.
	     */
	    s->cycles += RTP_SEQ_MOD;
	}
	s->max_seq = seq;
    } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
	/* the sequence number made a very large jump */
	if (seq == s->bad_seq) {
	    /*
	     * Two sequential packets -- assume that the other side
	     * restarted without telling us so just re-sync
	     * (i.e., pretend this was the first packet).
	     */
	    mast_rtp_init_sequence(s, seq);
	} else {
	    s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
	    return FALSE;
	}
    } else {
	/* duplicate or reordered packet */
    }

    s->received++;
    return TRUE;
}
