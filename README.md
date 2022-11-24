# zephyr_raw_802154

This project demonstrates how to send IEEE 802.15.4 packets without using the IP stack in Zephyr. Most of the code is based on Zephyrs `tests/net/ieee802154/l2` raw packet test. This functionality was added in 3.2.0 so older versions cannot use this apprach but rather must reimplement the L2 stack themselves.

The developer is still forced to define their 802.15.4 packets on their own, though. There's probably a way around this but I haven't dug deep enough to find it yet. If you get to it before me please submit a PR.

## Compatability

I have only had the chance to test this on a nrf52840-dk. I will take PRs to make this example board agnostic.

## Building

## Contributing

If you make a fix or improvement please submit a pull request.
