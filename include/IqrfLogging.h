#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

//initialize tracing
//if stream == "" => tracing to std::cout else stream value is taken as the trace file name
//if the file exceeds the filesize, the file is reset and overwritten by next trace outputs

#define TRC_INIT_FILE_MAXSIZE(stream, filesize) \
namespace iqrf { \
  Tracer& Tracer::getTracer() { static Tracer tracer(stream, filesize); return tracer; }\
  static Tracer& iqrfTracer(Tracer::getTracer()); }

static const long TRC_DEFAULT_FILE_MAXSIZE(10 * 1024 * 1024);

#define TRC_INIT(stream) TRC_INIT_FILE_MAXSIZE(stream, TRC_DEFAULT_FILE_MAXSIZE)

//convenient trace macros
#ifdef _DEBUG
#define TRC_ERR(trc)  TRC(iqrf::Level::err, trc)
#define TRC_WAR(trc)  TRC(iqrf::Level::war, trc)
#define TRC_INF(trc)  TRC(iqrf::Level::inf, trc)
#define TRC_DBG(trc)  TRC(iqrf::Level::dbg, trc)
#define TRC_ENTER(trc) TRC(iqrf::Level::dbg, "{ENTER} " << trc)
#define TRC_LEAVE(trc) TRC(iqrf::Level::dbg, "{LEAVE} " << trc)
#else
#define TRC_ERR(trc)  TRC(iqrf::Level::err, trc)
#define TRC_WAR(trc)  TRC(iqrf::Level::war, trc)
#define TRC_INF(trc)  TRC(iqrf::Level::inf, trc)
#define TRC_DBG(trc)
#define TRC_ENTER(trc)
#define TRC_LEAVE(trc)
#endif

#define PAR(par)                #par "=\"" << par << "\" "
#define PAR_HEX(par)            #par "=\"0x" << std::hex << par << std::dec << "\" "
#define NAME_PAR(name, par)     #name "=\"" << par << "\" "
#define NAME_PAR_HEX(name,par)  #name "=\"0x" << std::hex << par << std::dec << "\" "

// Inserts binary data to trace in the form:
// 16:  34 35 36 36 35 34 36 35 34 36 35 35 34 36 35 34    4566546546554654
// 32:  36 35 34 36 35 34 36 35 34 36 35 34 36 35 34 36    6546546546546546
// 48:  35 34 36 35 34 36 35 34 36 35 34 36 35 34 32 37    5465465465465427 <I/>
// It is developer's responsibility to passed proper len of "owned" data. Uprintable characters are replaced by '.' 
#define FORM_HEX(ptr, len)      iqrf::TracerHexString((unsigned char*)ptr, len)

//exceptions tracing
#define THROW_EX(extype, exmsg) { \
  std::ostringstream ostrex; ostrex << exmsg; \
  TRC_WAR("Throwing " << #extype << ": " << ostrex.str()); \
  extype ex(ostrex.str().c_str()); throw ex; }

#define CATCH_EX(msg, extype, ex) { \
  TRC_WAR("Caught " << msg << ": " << #extype << ": " << ex.what()); }

//auxiliary macro
#ifdef _DEBUG
#define FLF iqrf::TracerNiceFileName(__FILE__) << " ln:" << \
__LINE__ << iqrf::TracerNiceFuncName(__FUNCTION__)
#else
#define FLF iqrf::TracerNiceFuncName(__FUNCTION__)
#endif

#define TRC(level, msg) { \
  std::ostringstream ostr; \
  ostr << levelToChar(level) << FLF << std::endl << msg << std::endl; \
  iqrf::Tracer::getTracer().write(ostr.str()); }

namespace iqrf {

  enum Level {
    err,
    war,
    inf,
    dbg
  };

  class TracerNiceFileName {
  public:
    TracerNiceFileName(const std::string& fname)
      :file_name(fname)
    {}

    friend std::ostream& operator << (std::ostream& o, const TracerNiceFileName& fn)
    {
      std::string niceName = fn.file_name;
      size_t found = niceName.find_last_of("/\\");
      if (std::string::npos != found)
        niceName = niceName.substr(found + 1);

      o << " \"" << niceName << "\"";
      return o;
    }
  private:
    std::string file_name;
  };

  //TODO check conversion of lambdas
  class TracerNiceFuncName {
  public:
    TracerNiceFuncName(const std::string& fname)
      :func_name(fname)
    {}

    friend std::ostream& operator << (std::ostream& o, const TracerNiceFuncName& fn)
    {
      std::string niceName = fn.func_name;
      size_t end = niceName.find('(');
      if (std::string::npos != end)
    	  niceName = niceName.substr(0, end);

      size_t beg = niceName.rfind(' ');

      if (std::string::npos != beg) {
        beg++;
        niceName = niceName.substr(beg, niceName.size() - 1);
      }

      o << " " << niceName << "()";
      return o;
    }
  private:
    std::string func_name;
  };

  static const char* levelToChar(Level level)
  {
    switch (level) {
    case err:
      return "{ERR}";
    case war:
      return "{WAR}";
    case inf:
      return "{INF}";
    case dbg:
      return "{DBG}";
    default:
      return "{???}";
    }
  }

  class Tracer {
  public:
    void write(const std::string& msg)
    {
      std::lock_guard<std::recursive_mutex> lck(m_mtx);

      if (m_cout)
        std::cout << msg;
      else {
        if (m_ofstream.is_open()) {
          m_ofstream << msg;
          if (m_ofstream.tellp() > m_maxSize)
          {
            resetFile();
          }
        }
      }
    }

    static Tracer& getTracer();

  private:
    Tracer(const std::string& fname, long maxSize)
      :m_fname(fname)
      ,m_cout(fname.empty())
      ,m_maxSize(maxSize)
    {
      if (!m_fname.empty())
        openFile();
    }

    void openFile()
    {
      static unsigned count = 0;
      if (!m_ofstream.is_open())
      {
        m_ofstream.open(m_fname, std::ofstream::out | std::ofstream::trunc);
        if (!m_ofstream.is_open())
          std::cerr << std::endl << "Cannot open: " << PAR(m_fname) << std::endl;
        m_ofstream << "file: " << count++ << " opened/reset" << std::endl;
      }
    }

    void closeFile()
    {
      m_ofstream.flush();
      m_ofstream.close();
    }

    void resetFile()
    {
      closeFile();
      openFile();
    }

    Tracer & operator = (const Tracer& t);
    Tracer(const Tracer& t);

    std::recursive_mutex m_mtx;
    std::string m_fname;
    std::ofstream m_ofstream;
    bool m_cout;
    long m_maxSize;
  };

  class TracerHexString
  {
  public:
    TracerHexString(const void* buf, const long len, bool plain = false) {
      os.setf(std::ios::hex, std::ios::basefield); os.fill('0');

      int i = 1;
      for (; i < len + 1; i++) {
        unsigned char c = 0xff & ((const unsigned char*)buf)[i - 1];
        //bin output
        os << std::setw(2) << (int)c << ' ';
        oschar.put(isgraph(c) ? (char)c : '.');
        if (!(i % 16)) {
          if (!plain) {
            //char output
            os << oschar.str();
            oschar.seekp(0);
            os << std::endl;
          }
          //os << std::endl;
        }
      }
      if (!plain) {
        //align last line
        i--;
        if (i % 16) {
          for (; i % 16; i++) {
            os << "   ";
            oschar << ' ';
          }
          os << oschar.str();
          os << std::endl;
        }
      }
    }

    virtual ~TracerHexString() {}

  private:
    TracerHexString();

    std::ostringstream os;
    std::ostringstream oschar;

    friend std::ostream& operator<<(std::ostream& out, const TracerHexString& hex) {
      out << hex.os.str();
      return out;
    }
  };

} //namespace iqrf
