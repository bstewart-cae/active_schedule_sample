# Z-Wave Radio Test Tool

The Z-Wave Radio Test tool offers a UART CLI that can be used for performing
a number of RF tests on the chip.

The Radio Test Tool only contains the PHY layer of the Z-Wave protocol and
is targeted at performing tests on the PHY layer and in some cases the MAC
layer.

## UART configuration
Default UART configuration uses UART0 (pin 16,17) set to 230400,N,8,1
The UART configuration can be changed but requires a recompilation of the project.

## Using the Test Tool
1. Program the binary to a Z-Wave chip
2. Connect to the chip with a terminal program set to the correct UART and configuration
3. Type help for a list of commands

## Example of setting the test tool to US region and transmitting a frame on 100kbps
\> zw-region-set 1

\> zw-homeid-set 0x11111111

Setting homeID to 11111111

\> zw-init

\> zw-nodeid-set 2

Setting nodeID to 2

\>

\> zw-tx-channel-set 0

\> zw-tx-payload-set 11 11 11 11 02 41 06 17 01 01 01 D3 9C 01 10 00 5E 98 9F 55 6C

Setting payload 1111111102410617010101d39c0110005e989f556c

\> zw-tx-power-set 0

\> tx 1

Transmitting region = 1, channel = 0

\> Transmit complete, 0 success, 1 failed

# Using Radio Test Tool for transmitting an unmodulated carrier signal
The Radio Test Tool can be setup to enable a continues unmodulated
carrier signal on a specific channel in a specific Z-Wave region.
This using the zw-radio-tx-continues-set command. Here is an example of
enabling a continues unmodulated carrier signal on Z-Wave region US_LR,
channel 0 (100kbit GFSK channel):

First do power off/reset of DUT then enter in console

\> zw-region-set 9

Setting Z-Wave region to US_LR

\> zw-init

\> zw-tx-channel-set 0

\> zw-tx-power-set 0

\> zw-radio-tx-continues-set on 0

This should enable a measurable unmodulated carrier signal at around 916 MHz

To stop carrier signal do a power off/reset of device or

\> zw-radio-tx-continues-set off 0

# Using Radio Test Tool for determining which state the RF radio is in
The Radio Test Tool can be setup to indicate the RF Radio state on GPIOs.
To enable RF Radio state on GPIOs

\> zw-radio-rf-debug-set on

A text will ask you to reset module, this can be done either through hardware or through calling

\> reset

When module restarts it will output a text indicating the GPIOs and their meaning.
For disabling the RF Radio state on GPIOs feature

\> zw-radio-rf-debug-set off

A text will ask you to reset module, this can be done either through hardware or through calling

\> reset

When module restarts no RF Radio state text will be output.

When the RF Radio state on GPIO feature has been enabled
the Radio Test Tool will use GPIO 0, GPIO 21 and GPIO 28

The GPIO 0 show RF radio CPU_STATE, if HIGH the RF radio CPU is active, if LOW the RF radio CPU is in sleep.

The GPIO 21 shows RF RX state, HIGH for RF RX state ON.

The GPIO 28 shows RF TX state, HIGH for RF TX state ON.

# Troubleshooting

## My newly added command does not work in a script.
Make sure `cli_radio_script_state_transition_event()` is invoked in the end of the command handler
on a successful execution of the command.
