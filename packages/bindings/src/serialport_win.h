#ifndef PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
#define PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include <napi.h>
#include <uv.h>
#include <list>
#include <string>

#define ERROR_STRING_SIZE 1024

struct WriteBaton : public Napi::AsyncResource {
  WriteBaton() : AsyncResource("node-serialport:WriteBaton"), bufferData(), errorString() {}
  int fd = 0;
  char* bufferData = nullptr;
  size_t bufferLength = 0;
  size_t offset = 0;
  size_t bytesWritten = 0;
  void* hThread = nullptr;
  bool complete = false;
  Napi::Persistent<v8::Object> buffer;
  Napi::FunctionReference callback;
  int result = 0;
  char errorString[ERROR_STRING_SIZE];
};

Napi::Value Write(const Napi::CallbackInfo& info);
void EIO_Write(uv_work_t* req);
void EIO_AfterWrite(uv_async_t* req);
DWORD __stdcall WriteThread(LPVOID param);


struct ReadBaton : public Napi::AsyncResource {
  ReadBaton() : AsyncResource("node-serialport:ReadBaton"), errorString() {}
  int fd = 0;
  char* bufferData = nullptr;
  size_t bufferLength = 0;
  size_t bytesRead = 0;
  size_t bytesToRead = 0;
  size_t offset = 0;
  void* hThread = nullptr;
  bool complete = false;
  char errorString[ERROR_STRING_SIZE];
  Napi::FunctionReference callback;
};

Napi::Value Read(const Napi::CallbackInfo& info);
void EIO_Read(uv_work_t* req);
void EIO_AfterRead(uv_async_t* req);
DWORD __stdcall ReadThread(LPVOID param);


Napi::Value List(const Napi::CallbackInfo& info);
void EIO_List(uv_work_t* req);
void EIO_AfterList(uv_work_t* req);

struct ListResultItem {
  std::string path;
  std::string manufacturer;
  std::string serialNumber;
  std::string pnpId;
  std::string locationId;
  std::string vendorId;
  std::string productId;
};

struct ListBaton {
  Napi::Env env;
  Napi::FunctionReference callback;
  std::list<ListResultItem*> results;
  char errorString[ERROR_STRING_SIZE] = "";
};

#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
