#ifndef PACKAGES_SERIALPORT_SRC_POLLER_H_
#define PACKAGES_SERIALPORT_SRC_POLLER_H_

#include <napi.h>
#include <uv.h>

class Poller : public Napi::ObjectWrap<Poller> {
 public:
  Poller(const Napi::CallbackInfo& info);
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static void onData(uv_poll_t* handle, int status, int events);
  static void onClose(uv_handle_t* poll_handle);
  ~Poller();

 private:
  int fd;
  uv_poll_t* poll_handle;
  Napi::Env env;
  Napi::FunctionReference callback;
  bool uv_poll_init_success = false;

  // can this be read off of poll_handle?
  int events = 0;

  void poll(int events);
  void stop();
  int _stop();

  void poll(const Napi::CallbackInfo& info);
  void stop(const Napi::CallbackInfo& info);
  void destroy(const Napi::CallbackInfo& info);
};

#endif  // PACKAGES_SERIALPORT_SRC_POLLER_H_
