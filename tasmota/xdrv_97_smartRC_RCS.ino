/*
  xdrv_17_rcswitch.ino - RF transceiver using RcSwitch library for Tasmota

  Copyright (C) 2021  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define USE_SMARTRC_RCS
#ifdef USE_SMARTRC_RCS
#undef USE_RC_SWITCH
#define USE_SPI
/*********************************************************************************************\
 * RF send and receive using RCSwitch library https://github.com/sui77/rc-switch/
\*********************************************************************************************/

#define XDRV_97 97

#define D_JSON_SMARTRC_PROTOCOL "Protocol"
#define D_JSON_SMARTRC_BITS "Bits"
#define D_JSON_SMARTRC_DATA "Data"

#define D_CMND_RFSEND "smartRCsend"

#define D_JSON_SMARTRC_PULSE "Pulse"
#define D_JSON_SMARTRC_REPEAT "Repeat"
#define D_JSON_NONE_ENABLED "None Enabled"

const char kRfCommands[] PROGMEM = "|" // No prefix
    D_CMND_RFSEND;

void (*const RfCommands[])(void) PROGMEM = {
    &CmndRfSend};

#include <RCSwitch.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#warning ****  USE_SMARTRC_RCS - hardcoded PINS****

RCSwitch mySwitch = RCSwitch();

void RfInit(void)
{

  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5);
  //ELECHOUSE_cc1101.setSpiPin(SCK, MISO, MOSI, CSN);
  if (ELECHOUSE_cc1101.getCC1101())
  { // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(433.92);
    mySwitch.enableTransmit(2);
    ELECHOUSE_cc1101.SetTx();
  }
  else
  {
  }
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

void CmndRfSend(void)
{
  bool error = false;
  if (ELECHOUSE_cc1101.getCC1101())
  {
    ResponseCmndDone();
  }

  if (XdrvMailbox.data_len)
  {
    unsigned long long data = 0; // unsigned long long  => support payload >32bit
    unsigned int bits = 24;
    int protocol = 1;
    int repeat = 10;
    int pulse = 0; // 0 leave the library use the default value depending on protocol

    JsonParser parser(XdrvMailbox.data);
    JsonParserObject root = parser.getRootObject();
    if (root)
    {
      // RFsend {"data":0x501014,"bits":24,"protocol":1,"repeat":10,"pulse":350}
      char parm_uc[10];
      data = root.getULong(PSTR(D_JSON_SMARTRC_DATA), data); // read payload data even >32bit
      bits = root.getUInt(PSTR(D_JSON_SMARTRC_BITS), bits);
      protocol = root.getInt(PSTR(D_JSON_SMARTRC_PROTOCOL), protocol);
      repeat = root.getInt(PSTR(D_JSON_SMARTRC_REPEAT), repeat);
      pulse = root.getInt(PSTR(D_JSON_SMARTRC_PULSE), pulse);
    }
    else
    {
      //  RFsend data, bits, protocol, repeat, pulse
      char *p;
      uint8_t i = 0;
      for (char *str = strtok_r(XdrvMailbox.data, ", ", &p); str && i < 5; str = strtok_r(nullptr, ", ", &p))
      {
        switch (i++)
        {
        case 0:
          data = strtoul(str, nullptr, 0); // Allow decimal (5246996) and hexadecimal (0x501014) input
          break;
        case 1:
          bits = atoi(str);
          break;
        case 2:
          protocol = atoi(str);
          break;
        case 3:
          repeat = atoi(str);
          break;
        case 4:
          pulse = atoi(str);
        }
      }
    }

    if (!protocol)
    {
      protocol = 1;
    }
    mySwitch.setProtocol(protocol);
    // if pulse is specified in the command, enforce the provided value (otherwise lib takes default)
    if (pulse)
    {
      mySwitch.setPulseLength(pulse);
    }
    if (!repeat)
    {
      repeat = 10;
    } // Default at init
    mySwitch.setRepeatTransmit(repeat);
    if (!bits)
    {
      bits = 24;
    } // Default 24 bits
    if (data)
    {
      mySwitch.send(data, bits);
      ResponseCmndDone();
    }
    else
    {
      error = true;
    }
  }
  else
  {
    error = true;
  }
  if (error)
  {
    Response_P(PSTR("{\"" D_CMND_RFSEND "\":\"" D_JSON_NO " " D_JSON_SMARTRC_DATA ", " D_JSON_SMARTRC_BITS ", " D_JSON_SMARTRC_PROTOCOL ", " D_JSON_SMARTRC_REPEAT " " D_JSON_OR " " D_JSON_SMARTRC_PULSE "\"}"));
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv97(uint8_t function)
{
  bool result = false;

  switch (function)
  {
  case FUNC_EVERY_50_MSECOND:
    break;
  case FUNC_COMMAND:
    result = DecodeCommand(kRfCommands, RfCommands);
    break;
  case FUNC_INIT:
    RfInit();
    break;
  }
  return result;
}

#endif // USE_RC_SWITCH
