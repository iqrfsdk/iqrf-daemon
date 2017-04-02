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

#include <cstdint>

class Crc {
public:
  static Crc& get()
  {
    static Crc crc;
    return crc;
  }

  uint16_t GetCRC_CCITT(uint8_t *buf, uint16_t len)
  {
    uint16_t crc_ccitt;
    uint16_t i;

    //crc_tabccitt_init = false;
    crc_ccitt = 0;

    for (i = 0; i < len; i++)
    {
      crc_ccitt = UpdateCRC_CCITT(crc_ccitt, buf[i]);
    }

    return crc_ccitt;
  }

private:
  Crc()
  {
    InitTabCRC_CCITT();
  }

  /*******************************************************************\
  *                                                                   *
  *   uint16_t UpdateCRC_CCITT(uint16_t crc, uint8_t c);                *
  *                                                                   *
  *   The function update_crc_ccitt calculates  a  new  CRC-CCITT     *
  *   value  based  on the previous value of the CRC and the next     *
  *   byte of the data to be checked.                                 *
  *                                                                   *
  \*******************************************************************/

  uint16_t UpdateCRC_CCITT(uint16_t crc, uint8_t c)
  {
    uint16_t tmp, short_c;

    short_c = 0x00ff & (uint16_t)c;

    //if (!crc_tabccitt_init) InitTabCRC_CCITT();

    tmp = (crc >> 8) ^ short_c;
    crc = (crc << 8) ^ crc_tabccitt[tmp];

    return crc;
  }

  /*******************************************************************\
  *                                                                   *
  *   static void InitTabCRC_CCITT( void );                          *
  *                                                                   *
  *   The function init_crcccitt_tab() is used to fill the  array     *
  *   for calculation of the CRC-CCITT with values.                   *
  *                                                                   *
  \*******************************************************************/

  void InitTabCRC_CCITT()
  {
    int i, j;
    uint16_t crc, c;

    for (i = 0; i < 256; i++)
    {
      crc = 0;
      c = ((uint16_t)i) << 8;

      for (j = 0; j < 8; j++)
      {
        if ((crc ^ c) & 0x8000) crc = (crc << 1) ^ P_CCITT;
        else                     crc = crc << 1;

        c = c << 1;
      }

      crc_tabccitt[i] = crc;
    }

    //crc_tabccitt_init = true;
  }

  const uint16_t P_CCITT = 0x1021;

  //bool crc_tabccitt_init;
  uint16_t crc_tabccitt[256];

};
