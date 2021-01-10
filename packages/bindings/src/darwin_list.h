#ifndef PACKAGES_SERIALPORT_SRC_DARWIN_LIST_H_
#define PACKAGES_SERIALPORT_SRC_DARWIN_LIST_H_
#include <sys/param.h>  // For MAXPATHLEN
#include <napi.h>
#include <uv.h>
#include <list>
#include <string>

#define ERROR_STRING_SIZE 1024

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
  char errorString[ERROR_STRING_SIZE];
};

typedef struct SerialDevice {
    char port[MAXPATHLEN];
    char locationId[MAXPATHLEN];
    char vendorId[MAXPATHLEN];
    char productId[MAXPATHLEN];
    char manufacturer[MAXPATHLEN];
    char serialNumber[MAXPATHLEN];
} stSerialDevice;

typedef struct DeviceListItem {
    struct SerialDevice value;
    struct DeviceListItem *next;
    int* length;
} stDeviceListItem;

#endif  // PACKAGES_SERIALPORT_SRC_DARWIN_LIST_H_
