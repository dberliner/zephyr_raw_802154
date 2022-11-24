#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_example, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <stdio.h>

#include "net_private.h"
#include <ieee802154_frame.h>

#define MAX_PACKET_LEN 127

static void pkt_hexdump(uint8_t *pkt, uint8_t length)
{
	int i;

	printk(" -> Packet content:");

	for (i = 0; i < length;) {
		int j;

		printk("\t");

		for (j = 0; j < 10 && i < length; j++, i++) {
			printk("%02x ", *pkt);
			pkt++;
		}

		printk("");
	}
}

struct net_if *iface;
struct net_pkt *current_pkt;
K_SEM_DEFINE(driver_lock, 0, UINT_MAX);

int main(int argc, char** argv) {
	int fd;
	struct sockaddr_ll socket_sll = {0};
	struct msghdr msg = {0};
	struct iovec io_vector;
	uint8_t payload[20];
	struct ieee802154_mpdu mpdu;
	bool result = false;


	const struct device *dev;

    /* INIT */
	k_sem_reset(&driver_lock);

	current_pkt = net_pkt_rx_alloc(K_FOREVER);
	if (!current_pkt) {
		NET_ERR("*** No buffer to allocate");
		return false;
	}

	dev = device_get_binding("ieee802154");
	if (!dev) {
		NET_ERR("*** Could not get HW device");
		return false;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		NET_ERR("*** Could not get HW iface");
		return false;
	}


    /* SEND */
	NET_INFO("- Sending RAW packet via AF_PACKET socket");
    fd = socket(AF_PACKET, SOCK_RAW, ETH_P_IEEE802154);
    if (fd < 0) {
		NET_ERR("*** Failed to create RAW socket : %d\n", errno);
        goto out;
    }

	socket_sll.sll_ifindex = net_if_get_by_iface(iface);
	socket_sll.sll_family = AF_PACKET;
	socket_sll.sll_protocol = ETH_P_IEEE802154;
	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	/* Construct raw packet payload, length and FCS gets added in the kernel */
	payload[0] = 0x21; /* Frame Control Field */
	payload[1] = 0xc8; /* Frame Control Field */
	payload[2] = 0x8b; /* Sequence number */
	payload[3] = 0xff; /* Destination PAN ID 0xffff */
	payload[4] = 0xff; /* Destination PAN ID */
	payload[5] = 0x02; /* Destination short address 0x0002 */
	payload[6] = 0x00; /* Destination short address */
	payload[7] = 0x23; /* Source PAN ID 0x0023 */
	payload[8] = 0x00; /* */
	payload[9] = 0x60; /* Source extended address ae:c2:4a:1c:21:16:e2:60 */
	payload[10] = 0xe2; /* */
	payload[11] = 0x16; /* */
	payload[12] = 0x21; /* */
	payload[13] = 0x1c; /* */
	payload[14] = 0x4a; /* */
	payload[15] = 0xc2; /* */
	payload[16] = 0xae; /* */
	payload[17] = 0xAA; /* Payload */
	payload[18] = 0xBB; /* */
	payload[19] = 0xCC; /* */
	io_vector.iov_base = payload;
	io_vector.iov_len = sizeof(payload);
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;

	if (sendmsg(fd, &msg, 0) != sizeof(payload)) {
		NET_ERR("*** Failed to send, errno %d\n", errno);
		goto release_fd;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	if (!current_pkt->frags) {
		NET_ERR("*** Could not send RAW packet");
		goto release_fd;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** Sent packet is not valid");
		goto release_frag;
	}

	if (memcmp(mpdu.payload, &payload[17], 3) != 0) {
		NET_ERR("*** Payload of sent packet is incorrect");
		goto release_frag;
	}

release_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
release_fd:
	close(fd);
out:

    /* Note: We can not let main fall out of scope */
	for (;;) {
		k_sleep(K_MSEC(1000));
	}
    return 0;
}