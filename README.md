# zephyr_raw_802154

This project demonstrates how to send IEEE 802.15.4 packets without using the IP stack in Zephyr. Most of the code is based on Zephyrs `tests/net/ieee802154/l2` raw packet test. This functionality was added in 3.2.0 so older versions cannot use this apprach but rather must reimplement the L2 stack themselves.

The added functionality allows you to use the DGRAM message type to send "plain" 802.15.4 messages. You can also take complete manual control of the sent data by sending the RAW type, the L2 test contians code for this as well.

## Compatability

I have only had the chance to test this on a nrf52840-dk. I will take PRs to make this example board agnostic.

## Building

## Contributing

If you make a fix or improvement please submit a pull request.
