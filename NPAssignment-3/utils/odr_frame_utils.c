/*
 * odr_frame_utils.c
 *
 *  Created on: Nov 12, 2015
 *      Author: sujan
 */

#include "../src/usp.h"


/**
 *  Method to build the header of the odr_frame.
 */
struct odr_hdr build_odr_hdr(char *src,char *dst,int hop_count,int pkt_type,
		int broadcast_id, int frc_dsc,int rreply_sent){

	struct odr_hdr hdr;
	strcpy(hdr.cn_src_ipaddr, src);
	strcpy(hdr.cn_dsc_ipaddr, dst);
	hdr.force_route_dcvry = frc_dsc;
	hdr.hop_count = hop_count;
	hdr.pkt_type = pkt_type;
	hdr.broadcast_id = broadcast_id;
	hdr.rreply_sent = rreply_sent;

	convertToNetworkOrder(&hdr);
	return hdr;
}

/**
 *  Method to build the frame.
 */
struct odr_frame build_odr_frame(char *src,char *dst,int hop_count,int pkt_type, int broadcast_id,
			int frc_dsc,int rreply_sent,char *pay_load){

		struct odr_frame frame;

		frame.hdr = build_odr_hdr(src,dst,hop_count,pkt_type, broadcast_id, frc_dsc,rreply_sent);
		if(pkt_type == PAY_LOAD){
			strcpy(frame.payload,pay_load);
		}

		return frame;
}

/**
 *  This method need to be used after filling in the routing table.
 */
void build_rreply_odr_frame(struct odr_frame *rrep_frame){

	char *temp = (char *)malloc(20*sizeof(char));

	strcpy(temp,rrep_frame->hdr.cn_src_ipaddr);
	strcpy(rrep_frame->hdr.cn_src_ipaddr,rrep_frame->hdr.cn_dsc_ipaddr);
	strcpy(rrep_frame->hdr.cn_dsc_ipaddr,temp);

	rrep_frame->hdr.hop_count = 0;

}
