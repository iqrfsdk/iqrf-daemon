/**
 * Copyright 2016-2017 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

typedef std::basic_string<unsigned char> ustring;

const unsigned IQRF_UDP_BUFFER_SIZE = 1024;

const unsigned char IQRF_UDP_GW_ADR = 0x20;    // 3rd party or user device
const unsigned char IQRF_UDP_HEADER_SIZE = 9; //header length
const unsigned char IQRF_UDP_CRC_SIZE = 2; // CRC has 2 bytes

//--- IQRF UDP commands (CMD) ---
const unsigned char IQRF_UDP_GET_GW_INFO = 0x01;	// Returns GW identification
const unsigned char IQRF_UDP_GET_GW_STATUS = 0x02;	// Returns GW status
const unsigned char IQRF_UDP_WRITE_IQRF = 0x03;	// Writes data to the TR module's SPI
const unsigned char IQRF_UDP_IQRF_SPI_DATA = 0x04;	// Data from TR module's SPI (async)

//--- IQRF UDP subcommands (SUBCMD) ---
const unsigned char IQRF_UDP_ACK = 0x50;	// Positive answer
const unsigned char IQRF_UDP_NAK = 0x60;	// Negative answer
const unsigned char IQRF_UDP_BUS_BUSY = 0x61;	// Communication channel (IQRF SPI or RS485) is busy

//TODO
enum IqrfWriteResults {
  wr_OK = 0x50,
  wr_Error_Len = 0x60, //(number of data = 0 or more than TR buffer COM length)
  wr_Error_SPI = 0x61, //(SPI bus busy)
  wr_Error_IQRF = 0x62 //(IQRF - CRCM Error)
};

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

//--- IQRF UDP GW status ---
enum UdpGwStatus
{
  trStatus,   //DB1 TR module status(see the IQRF SPI protocol)
  unused2,    //DB2 no used
  supplyExt,  //DB3 0x01 supplied from external source
  timeSec,    //DB4 GW time – seconds(see Time and date coding)
  timeMin,    //DB5 GW time – minutes
  timeHour,    //DB6 GW time – hours
  timeWday,    //DB7 GW date – day of the week
  timeMday,    //DB8 GW date – day
  timeMon,    //DB9 GW date – month
  timeYear,   //DB10 GW date – year
  unused11,   //DB11 no used
  unused12,   //DB12 no used
};
