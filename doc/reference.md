# REST in Curl - API Reference

*RESTinCurl* is a single, header-only C++ "library". In order to use it, you
just:

```C++
#include <restincurl/restincurl.h>
```

You can add it to you project as a git submodule, dowload it as part of your
projects build (from CMake or whatever magic you use), or you can just
download the header and copy it to your project (if you copy it, you should
pay attention to new releases in case there are any security fixes).

The library expose two simple C++ classes; `Client` and `RequestBuilder`. In most
cases you will only instantiate `Client` in your own code.

## The `Client` class

An instance of a client will, if asynchronous mode is enabled, create a
worker-thread when the first request is made. This single worker-thread
is then shared between all requests made for this client. When no requests
are active, the thread will wait for a while (see `RESTINCURL_IDLE_TIMEOUT_SEC`), keeping libcurls connection-cache warm, to serve new requests as efficiently as possible. When the idle time period expires, the thread will terminate and close the connection-cache associated with the client. If a new request is made later on, a new worker-thread will be created.


### Constructor

**`Client(bool init)`**

- **init** If true, the curl library is initialized. Set to false it libcurl
    is initialized somewhere else. The client will only initialize libcurl
    once, even if you call `Client(true)` multiple times.

### Methods

**`std::unique_ptr<RequestBuilder> Build()`**

Creates an instance of the `RequestBuilder` for use with this client.


**`void CloseWhenFinished()`**

Instructs the client to shut down the worker thread and release internal
resources as soon as all ongoing requests are served. If new requests are
added, they will also be served.

**`void Close()`**

Close the client immediately. Any ongoing requests will be aborted. Callbacks
for those requests are not called.

**`void WaitForFinish()`**

Puts the calling thread on hold until the worker-thread in the client is
stopped. Can be used from an applications main thread after `CloseWhenFinished()`
is called to keep the application alive until all requests are finished.

**`bool HaveWorker()`**

Returns true if the client have a worker-thread.

**`size_t GetNumActiveRequests()`**

Returns the current number of active requests. Queued requests are not
counted here.

## The `RequestBuilder` class

The RequestBuilder class is used to define a http(s) request. It can be
used to send synchronous and asynchronous requests.

