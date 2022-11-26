#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_example, LOG_LEVEL_DBG);

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

#include "net_private.h"
#include <ieee802154_frame.h>

#define MAX_PACKET_LEN 127

static void pkt_hexdump(uint8_t *pkt, uint8_t length) {
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
  printk("\n");
}

struct net_if *iface;

static int set_up_short_addr(struct net_if *iface, struct ieee802154_context *ctx, uint16_t short_addr)
{
	int ret = net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface, &short_addr,
			   sizeof(short_addr));
	if (ret) {
		NET_ERR("*** Failed to set short address\n");
	}

	return ret;
}

static int tear_down_short_addr(struct net_if *iface, struct ieee802154_context *ctx)
{
	uint16_t short_addr;

	if (ctx->linkaddr.len != IEEE802154_SHORT_ADDR_LENGTH) {
		/* nothing to do */
		return 0;
	}

	short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	int ret = net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface, &short_addr,
			   sizeof(short_addr));
	if (ret) {
		NET_ERR("*** Failed to unset short address\n");
	}

	return ret;
}

bool setup() {
  const struct device *dev;

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

  return true;
}

int main(int argc, char **argv) {
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	int fd;
	uint16_t dst_short_addr = htons(0x1337);
  uint16_t src_short_addr = 0xBEEF;
	struct sockaddr_ll socket_sll = {
		.sll_halen = sizeof(dst_short_addr),
    .sll_family = AF_PACKET,
    .sll_protocol = ETH_P_IEEE802154
	};
	memcpy(socket_sll.sll_addr, &dst_short_addr, sizeof(dst_short_addr));

	uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
	struct ieee802154_mpdu mpdu;
	bool result = false;

  /* Set up the interface */
  if (!setup()) {
    NET_ERR("Could not set up interface. Exiting.");
    goto out;
  }

  /* Set up our address */
	set_up_short_addr(iface, ctx, src_short_addr);

	NET_INFO("- Sending DGRAM packet via AF_PACKET socket\n");

  /* Set up the socket to send to */
	fd = socket(AF_PACKET, SOCK_DGRAM, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create DGRAM socket : %d\n", errno);
		goto out;
	}

  /* Set up the dest address. */
  socket_sll.sll_ifindex = net_if_get_by_iface(iface);
	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

  /* Actually send */
	if (sendto(fd, payload, sizeof(payload), 0, (const struct sockaddr *)&socket_sll,
		   sizeof(struct sockaddr_ll)) != sizeof(payload)) {
		NET_ERR("*** Failed to send, errno %d\n", errno);
		goto release_fd;
	}

	k_yield();

release_fd:
	tear_down_short_addr(iface, ctx);
	close(fd);
out:
  return 0;
}