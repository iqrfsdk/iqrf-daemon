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

#include "LaunchUtils.h"
#include "JsonSerializer.h"
#include "DpaTransactionTask.h"
#include "DpaMessage.h"
#include "IqrfLogging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"
#include <vector>
#include <utility>
#include <stdexcept>

 //TODO using istream is slower according http://rapidjson.org/md_doc_stream.html

using namespace rapidjson;

INIT_COMPONENT(ISerializer, JsonSerializer)

#define CTYPE_STR "ctype"
#define TYPE_STR "type"
#define NADR_STR "nadr"
#define HWPID_STR "hwpid"
#define TIMEOUT_STR "timeout"
#define MSGID_STR "msgid"
#define REQUEST_STR "request"
#define REQUEST_TS_STR "request_ts"
#define RESPONSE_STR "response"
#define RESPONSE_TS_STR "response_ts"
#define CONFIRMATION_STR "confirmation"
#define CONFIRMATION_TS_STR "confirmation_ts"
#define CMD_STR "cmd"
#define RCODE_STR "rcode"
#define RESD_STR "rdata"
#define DPAVAL_STR "dpaval"
#define STATUS_STR "status"

//////////////////////////////////////////
PrfCommonJson::PrfCommonJson()
{
  m_doc.SetObject();
}

PrfCommonJson::PrfCommonJson(const PrfCommonJson& o)
{
  m_has_ctype = o.m_has_ctype;
  m_has_type = o.m_has_type;
  m_has_nadr = o.m_has_nadr;
  m_has_hwpid = o.m_has_hwpid;
  m_has_timeout = o.m_has_timeout;
  m_has_msgid = o.m_has_msgid;
  m_has_request = o.m_has_request;
  m_has_request_ts = o.m_has_request_ts;
  m_has_response = o.m_has_response;
  m_has_response_ts = o.m_has_response_ts;
  m_has_confirmation = o.m_has_confirmation;
  m_has_confirmation_ts = o.m_has_confirmation_ts;
  m_has_cmd = o.m_has_cmd;
  m_has_rcode = o.m_has_rcode;
  m_has_dpaval = o.m_has_dpaval;

  m_ctype = o.m_ctype;
  m_type = o.m_type;
  m_nadr = o.m_nadr;
  m_hwpid = o.m_hwpid;
  m_timeoutJ = o.m_timeoutJ;
  m_msgid = o.m_msgid;
  m_requestJ = o.m_requestJ;
  m_request_ts = o.m_request_ts;
  m_responseJ = o.m_responseJ;
  m_response_ts = o.m_response_ts;
  m_confirmationJ = o.m_confirmationJ;
  m_confirmation_ts = o.m_confirmation_ts;
  m_cmdJ = o.m_cmdJ;
  m_statusJ = o.m_statusJ;
  m_rcodeJ = o.m_rcodeJ;
  m_dpavalJ = o.m_dpavalJ;

  m_doc.SetObject();
}

int PrfCommonJson::parseBinary(uint8_t* to, const std::string& from, int maxlen)
{
  int retval = 0;
  if (!from.empty()) {
    std::string buf = from;
    if (std::string::npos != buf.find_first_of('.')) {
      std::replace(buf.begin(), buf.end(), '.', ' ');
      m_dotNotation = true;
    }
    std::istringstream istr(buf);

    int val;
    while (retval < maxlen) {
      if (!(istr >> std::hex >> val)) {
        if (istr.eof()) break;
        THROW_EX(std::logic_error, "Unexpected format: " << PAR(from));
      }
      to[retval++] = (uint8_t)val;
    }
  }
  return retval;
}

void PrfCommonJson::encodeHexaNum(std::string& to, uint8_t from)
{
  std::ostringstream os;
  os.fill('0'); os.width(2);
  os << std::hex << (int)from;
  to = os.str();
}

void PrfCommonJson::encodeHexaNum(std::string& to, uint16_t from)
{
  std::ostringstream os;
  os.fill('0'); os.width(4);
  os << std::hex << (int)from;
  to = os.str();
}

void PrfCommonJson::encodeBinary(std::string& to, const uint8_t* from, int len)
{
  bool dot = std::string::npos != to.find_first_of('.');
  to.clear();
  if (len > 0) {
    std::ostringstream ostr;
    ostr << iqrf::TracerHexString(from, len, true);

    if (m_dotNotation || dot) {
      to = ostr.str();
      std::replace(to.begin(), to.end(), ' ', '.');
      if (to[to.size() - 1] == '.')
        to.pop_back();
    }
    else {
      to = ostr.str();
      if (to[to.size() - 1] == ' ')
        to.pop_back();
    }
  }
}

void PrfCommonJson::encodeTimestamp(std::string& to, std::chrono::time_point<std::chrono::system_clock> from)
{
  using namespace std::chrono;

  to.clear();
  if (from.time_since_epoch() != system_clock::duration()) {
    auto fromUs = std::chrono::duration_cast<std::chrono::microseconds>(from.time_since_epoch()).count() % 1000000;
    auto time = std::chrono::system_clock::to_time_t(from);
    //auto tm = *std::gmtime(&time);
    auto tm = *std::localtime(&time);

    char buf[80];
    strftime(buf, sizeof(buf), "%FT%T", &tm);

    std::ostringstream os;
    os.fill('0'); os.width(6);
    //os << std::put_time(&tm, "%F %T.") <<  fromUs; // << std::put_time(&tm, " %Z\n");
    os << buf << "." << fromUs;

    to = os.str();
  }
}

void PrfCommonJson::parseRequestJson(const rapidjson::Value& val, DpaTask& dpaTask)
{
  jutils::assertIsObject("", val);

  m_has_ctype = jutils::getMemberIfExistsAs<std::string>(CTYPE_STR, val, m_ctype);
  m_has_type = jutils::getMemberIfExistsAs<std::string>(TYPE_STR, val, m_type);
  m_has_nadr = jutils::getMemberIfExistsAs<std::string>(NADR_STR, val, m_nadr);
  m_has_hwpid = jutils::getMemberIfExistsAs<std::string>(HWPID_STR, val, m_hwpid);
  m_has_timeout = jutils::getMemberIfExistsAs<int>(TIMEOUT_STR, val, m_timeoutJ);
  m_has_msgid = jutils::getMemberIfExistsAs<std::string>(MSGID_STR, val, m_msgid);
  m_has_request = jutils::getMemberIfExistsAs<std::string>(REQUEST_STR, val, m_requestJ);
  m_has_request_ts = jutils::getMemberIfExistsAs<std::string>(REQUEST_TS_STR, val, m_request_ts);
  m_has_response = jutils::getMemberIfExistsAs<std::string>(RESPONSE_STR, val, m_responseJ);
  m_has_response_ts = jutils::getMemberIfExistsAs<std::string>(RESPONSE_TS_STR, val, m_response_ts);
  m_has_confirmation = jutils::getMemberIfExistsAs<std::string>(CONFIRMATION_STR, val, m_confirmationJ);
  m_has_confirmation_ts = jutils::getMemberIfExistsAs<std::string>(CONFIRMATION_TS_STR, val, m_confirmation_ts);
  m_has_cmd = jutils::getMemberIfExistsAs<std::string>(CMD_STR, val, m_cmdJ);
  m_has_rcode = jutils::getMemberIfExistsAs<std::string>(RCODE_STR, val, m_rcodeJ);
  m_has_dpaval = jutils::getMemberIfExistsAs<std::string>(DPAVAL_STR, val, m_dpavalJ);

  if (m_has_nadr) {
    uint16_t nadr;
    parseHexaNum(nadr, m_nadr);
    dpaTask.setAddress(nadr);
  }
  if (m_has_hwpid) {
    uint16_t hwpid;
    parseHexaNum(hwpid, m_hwpid);
    dpaTask.setHwpid(hwpid);
  }
  if (m_has_cmd) {
    dpaTask.parseCommand(m_cmdJ);
  }
  if (m_has_timeout && m_timeoutJ >= 0) {
    dpaTask.setTimeout(m_timeoutJ);
  }
}

void PrfCommonJson::addResponseJsonPrio1Params(const DpaTask& dpaTask)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  bool responded = true;
  if (0 >= dpaTask.getResponse().GetLength())
    responded = false;

  if (m_has_ctype) {
    v.SetString(m_ctype.c_str(), alloc);
    m_doc.AddMember(CTYPE_STR, v, alloc);
  }
  if (m_has_type) {
    v.SetString(m_type.c_str(), alloc);
    m_doc.AddMember(TYPE_STR, v, alloc);
  }
  if (m_has_msgid) {
    v.SetString(m_msgid.c_str(), alloc);
    m_doc.AddMember(MSGID_STR, v, alloc);
  }
  if (m_has_timeout) {
    v = m_timeoutJ;
    m_doc.AddMember(TIMEOUT_STR, v, alloc);
  }
  if (m_has_nadr) {
    if (!responded)
      m_nadr.clear();
    v.SetString(m_nadr.c_str(), alloc);
    m_doc.AddMember(NADR_STR, v, alloc);
  }
}

void PrfCommonJson::addResponseJsonPrio2Params(const DpaTask& dpaTask)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  bool responded = true;
  if (0 >= dpaTask.getResponse().GetLength())
    responded = false;

  if (m_has_cmd) {
    if (!responded)
      m_cmdJ.clear();
    v.SetString(m_cmdJ.c_str(), alloc);
    m_doc.AddMember(CMD_STR, v, alloc);
  }
  if (m_has_hwpid) {
    if (!responded)
      m_hwpid.clear();
    v.SetString(m_hwpid.c_str(), alloc);
    m_doc.AddMember(HWPID_STR, v, alloc);
  }
}

std::string PrfCommonJson::encodeResponseJsonFinal(const DpaTask& dpaTask)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  bool responded = true;
  if (0 >= dpaTask.getResponse().GetLength())
    responded = false;

  if (m_has_rcode) {
    if (responded)
      encodeHexaNum(m_rcodeJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode);
    else
      m_rcodeJ.clear();
    v.SetString(m_rcodeJ.c_str(), alloc);
    m_doc.AddMember(RCODE_STR, v, alloc);
  }

  if (m_has_dpaval) {
    if (responded)
      encodeHexaNum(m_dpavalJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue);
    else
      m_dpavalJ.clear();
    v.SetString(m_dpavalJ.c_str(), alloc);
    m_doc.AddMember(DPAVAL_STR, v, alloc);
  }

  if (m_has_rdata) {
    if (responded) {
      int datalen = dpaTask.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2; //DpaValue ResponseCode
      if (datalen > 0) {
        encodeBinary(m_rdataJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, datalen);
      }
    }
    else
      m_rdataJ.clear();
    v.SetString(m_rdataJ.c_str(), alloc);
    m_doc.AddMember(RESD_STR, v, alloc);
  }

  if (m_has_request) {
    encodeBinary(m_requestJ, dpaTask.getRequest().DpaPacket().Buffer, dpaTask.getRequest().GetLength());
    v.SetString(m_requestJ.c_str(), alloc);
    m_doc.AddMember(REQUEST_STR, v, alloc);
  }
  if (m_has_request_ts) {
    encodeTimestamp(m_request_ts, dpaTask.getRequestTs());
    v.SetString(m_request_ts.c_str(), alloc);
    m_doc.AddMember(REQUEST_TS_STR, v, alloc);
  }
  if (m_has_confirmation) {
    encodeBinary(m_confirmationJ, dpaTask.getConfirmation().DpaPacket().Buffer, dpaTask.getConfirmation().GetLength());
    v.SetString(m_confirmationJ.c_str(), alloc);
    m_doc.AddMember(CONFIRMATION_STR, v, alloc);
  }
  if (m_has_confirmation_ts) {
    encodeTimestamp(m_confirmation_ts, dpaTask.getConfirmationTs());
    v.SetString(m_confirmation_ts.c_str(), alloc);
    m_doc.AddMember(CONFIRMATION_TS_STR, v, alloc);
  }
  if (m_has_response) {
    encodeBinary(m_responseJ, dpaTask.getResponse().DpaPacket().Buffer, dpaTask.getResponse().GetLength());
    v.SetString(m_responseJ.c_str(), alloc);
    m_doc.AddMember(RESPONSE_STR, v, alloc);
  }
  if (m_has_response_ts) {
    encodeTimestamp(m_response_ts, dpaTask.getResponseTs());
    v.SetString(m_response_ts.c_str(), alloc);
    m_doc.AddMember(RESPONSE_TS_STR, v, alloc);
  }

  v.SetString(m_statusJ.c_str(), alloc);
  m_doc.AddMember(STATUS_STR, v, alloc);

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  m_doc.Accept(writer);
  return buffer.GetString();
}

/////////////////////////////////////////
//-------------------------------
PrfRawJson::PrfRawJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);

  if (!m_has_request) {
    THROW_EX(std::logic_error, "Missing member: " << REQUEST_STR);
  }

  int len = parseBinary(m_request.DpaPacket().Buffer, m_requestJ, MAX_DPA_BUFFER);
  m_request.SetLength(len);
}

std::string PrfRawJson::encodeResponse(const std::string& errStr)
{
  if (m_dotNotation) {
    m_responseJ = ".";
  }
  m_has_response = true; //mandatory here
  m_statusJ = errStr;
  
  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);
  return encodeResponseJsonFinal(*this);
}

std::string PrfRawJson::encodeAsyncRequest(const std::string& errStr)
{
  if (m_dotNotation) {
    m_responseJ = ".";
  }
  m_has_response = false; //unwanted here
  m_statusJ = errStr;

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);
  return encodeResponseJsonFinal(*this);
}

//just for Async
PrfRawJson::PrfRawJson(const DpaMessage& dpaMessage)
{
  m_ctype = CAT_DPA_STR;
  m_type = DpaRaw::PRF_NAME;
  switch (dpaMessage.MessageDirection()) {
  case DpaMessage::MessageType::kRequest:
    setRequest(dpaMessage);
    m_has_request = true;
    m_has_response = true;
    m_has_request_ts = true;
    timestampRequest();
    break;
  case DpaMessage::MessageType::kResponse:
    parseResponse(dpaMessage);
    m_has_request = true;
    m_has_response = true;
    m_has_response_ts = true;
    timestampResponse();
    break;
  default:;
  }
  m_has_ctype = true;
  m_has_type = true;
}

//-------------------------------
const std::string PrfRawHdpJson::PRF_NAME("raw-hdp");

#define PNUM_STR "pnum"
#define PCMD_STR "pcmd"
#define REQD_STR "rdata"

PrfRawHdpJson::PrfRawHdpJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);

  //mandatory here
  m_pnum = jutils::getMemberAs<std::string>(PNUM_STR, val);
  m_pcmd = jutils::getMemberAs<std::string>(PCMD_STR, val);
  m_hwpid = jutils::getMemberAs<std::string>(HWPID_STR, val);

  m_data = jutils::getPossibleMemberAs<std::string>(REQD_STR, val, m_data);

  {
    uint8_t pnum;
    parseHexaNum(pnum, m_pnum);
    m_request.DpaPacket().DpaRequestPacket_t.PNUM = pnum;
  }

  {
    uint8_t pcmd;
    parseHexaNum(pcmd, m_pcmd);
    m_request.DpaPacket().DpaRequestPacket_t.PCMD = pcmd;
  }

  {
    uint16_t hwpid;
    parseHexaNum(hwpid, m_hwpid);
    m_request.DpaPacket().DpaRequestPacket_t.HWPID = hwpid;
  }

  int len = parseBinary(m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, m_data, DPA_MAX_DATA_LENGTH);
  m_request.SetLength(sizeof(TDpaIFaceHeader) + len);

}

std::string PrfRawHdpJson::encodeResponse(const std::string& errStr)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  addResponseJsonPrio1Params(*this);

  bool responded = true;
  if (0 >= getResponse().GetLength())
    responded = false;

  uint8_t pnum = getResponse().DpaPacket().DpaResponsePacket_t.PNUM;
  uint8_t pcmd = getResponse().DpaPacket().DpaResponsePacket_t.PCMD;
  uint16_t hwpid = getResponse().DpaPacket().DpaResponsePacket_t.HWPID;
  m_has_hwpid = true;

  if (responded) {
    encodeHexaNum(m_pnum, pnum);
    encodeHexaNum(m_pcmd, pcmd);
    encodeHexaNum(m_hwpid, hwpid);
  }
  else {
    m_pnum.clear();
    m_pcmd.clear();
    m_hwpid.clear();
  }

  v.SetString(m_pnum.c_str(), alloc);
  m_doc.AddMember(PNUM_STR, v, alloc);
  
  v.SetString(m_pcmd.c_str(), alloc);
  m_doc.AddMember(PCMD_STR, v, alloc);

  //mandatory here
  m_has_rcode = true;
  m_has_rdata = true;
  m_has_dpaval = true;

  m_statusJ = errStr;
  
  addResponseJsonPrio2Params(*this);
  return encodeResponseJsonFinal(*this);
}

//-------------------------------
PrfThermometerJson::PrfThermometerJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);
}

std::string PrfThermometerJson::encodeResponse(const std::string& errStr)
{
  rapidjson::Value v;

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);

  v = getFloatTemperature();
  m_doc.AddMember("temperature", v, m_doc.GetAllocator());

  m_statusJ = errStr;
  return encodeResponseJsonFinal(*this);
}

//-------------------------------
#define FRC_CMD_STR "frc_cmd"
#define FRC_TYPE_STR "frc_type"
#define FRC_USER_STR "frc_user"
#define FRC_USER_DATA_STR "user_data"
#define FRC_DATA_STR "frc_data"

PrfFrcJson::PrfFrcJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);

  std::string frcCmd = jutils::getPossibleMemberAs<std::string>(FRC_CMD_STR, val, "");
  if (!frcCmd.empty()) {
    setFrcCommand(parseFrcCmd(frcCmd));
    m_predefinedFrcCommand = true;
  }
  else {
    std::string frcType = jutils::getMemberAs<std::string>(FRC_TYPE_STR, val);
    uint8_t frcUser = (uint8_t)jutils::getMemberAs<int>(FRC_USER_STR, val);
    setFrcCommand(parseFrcType(frcType), frcUser);
  }

  m_userData = jutils::getPossibleMemberAs<std::string>(FRC_USER_DATA_STR, val, m_userData);
  if (!m_userData.empty()) {
    const int udatalen = 30;
    uint8_t buf[udatalen];
    int len = parseBinary(buf, m_userData, udatalen);
    setUserData(PrfFrc::UserData(buf, len));
  }

}

std::string PrfFrcJson::encodeResponse(const std::string& errStr)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);

  if (m_predefinedFrcCommand) {
    v.SetString(PrfFrc::encodeFrcCmd((FrcCmd)getFrcCommand()).c_str(), alloc);
    m_doc.AddMember(FRC_CMD_STR, v, alloc);
  }
  else {
    v.SetString(PrfFrc::encodeFrcType((FrcType)getFrcType()).c_str(), alloc);
    m_doc.AddMember(FRC_TYPE_STR, v, alloc);

    v = (int)getFrcUser();
    m_doc.AddMember(FRC_USER_STR, v, alloc);
  }

  std::ostringstream os;
  os.setf(std::ios::hex, std::ios::basefield);
  os.fill('0');
  char separator = m_dotNotation ? '.' : ' ';

  switch (getFrcType()) {
  case FrcType::GET_BIT2:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BIT2; i++) {
      os << std::setw(2) << (int)getFrcData_bit2(i) << separator;
    }
  }
  break;

  case FrcType::GET_BYTE:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BYTE; i++) {
      os << std::setw(2) << (int)getFrcData_Byte(i) << separator;
    }
  }
  break;

  case FrcType::GET_BYTE2:
  {
    for (int i = 1; i <= PrfFrc::FRC_MAX_NODE_BYTE2; i++) {
      os << std::setw(4) << (int)getFrcData_Byte2(i) << separator;
    }
  }
  break;
  default:
    ;
  }

  std::string values(os.str());
  values.pop_back();

  v.SetString(values.c_str(), alloc);
  m_doc.AddMember(FRC_DATA_STR, v, alloc);

  m_statusJ = errStr;
  return encodeResponseJsonFinal(*this);
}

//-------------------------------
#define PORT_STR "port"
#define BIT_STR "bit"
#define DIR_STR "inp"
#define VAL_STR "val"

PrfIoJson::PrfIoJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);

  switch (getCmd()) {

  case PrfIo::Cmd::DIRECTION:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>(PORT_STR, val));
    int bit = jutils::getMemberAs<int>(BIT_STR, val);
    bool inp = jutils::getMemberAs<bool>(DIR_STR, val);
    directionCommand(port, bit, inp);
    m_port = port;
    m_bit = bit;
    m_val = inp;
  }
  break;

  case PrfIo::Cmd::SET:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>(PORT_STR, val));
    int bit = jutils::getMemberAs<int>(BIT_STR, val);
    bool value = jutils::getMemberAs<bool>(VAL_STR, val);
    setCommand(port, bit, value);
    m_port = port;
    m_bit = bit;
    m_val = value;
  }
  break;

  case PrfIo::Cmd::GET:
  {
    Port port = parsePort(jutils::getMemberAs<std::string>(PORT_STR, val));
    int bit = jutils::getMemberAs<int>(BIT_STR, val);
    m_port = port;
    m_bit = bit;
    getCommand();
  }
  break;

  default:
    ;
  }
}

std::string PrfIoJson::encodeResponse(const std::string& errStr)
{

  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);

  switch (getCmd()) {

  case PrfIo::Cmd::DIRECTION:
  {
    v.SetString(encodePort(m_port).c_str(), alloc);
    m_doc.AddMember(PORT_STR, v, alloc);

    v = m_bit;
    m_doc.AddMember(BIT_STR, v, alloc);

    v = m_val;
    m_doc.AddMember(DIR_STR, v, alloc);
  }
  break;

  case PrfIo::Cmd::SET:
  {
    v.SetString(encodePort(m_port).c_str(), alloc);
    m_doc.AddMember(PORT_STR, v, alloc);

    v = m_bit;
    m_doc.AddMember(BIT_STR, v, alloc);

    v = m_val;
    m_doc.AddMember(VAL_STR, v, alloc);
  }
  break;

  case PrfIo::Cmd::GET:
  {
    v.SetString(encodePort(m_port).c_str(), alloc);
    m_doc.AddMember(PORT_STR, v, alloc);

    v = m_bit;
    m_doc.AddMember(BIT_STR, v, alloc);

    v = getInput(m_port, m_bit);
    m_doc.AddMember(VAL_STR, v, alloc);
  }
  break;

  default:
    ;
  }

  m_statusJ = errStr;
  return encodeResponseJsonFinal(*this);
}

//////////////////
//-------------------------------
PrfOsJson::PrfOsJson(const rapidjson::Value& val)
{
  parseRequestJson(val, *this);

  switch (getCmd()) {

  case Cmd::READ:
  {
  }
  break;

  default:
    ;
  }
}

std::string PrfOsJson::encodeResponse(const std::string& errStr)
{
  Document::AllocatorType& alloc = m_doc.GetAllocator();
  rapidjson::Value v;

  addResponseJsonPrio1Params(*this);
  addResponseJsonPrio2Params(*this);

  switch (getCmd()) {

  case Cmd::READ:
  {
  }
  break;

  default:
    ;
  }

  m_statusJ = errStr;
  return encodeResponseJsonFinal(*this);
}

///////////////////////////////////////////
JsonSerializer::JsonSerializer()
  :m_name("Json")
{
  init();
}

JsonSerializer::JsonSerializer(const std::string& name)
  : m_name(name)
{
  init();
}

void JsonSerializer::init()
{
  registerClass<PrfRawJson>(DpaRaw::PRF_NAME);
  registerClass<PrfRawHdpJson>(PrfRawHdpJson::PRF_NAME);
  registerClass<PrfThermometerJson>(PrfThermometer::PRF_NAME);
  registerClass<PrfLedGJson>(PrfLedG::PRF_NAME);
  registerClass<PrfLedRJson>(PrfLedR::PRF_NAME);
  registerClass<PrfFrcJson>(PrfFrc::PRF_NAME);
  registerClass<PrfIoJson>(PrfIo::PRF_NAME);
  registerClass<PrfOsJson>(PrfOs::PRF_NAME);
}

std::string JsonSerializer::parseCategory(const std::string& request)
{
  std::string ctype;
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);
    ctype = jutils::getMemberAs<std::string>("ctype", doc);
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return ctype;
}

std::unique_ptr<DpaTask> JsonSerializer::parseRequest(const std::string& request)
{
  std::unique_ptr<DpaTask> obj;
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);
    std::string perif = jutils::getMemberAs<std::string>("type", doc);

    obj = createObject(perif, doc);
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return std::move(obj);
}

std::string JsonSerializer::parseConfig(const std::string& request)
{
  std::string cmd;
  try {
    Document doc;
    jutils::parseString(request, doc);

    jutils::assertIsObject("", doc);

    std::string type = jutils::getMemberAs<std::string>("type", doc);

    if (type == "mode") {
      cmd = jutils::getMemberAs<std::string>("cmd", doc);
    }
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return cmd;
}

std::string JsonSerializer::encodeConfig(const std::string& request, const std::string& response)
{
  std::string res;
  try {
    Document doc;

    jutils::parseString(request, doc);
    jutils::assertIsObject("", doc);

    Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value v;
    v.SetString(response.c_str(), alloc);
    doc.AddMember("status", v, alloc);
  
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    res = buffer.GetString();
  }
  catch (std::exception &e) {
    m_lastError = e.what();
  }
  return res;
}

std::string JsonSerializer::getLastError() const
{
  return m_lastError;
}

std::string JsonSerializer::encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const
{
  PrfRawJson raw(dpaMessage);
  raw.m_dotNotation = true;
  std::string status;
  switch (dpaMessage.MessageDirection()) {
  case DpaMessage::MessageType::kRequest:
    raw.m_has_request = true;
    raw.m_has_response = false;
    status = "ASYNC_REQUEST";
    return raw.encodeAsyncRequest(status);
  case DpaMessage::MessageType::kResponse:
    raw.m_has_request = false;
    raw.m_has_response = true;
    status = "ASYNC_RESPONSE";
    return raw.encodeResponse(status);
  default:
    status = "ASYNC_MESSAGE";
  }
  return raw.encodeResponse(status);
}
