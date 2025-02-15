#ifndef PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#define PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <napi.h>
#include <uv.h>
#include <string>

#define ERROR_STRING_SIZE 1024

Napi::Value Open(const Napi::CallbackInfo& info);
void EIO_Open(uv_work_t* req);
void EIO_AfterOpen(uv_work_t* req);

Napi::Value Update(const Napi::CallbackInfo& info);
void EIO_Update(uv_work_t* req);
void EIO_AfterUpdate(uv_work_t* req);

Napi::Value Close(const Napi::CallbackInfo& info);
void EIO_Close(uv_work_t* req);
void EIO_AfterClose(uv_work_t* req);

Napi::Value Flush(const Napi::CallbackInfo& info);
void EIO_Flush(uv_work_t* req);
void EIO_AfterFlush(uv_work_t* req);

Napi::Value Set(const Napi::CallbackInfo& info);
void EIO_Set(uv_work_t* req);
void EIO_AfterSet(uv_work_t* req);

Napi::Value Get(const Napi::CallbackInfo& info);
void EIO_Get(uv_work_t* req);
void EIO_AfterGet(uv_work_t* req);

Napi::Value GetBaudRate(const Napi::CallbackInfo& info);
void EIO_GetBaudRate(uv_work_t* req);
void EIO_AfterGetBaudRate(uv_work_t* req);

Napi::Value Drain(const Napi::CallbackInfo& info);
void EIO_Drain(uv_work_t* req);
void EIO_AfterDrain(uv_work_t* req);

enum SerialPortParity {
  SERIALPORT_PARITY_NONE  = 1,
  SERIALPORT_PARITY_MARK  = 2,
  SERIALPORT_PARITY_EVEN  = 3,
  SERIALPORT_PARITY_ODD   = 4,
  SERIALPORT_PARITY_SPACE = 5
};

enum SerialPortStopBits {
  SERIALPORT_STOPBITS_ONE      = 1,
  SERIALPORT_STOPBITS_ONE_FIVE = 2,
  SERIALPORT_STOPBITS_TWO      = 3
};

SerialPortParity ToParityEnum(const Napi::Env& env, const Napi::String& str);
SerialPortStopBits ToStopBitEnum(double stopBits);

struct OpenBaton {
  char errorString[ERROR_STRING_SIZE];
  Napi::FunctionReference callback;
  Napi::Env env;
  char path[1024];
  int fd = 0;
  int result = 0;
  int baudRate = 0;
  int dataBits = 0;
  bool rtscts = false;
  bool xon = false;
  bool xoff = false;
  bool xany = false;
  bool dsrdtr = false;
  bool hupcl = false;
  bool lock = false;
  SerialPortParity parity;
  SerialPortStopBits stopBits;
#ifndef WIN32
  uint8_t vmin = 0;
  uint8_t vtime = 0;
#endif
};

struct ConnectionOptions {
  char errorString[ERROR_STRING_SIZE];
  int fd = 0;
  int baudRate = 0;
};

struct ConnectionOptionsBaton : ConnectionOptions {
  ConnectionOptionsBaton (Napi::Env &env): env(env) {};
  Napi::Env env;
  Napi::FunctionReference callback;
};

struct SetBaton {
  int fd = 0;
  Napi::Env env;
  Napi::FunctionReference callback;
  int result = 0;
  char errorString[ERROR_STRING_SIZE];
  bool rts = false;
  bool cts = false;
  bool dtr = false;
  bool dsr = false;
  bool brk = false;
};

struct GetBaton {
  int fd = 0;
  Napi::Env env;
  Napi::FunctionReference callback;
  char errorString[ERROR_STRING_SIZE];
  bool cts = false;
  bool dsr = false;
  bool dcd = false;
};

struct GetBaudRateBaton {
  int fd = 0;
  Napi::Env env;
  Napi::FunctionReference callback;
  char errorString[ERROR_STRING_SIZE];
  int baudRate = 0;
};

struct VoidBaton {
  int fd = 0;
  Napi::Env env;
  Napi::FunctionReference callback;
  char errorString[ERROR_STRING_SIZE];
};

int setup(int fd, OpenBaton *data);
int setBaudRate(ConnectionOptions *data);
#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
