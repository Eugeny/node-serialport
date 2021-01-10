#include "node.h"
#include "./serialport.h"

#ifdef __APPLE__
  #include "./darwin_list.h"
#endif

#ifdef WIN32
  #define strncasecmp strnicmp
  #include "./serialport_win.h"
#else
  #include "./poller.h"
#endif

Napi::Value getValueFromObject(Napi::Object options, std::string key) {
  Napi::String v8str = Napi::String::New(options.Env(), key);
  return (options).Get(v8str);
}

int getIntFromObject(Napi::Object options, std::string key) {
  return getValueFromObject(options, key).As<Napi::Number>().Int32Value();
}

bool getBoolFromObject(Napi::Object options, std::string key) {
  return getValueFromObject(options, key).As<Napi::Boolean>().Value();
}

Napi::String getStringFromObj(Napi::Object options, std::string key) {
  return getValueFromObject(options, key).As<Napi::String>();
}

double getDoubleFromObject(Napi::Object options, std::string key) {
  return getValueFromObject(options, key).As<Napi::Number>().DoubleValue();
}

Napi::Value Open(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // path
  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string path = info[0].As<Napi::String>();

  // options
  if (!info[1].IsObject()) {
    Napi::TypeError::New(env, "Second argument must be an object").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Object options = info[1].As<Napi::Object>();

  // callback
  if (!info[2].IsFunction()) {
    Napi::TypeError::New(env, "Third argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  OpenBaton* baton = new OpenBaton {
    .env = env,
    .baudRate = getIntFromObject(options, "baudRate"),
    .dataBits = getIntFromObject(options, "dataBits"),
    .parity = ToParityEnum(env, getStringFromObj(options, "parity")),
    .stopBits = ToStopBitEnum(getDoubleFromObject(options, "stopBits")),
    .rtscts = getBoolFromObject(options, "rtscts"),
    .xon = getBoolFromObject(options, "xon"),
    .xoff = getBoolFromObject(options, "xoff"),
    .xany = getBoolFromObject(options, "xany"),
    .hupcl = getBoolFromObject(options, "hupcl"),
    .lock = getBoolFromObject(options, "lock"),
  };
  snprintf(baton->path, sizeof(baton->path), "%s", path.c_str());
  baton->callback.Reset(info[2].As<Napi::Function>());

  #ifndef WIN32
    baton->vmin = getIntFromObject(options, "vmin");
    baton->vtime = getIntFromObject(options, "vtime");
  #endif

  uv_work_t* req = new uv_work_t();
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, EIO_Open, (uv_after_work_cb)EIO_AfterOpen);
  return env.Undefined();
}

void EIO_AfterOpen(uv_work_t* req) {
  OpenBaton* data = static_cast<OpenBaton*>(req->data);
  auto env = data->env;

  Napi::Value argv[2];
  if (data->errorString[0]) {
    auto err = Napi::Error::New(env, data->errorString).Value();
    data->callback.Call({ err, env.Undefined() });
  } else {
    data->callback.Call({ env.Null(), Napi::Number::New(env, data->result) });
  }

  delete data;
  delete req;
}

Napi::Value Update(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // options
  if (!info[1].IsObject()) {
    Napi::TypeError::New(env, "Second argument must be an object").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Object options = info[1].As<Napi::Object>();

  if (!options.Has(Napi::String::New(env, "baudRate"))) {
    Napi::TypeError::New(env, "\"baudRate\" must be set on options object").ThrowAsJavaScriptException();
    return env.Null();
  }

  // callback
  if (!info[2].IsFunction()) {
    Napi::TypeError::New(env, "Third argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  ConnectionOptionsBaton* baton = new ConnectionOptionsBaton(env);
  baton->baudRate = getIntFromObject(options, "baudRate");
  baton->fd = fd;
  baton->callback.Reset(info[2].As<Napi::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, EIO_Update, (uv_after_work_cb)EIO_AfterUpdate);
  return env.Undefined();
}

void EIO_AfterUpdate(uv_work_t* req) {
  ConnectionOptionsBaton* data = static_cast<ConnectionOptionsBaton*>(req->data);
  auto env = data->env;

  if (data->errorString[0]) {
    data->callback.Call({ Napi::Error::New(env, data->errorString).Value() });
  } else {
    data->callback.Call({ env.Null() });
  }

  delete data;
  delete req;
}

Napi::Value Close(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }

  // callback
  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  VoidBaton* baton = new VoidBaton {
    .env = env,
    .fd = info[0].As<Napi::Number>().Int32Value()
  };
  baton->callback.Reset(info[1].As<Napi::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Close, (uv_after_work_cb)EIO_AfterClose);
  return env.Undefined();
}

void EIO_AfterClose(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);

  if (data->errorString[0]) {
    data->callback.Call({ Napi::Error::New(data->env, data->errorString).Value() });
  } else {
    data->callback.Call({ data->env.Null() });
  }

  delete data;
  delete req;
}

Napi::Value Flush(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // callback
  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Function callback = info[1].As<Napi::Function>();

  VoidBaton* baton = new VoidBaton {
    .env = env,
    .fd = fd
  };
  baton->callback.Reset(callback);

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Flush, (uv_after_work_cb)EIO_AfterFlush);
  return env.Undefined();
}

void EIO_AfterFlush(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);

  if (data->errorString[0]) {
    data->callback.Call({ Napi::Error::New(data->env, data->errorString).Value() });
  } else {
    data->callback.Call({ data->env.Null() });
  }

  delete data;
  delete req;
}

Napi::Value Set(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // options
  if (!info[1].IsObject()) {
    Napi::TypeError::New(env, "Second argument must be an object").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Object options = info[1].As<Napi::Object>();

  // callback
  if (!info[2].IsFunction()) {
    Napi::TypeError::New(env, "Third argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Function callback = info[2].As<Napi::Function>();

  SetBaton* baton = new SetBaton {
    .env = env,
    .fd = fd,
    .brk = getBoolFromObject(options, "brk"),
    .rts = getBoolFromObject(options, "rts"),
    .cts = getBoolFromObject(options, "cts"),
    .dtr = getBoolFromObject(options, "dtr"),
    .dsr = getBoolFromObject(options, "dsr")
  };
  baton->callback.Reset(callback);

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Set, (uv_after_work_cb)EIO_AfterSet);
  return env.Undefined();
}

void EIO_AfterSet(uv_work_t* req) {
  SetBaton* data = static_cast<SetBaton*>(req->data);

  if (data->errorString[0]) {
    data->callback.Call({ Napi::Error::New(data->env, data->errorString).Value() });
  } else {
    data->callback.Call({ data->env.Null() });
  }

  delete data;
  delete req;
}

Napi::Value Get(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // callback
  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  GetBaton* baton = new GetBaton {
    .env = env,
    .fd = fd,
    .cts = false,
    .dsr = false,
    .dcd = false
  };
  baton->callback.Reset(info[1].As<Napi::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Get, (uv_after_work_cb)EIO_AfterGet);
  return env.Undefined();
}

void EIO_AfterGet(uv_work_t* req) {
  GetBaton* data = static_cast<GetBaton*>(req->data);
  auto env = data->env;

  if (data->errorString[0]) {
    data->callback.Call({
      Napi::Error::New(env, data->errorString).Value(),
      env.Undefined()
    });
  } else {
    Napi::Object results = Napi::Object::New(env);
    (results).Set(Napi::String::New(env, "cts"), Napi::Boolean::New(env, data->cts));
    (results).Set(Napi::String::New(env, "dsr"), Napi::Boolean::New(env, data->dsr));
    (results).Set(Napi::String::New(env, "dcd"), Napi::Boolean::New(env, data->dcd));

    data->callback.Call({ env.Null(), results });
  }

  delete data;
  delete req;
}

Napi::Value GetBaudRate(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // callback
  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  GetBaudRateBaton* baton = new GetBaudRateBaton {
    .env = env,
    .fd = fd,
    .baudRate = 0
  };
  baton->callback.Reset(info[1].As<Napi::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_GetBaudRate, (uv_after_work_cb)EIO_AfterGetBaudRate);
  return env.Undefined();
}

void EIO_AfterGetBaudRate(uv_work_t* req) {
  GetBaudRateBaton* data = static_cast<GetBaudRateBaton*>(req->data);
  auto env = data->env;

  if (data->errorString[0]) {
    data->callback.Call({
      Napi::Error::New(env, data->errorString).Value(),
      env.Undefined()
    });
  } else {
    Napi::Object results = Napi::Object::New(env);
    (results).Set(Napi::String::New(env, "baudRate"), Napi::Number::New(env, data->baudRate));

    data->callback.Call({ env.Null(), results });
  }

  delete data;
  delete req;
}

Napi::Value Drain(const Napi::CallbackInfo& info) {
  auto env = info.Env();

  // file descriptor
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be an int").ThrowAsJavaScriptException();
    return env.Null();
  }
  int fd = info[0].As<Napi::Number>().Int32Value();

  // callback
  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
    return env.Null();
  }

  VoidBaton* baton = new VoidBaton {
    .env = env,
    .fd = fd,
  };
  baton->callback.Reset(info[1].As<Napi::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Drain, (uv_after_work_cb)EIO_AfterDrain);
  return env.Undefined();
}

void EIO_AfterDrain(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);
  auto env = data->env;

  if (data->errorString[0]) {
    data->callback.Call({ Napi::Error::New(data->env, data->errorString).Value() });
  } else {
    data->callback.Call({ env.Null() });
  }

  delete data;
  delete req;
}

SerialPortParity inline(ToParityEnum(const Napi::Env& env, const Napi::String& v8str)) {
  auto str = std::string(v8str);
  size_t count = str.size();
  SerialPortParity parity = SERIALPORT_PARITY_NONE;
  if (!strncasecmp(str.c_str(), "none", count)) {
    parity = SERIALPORT_PARITY_NONE;
  } else if (!strncasecmp(str.c_str(), "even", count)) {
    parity = SERIALPORT_PARITY_EVEN;
  } else if (!strncasecmp(str.c_str(), "mark", count)) {
    parity = SERIALPORT_PARITY_MARK;
  } else if (!strncasecmp(str.c_str(), "odd", count)) {
    parity = SERIALPORT_PARITY_ODD;
  } else if (!strncasecmp(str.c_str(), "space", count)) {
    parity = SERIALPORT_PARITY_SPACE;
  }
  return parity;
}

SerialPortStopBits inline(ToStopBitEnum(double stopBits)) {
  if (stopBits > 1.4 && stopBits < 1.6) {
    return SERIALPORT_STOPBITS_ONE_FIVE;
  }
  if (stopBits == 2) {
    return SERIALPORT_STOPBITS_TWO;
  }
  return SERIALPORT_STOPBITS_ONE;
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "set"), Napi::Function::New(env, Set));
  exports.Set(Napi::String::New(env, "get"), Napi::Function::New(env, Get));
  exports.Set(Napi::String::New(env, "getBaudRate"), Napi::Function::New(env, GetBaudRate));
  exports.Set(Napi::String::New(env, "open"), Napi::Function::New(env, Open));
  exports.Set(Napi::String::New(env, "update"), Napi::Function::New(env, Update));
  exports.Set(Napi::String::New(env, "close"), Napi::Function::New(env, Close));
  exports.Set(Napi::String::New(env, "flush"), Napi::Function::New(env, Flush));
  exports.Set(Napi::String::New(env, "drain"), Napi::Function::New(env, Drain));

  #ifdef __APPLE__
  exports.Set(Napi::String::New(env, "list"), Napi::Function::New(env, List));
  #endif

  #ifdef WIN32
  exports.Set(Napi::String::New(env, "write"), Napi::Function::New(env, Write));
  exports.Set(Napi::String::New(env, "read"), Napi::Function::New(env, Read));
  exports.Set(Napi::String::New(env, "list"), Napi::Function::New(env, List));
  #else
  Poller::Init(env, exports);
  #endif
  return exports;
}

NODE_API_MODULE(serialport, init);
