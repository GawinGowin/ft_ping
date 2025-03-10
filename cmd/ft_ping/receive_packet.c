#include "ft_ping.h"

static double calculate_rtt(struct timeval *send_time) {
  struct timeval recv_time;
  struct timeval diff;
  gettimeofday(&recv_time, NULL);
  
  /* Calculate time difference */
  diff.tv_sec = recv_time.tv_sec - send_time->tv_sec;
  diff.tv_usec = recv_time.tv_usec - send_time->tv_usec;
  if (diff.tv_usec < 0) {
    diff.tv_sec--;
    diff.tv_usec += 1000000;
  }
  
  /* Convert to milliseconds */
  return (double)diff.tv_sec * 1000.0 + (double)diff.tv_usec / 1000.0;
}

static void update_statistics(t_ping_master *master, double rtt) {
  /* Update statistics */
  master->stats.received++;
  
  /* Update RTT statistics */
  if (master->stats.received == 1) {
    master->stats.rtt_min = rtt;
    master->stats.rtt_max = rtt;
  } else {
    if (rtt < master->stats.rtt_min)
      master->stats.rtt_min = rtt;
    if (rtt > master->stats.rtt_max)
      master->stats.rtt_max = rtt;
  }
  
  master->stats.rtt_sum += rtt;
  master->stats.rtt_sum_sq += rtt * rtt;
}

static void print_packet_info(t_ping_master *master, void *packet, size_t packet_size, double rtt, int bytes_received, struct sockaddr_in *from) {
  struct ip *ip_header = (struct ip *)packet;
  int hlen = ip_header->ip_hl << 2;
  t_icmp *icmp = (t_icmp *)(packet + hlen);
  
  printf("%d bytes from %s: icmp_seq=%u ttl=%d time=%.1f ms\n",
         bytes_received - hlen,
         inet_ntoa(from->sin_addr),
         ntohs(icmp->seq),
         ip_header->ip_ttl,
         rtt);
}

static void print_error_message(int error_code, struct sockaddr_in *from) {
  char *error_msg;
  
  switch (error_code) {
    case ICMP_DEST_UNREACH:
      error_msg = "Destination Unreachable";
      break;
    case ICMP_SOURCE_QUENCH:
      error_msg = "Source Quench";
      break;
    case ICMP_REDIRECT:
      error_msg = "Redirect";
      break;
    case ICMP_TIME_EXCEEDED:
      error_msg = "Time Exceeded";
      break;
    case ICMP_PARAMETERPROB:
      error_msg = "Parameter Problem";
      break;
    default:
      error_msg = "Unknown ICMP Error";
      break;
  }
  
  printf("From %s: %s\n", inet_ntoa(from->sin_addr), error_msg);
}

int receive_packet(t_ping_master *master, void *packet, size_t packet_size) {
  struct msghdr msg;
  struct iovec iov;
  struct sockaddr_in from;
  char control_buf[512];  /* For control messages */
  int bytes_received;
  
  /* Set up message header */
  memset(&msg, 0, sizeof(msg));
  iov.iov_base = packet;
  iov.iov_len = packet_size + sizeof(struct ip);
  msg.msg_name = &from;
  msg.msg_namelen = sizeof(from);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control_buf;
  msg.msg_controllen = sizeof(control_buf);
  
  /* Receive packet */
  bytes_received = recvmsg(master->sockfd, &msg, MSG_DONTWAIT);
  if (bytes_received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      /* No data available (not an error) */
      return 0;
    }
    if (errno == EINTR) {
      /* Interrupted by signal */
      return 0;
    }
    if (master->opt_verbose) {
      fprintf(stderr, "recvmsg error: %s\n", strerror(errno));
    }
    return -1;
  }
  
  /* Check if packet is too small to be valid */
  if (bytes_received < (int)(sizeof(struct ip) + sizeof(t_icmp))) {
    if (master->opt_verbose) {
      fprintf(stderr, "Packet too short (%d bytes) from %s\n", 
              bytes_received, inet_ntoa(from.sin_addr));
    }
    master->stats.errors++;
    return 0;
  }
  
  /* Get IP header */
  struct ip *ip_header = (struct ip *)packet;
  int hlen = ip_header->ip_hl << 2;  /* IP header length in bytes */
  
  /* Check if packet is too small to contain a complete ICMP header */
  if (bytes_received < hlen + 8) {
    if (master->opt_verbose) {
      fprintf(stderr, "Packet too short from %s (IP header: %d bytes)\n", 
              inet_ntoa(from.sin_addr), hlen);
    }
    master->stats.errors++;
    return 0;
  }
  
  /* Get ICMP header */
  t_icmp *icmp = (t_icmp *)(packet + hlen);
  
  /* Check if this is an echo reply */
  if (icmp->type == ICMP_ECHOREPLY) {
    /* Check if this packet is for us (matching ID) */
    if (ntohs(icmp->id) != master->pid) {
      if (master->opt_verbose) {
        fprintf(stderr, "Received ICMP echo reply with wrong ID: %d (expected %d)\n", 
                ntohs(icmp->id), master->pid);
      }
      return 0;
    }
    
    /* Extract timestamp and calculate RTT */
    struct timeval *send_time = (struct timeval *)&icmp->timestamp;
    double rtt = calculate_rtt(send_time);
    
    /* Update statistics and print packet info */
    update_statistics(master, rtt);
    print_packet_info(master, packet, packet_size, rtt, bytes_received, &from);
    
    return 1;
  } 
  /* Handle ICMP error messages */
  else if (icmp->type != ICMP_ECHO) {
    /* For ICMP error messages, the original packet is included after the ICMP header */
    struct ip *orig_ip = (struct ip *)(packet + hlen + 8);
    int orig_hlen = orig_ip->ip_hl << 2;
    
    /* Check if error message contains at least IP+ICMP headers from original packet */
    if (bytes_received < hlen + 8 + orig_hlen + 8) {
      return 0;
    }
    
    /* Get original ICMP header from the error message */
    t_icmp *orig_icmp = (t_icmp *)((char *)orig_ip + orig_hlen);
    
    /* Check if the original packet was ours */
    if (ntohs(orig_icmp->id) != master->pid) {
      return 0;
    }
    
    /* Process error message */
    master->stats.errors++;
    print_error_message(icmp->type, &from);
    
    return 1;
  }
  
  return 0;
}
