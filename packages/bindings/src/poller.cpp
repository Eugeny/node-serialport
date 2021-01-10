#include <napi.h>
#include <uv.h>
#include "./poller.h"

Poller::Poller(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Poller>(info), env(info.Env()) {
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "fd must be an int").ThrowAsJavaScriptException();
    return;
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "cb must be a function").ThrowAsJavaScriptException();
    return;
  }

  this->fd = fd;
  this->callback.Reset(info[1].As<Napi::Function>());

  this->poll_handle = new uv_poll_t();
  memset(this->poll_handle, 0, sizeof(uv_poll_t));
  poll_handle->data = this;
  int status = uv_poll_init(uv_default_loop(), poll_handle, fd);
  if (0 != status) {
    Napi::Error::New(env, uv_strerror(status)).ThrowAsJavaScriptException();
    return;
  }
  uv_poll_init_success = true;
}

Poller::~Poller() {
  // if we call uv_poll_stop after uv_poll_init failed we segfault
  if (uv_poll_init_success) {
    uv_poll_stop(poll_handle);
    uv_close(reinterpret_cast<uv_handle_t*> (poll_handle), Poller::onClose);
  } else {
    delete poll_handle;
  }
}

void Poller::onClose(uv_handle_t* poll_handle) {
  // fprintf(stdout, "~Poller is closed\n");
  delete poll_handle;
}

int Poller::_stop() {
  return uv_poll_stop(poll_handle);
}

void Poller::onData(uv_poll_t* handle, int status, int events) {
  Poller* obj = static_cast<Poller*>(handle->data);
  auto env = obj->env;

  // if Error
  if (0 != status) {
    // fprintf(stdout, "OnData Error status=%s events=%d\n", uv_strerror(status), events);
    obj->_stop(); // doesn't matter if this errors
    obj->callback.Call({
      Napi::Error::New(env, uv_strerror(status)).Value(),
      env.Undefined()
    });
  } else {
    // fprintf(stdout, "OnData status=%d events=%d subscribed=%d\n", status, events, obj->events);
    // remove triggered events from the poll
    int newEvents = obj->events & ~events;
    obj->poll(newEvents);
    obj->callback.Call({ env.Null(), Napi::Number::New(env, events) });
  }
}

Napi::Object Poller::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Poller", {
    InstanceMethod("poll", &Poller::poll),
    InstanceMethod("stop", &Poller::stop),
    InstanceMethod("destroy", &Poller::destroy),
  });

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  exports.Set("Poller", func);
  env.SetInstanceData<Napi::FunctionReference>(constructor);
  return exports;
}

void Poller::poll(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "events must be an int").ThrowAsJavaScriptException();
  }
  int events = info[0].As<Napi::Number>().Int32Value();

  // Events can be UV_READABLE | UV_WRITABLE | UV_DISCONNECT
  // fprintf(stdout, "Poller:poll for %d\n", events);
  this->events = this->events | events;
  int status = uv_poll_start(poll_handle, events, Poller::onData);
  if (0 != status) {
    Napi::TypeError::New(env, uv_strerror(status)).ThrowAsJavaScriptException();
  }
}

void Poller::stop(const Napi::CallbackInfo& info) {
  int status = uv_poll_stop(poll_handle);
  if (0 != status) {
    Napi::TypeError::New(env, uv_strerror(status)).ThrowAsJavaScriptException();
  }
  this->stop();
}

void Poller::destroy(const Napi::CallbackInfo& info) {
  //this->persistent().Reset();
}
