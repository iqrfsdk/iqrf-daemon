#pragma once

#include <string>

typedef std::basic_string<unsigned char> ustring;

const unsigned IQRF_UDP_BUFFER_SIZE = 1024;

const unsigned char IQRF_UDP_GW_ADR = 0x20;    // 3rd party or user device
unsigned char IQRF_UDP_HEADER_SIZE = 9; //header length
unsigned char IQRF_UDP_CRC_SIZE = 2; // CRC has 2 bytes

//--- IQRF UDP commands (CMD) ---
const unsigned char IQRF_UDP_GET_GW_INFO = 0x01;	// Returns GW identification
const unsigned char IQRF_UDP_GET_GW_STATUS = 0x02;	// Returns GW status
const unsigned char IQRF_UDP_WRITE_IQRF = 0x03;	// Writes data to the TR module's SPI
const unsigned char IQRF_UDP_IQRF_SPI_DATA = 0x04;	// Data from TR module's SPI (async)

//--- IQRF UDP subcommands (SUBCMD) ---
const unsigned char IQRF_UDP_ACK = 0x50;	// Positive answer
const unsigned char IQRF_UDP_NAK = 0x60;	// Negative answer
const unsigned char IQRF_UDP_BUS_BUSY = 0x61;	// Communication channel (IQRF SPI or RS485) is busy

//--- IQRF UDP header ---
enum UdpHeader
{
  gwAddr,
  cmd,
  subcmd,
  res0,
  res1,
  pacid_H,
  pacid_L,
  dlen_H,
  dlen_L
};
