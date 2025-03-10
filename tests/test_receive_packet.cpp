#include "../cmd/ft_ping/ft_ping.h"
#include <gtest/gtest.h>
#include <netinet/ip_icmp.h>

/* Mock function for recvmsg */
ssize_t recvmsg_mock(int sockfd, struct msghdr *msg, int flags);

/* Global variables for the mock */
static uint8_t mock_packet[1024];
static size_t mock_packet_size = 0;
static int mock_recvmsg_return = 0;
static struct sockaddr_in mock_from;

/* Setup the mock packet for testing */
void setup_mock_packet(uint8_t type, uint16_t id, uint16_t seq, struct timeval *timestamp) {
    /* Create a mock IP header */
    struct ip *ip_header = (struct ip *)mock_packet;
    ip_header->ip_hl = 5;  /* header length in 32-bit words */
    ip_header->ip_v = 4;   /* IPv4 */
    ip_header->ip_ttl = 64;
    ip_header->ip_p = IPPROTO_ICMP;
    ip_header->ip_len = htons(sizeof(struct ip) + sizeof(t_icmp));
    ip_header->ip_src.s_addr = inet_addr("192.168.1.1");
    ip_header->ip_dst.s_addr = inet_addr("192.168.1.2");
    
    /* Create ICMP header after IP header */
    t_icmp *icmp = (t_icmp *)(mock_packet + sizeof(struct ip));
    icmp->type = type;
    icmp->code = 0;
    icmp->id = htons(id);
    icmp->seq = htons(seq);
    
    /* Set timestamp if provided */
    if (timestamp) {
        memcpy(&icmp->timestamp, timestamp, sizeof(struct timeval));
    }
    
    /* Calculate checksum */
    icmp->checksum = 0;
    icmp->checksum = calculate_checksum(icmp, sizeof(t_icmp));
    
    /* Set mock packet size */
    mock_packet_size = sizeof(struct ip) + sizeof(t_icmp);
    
    /* Setup source address for mock */
    memset(&mock_from, 0, sizeof(mock_from));
    mock_from.sin_family = AF_INET;
    mock_from.sin_addr.s_addr = inet_addr("192.168.1.1");
}

/* Override recvmsg function for testing */
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    if (mock_recvmsg_return <= 0) {
        return mock_recvmsg_return;
    }
    
    /* Copy the mock packet data */
    memcpy(msg->msg_iov[0].iov_base, mock_packet, mock_packet_size);
    
    /* Set the source address */
    memcpy(msg->msg_name, &mock_from, sizeof(mock_from));
    
    return mock_packet_size;
}

/* Test receive_packet with echo reply */
TEST(ReceivePacketTest, EchoReplySuccess) {
    t_ping_master master;
    memset(&master, 0, sizeof(master));
    
    /* Setup master */
    master.pid = 1234;  /* Process ID to match in packet */
    
    /* Setup mock packet */
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    timestamp.tv_sec -= 1;  /* Make it 1 second old for RTT calculation */
    
    setup_mock_packet(ICMP_ECHOREPLY, master.pid, 42, &timestamp);
    mock_recvmsg_return = mock_packet_size;
    
    /* Buffer for receive_packet */
    uint8_t buffer[1024];
    
    /* Call receive_packet */
    int result = receive_packet(&master, buffer, sizeof(t_icmp));
    
    /* Verify result */
    EXPECT_EQ(result, 1);
    EXPECT_EQ(master.stats.received, 1);
    EXPECT_GT(master.stats.rtt_min, 0);
    EXPECT_GT(master.stats.rtt_max, 0);
    EXPECT_GT(master.stats.rtt_sum, 0);
}

/* Test receive_packet with wrong ID */
TEST(ReceivePacketTest, WrongIdIgnored) {
    t_ping_master master;
    memset(&master, 0, sizeof(master));
    
    /* Setup master */
    master.pid = 1234;  /* Process ID to match in packet */
    
    /* Setup mock packet with wrong ID */
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    
    setup_mock_packet(ICMP_ECHOREPLY, master.pid + 1, 42, &timestamp); /* Different ID */
    mock_recvmsg_return = mock_packet_size;
    
    /* Buffer for receive_packet */
    uint8_t buffer[1024];
    
    /* Call receive_packet */
    int result = receive_packet(&master, buffer, sizeof(t_icmp));
    
    /* Verify packet was ignored */
    EXPECT_EQ(result, 0);
    EXPECT_EQ(master.stats.received, 0);
}

/* Test receive_packet with error message */
TEST(ReceivePacketTest, ErrorMessageProcessed) {
    t_ping_master master;
    memset(&master, 0, sizeof(master));
    
    /* Setup master */
    master.pid = 1234;
    
    /* Create a mock ICMP error message with our original packet embedded */
    uint8_t error_packet[1024];
    struct ip *ip_header = (struct ip *)error_packet;
    ip_header->ip_hl = 5;
    ip_header->ip_v = 4;
    ip_header->ip_ttl = 64;
    ip_header->ip_p = IPPROTO_ICMP;
    ip_header->ip_len = htons(sizeof(struct ip) + 8 + sizeof(struct ip) + sizeof(t_icmp));
    
    /* ICMP error header */
    struct icmp *error_icmp = (struct icmp *)(error_packet + sizeof(struct ip));
    error_icmp->icmp_type = ICMP_DEST_UNREACH;
    error_icmp->icmp_code = ICMP_HOST_UNREACH;
    
    /* Original IP header in the error message */
    struct ip *orig_ip = (struct ip *)(error_packet + sizeof(struct ip) + 8);
    orig_ip->ip_hl = 5;
    orig_ip->ip_v = 4;
    orig_ip->ip_ttl = 64;
    orig_ip->ip_p = IPPROTO_ICMP;
    orig_ip->ip_len = htons(sizeof(struct ip) + sizeof(t_icmp));
    
    /* Original ICMP header with our ID */
    t_icmp *orig_icmp = (t_icmp *)(error_packet + sizeof(struct ip) + 8 + sizeof(struct ip));
    orig_icmp->type = ICMP_ECHO;
    orig_icmp->id = htons(master.pid);
    orig_icmp->seq = htons(42);
    
    /* Setup for recvmsg mock */
    memcpy(mock_packet, error_packet, sizeof(struct ip) + 8 + sizeof(struct ip) + sizeof(t_icmp));
    mock_packet_size = sizeof(struct ip) + 8 + sizeof(struct ip) + sizeof(t_icmp);
    mock_recvmsg_return = mock_packet_size;
    
    /* Buffer for receive_packet */
    uint8_t buffer[1024];
    
    /* Call receive_packet */
    int result = receive_packet(&master, buffer, sizeof(t_icmp));
    
    /* Verify error was processed */
    EXPECT_EQ(result, 1);
    EXPECT_EQ(master.stats.errors, 1);
}

/* Test receive_packet timeout */
TEST(ReceivePacketTest, Timeout) {
    t_ping_master master;
    memset(&master, 0, sizeof(master));
    
    /* Simulate no data available */
    mock_recvmsg_return = -1;
    errno = EAGAIN;
    
    /* Buffer for receive_packet */
    uint8_t buffer[1024];
    
    /* Call receive_packet */
    int result = receive_packet(&master, buffer, sizeof(t_icmp));
    
    /* Verify handled correctly */
    EXPECT_EQ(result, 0);
}
