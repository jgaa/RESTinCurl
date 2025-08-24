# REST in Curl – Modern C++ Header-Only Library Wrapper Around libcurl

C++ is fun. Boost::asio is lots of fun. Unfortunately, in some projects, they cannot be used without introducing great pain. That's why we need the [*RESTinCurl*](https://github.com/jgaa/RESTinCurl) library.
For desktop and server projects, I personally prefer the more feature-rich [restc-cpp](https://github.com/jgaa/restc-cpp) project.

*RESTinCurl* is a very simple library that uses the C library libcurl to perform HTTP client operations on web servers. It targets REST APIs over HTTP and (probably) JSON. RESTinCurl does not provide JSON support by itself. In my own projects, I use the excellent [nlohmann/json](https://github.com/nlohmann/json) header-only library.

## Design Goals

* Use libcurl to keep code size and dependencies at a minimum (for mobile and IoT). No Boost library dependencies.
* IO must be asynchronous, allowing realistic numbers of REST requests from a single worker thread.
* Support immediate shutdown, killing the worker thread at any time and cleaning up.
* Use resources responsibly, and silently shut down the worker thread and curl connection pool when idle.
* Utilize C++14 to keep simple things simple.

## Features

* Very simple to use. You don't need to understand libcurl’s internals to use RESTinCurl.
* Supports C++20 coroutines (generic and Asio).
* Limited use of C++ templates to keep binary size down (geared toward mobile/IoT use cases).
* Supports all standard HTTP request types.
* Supports synchronous and asynchronous requests (with the same API).
* Provides a request-builder class to make requests easy to express.
* Flexible logging.
* Hides libcurl's awkward data callbacks and lets you work with `std::string` instead (if you want to).
* Exposes all libcurl options to you via convenience methods or directly.
* Implements its own asynchronous event loop, exposing only a simple, modern, intuitive API to your code.
* One instance uses only one worker thread. The thread starts on demand and stops when the instance has been idle for a while.
* Tuned toward REST/JSON use cases (but still usable for general/binary HTTP requests).
* Supports sending files as raw data or MIME attachments.

## How to Use It in Your C++ Project

You can add it to your project as a Git submodule, download it as part of your project’s build (via CMake or whatever you prefer), or simply copy the header file into your project.
If you copy it manually, make sure to check for new releases in case of security fixes.

The project provides a `CMakeLists.txt` file. This is mainly for building and running the tests with CMake; it is not required to use the library.
All you need to do is include the header file.

### CMake Integration

If you're using CMake, RESTinCurl provides a modern CMake target for easy integration.

#### Using FetchContent (Recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
  RESTinCurl
  GIT_REPOSITORY https://github.com/jgaa/RESTinCurl.git
  GIT_TAG main  # or a specific version tag
)
FetchContent_MakeAvailable(RESTinCurl)

target_link_libraries(your_target RESTinCurl::RESTinCurl)
```

#### Using add\_subdirectory

```cmake
add_subdirectory(path/to/RESTinCurl)
target_link_libraries(your_target RESTinCurl::RESTinCurl)
```

The `RESTinCurl::RESTinCurl` target automatically handles:

* Include directories
* Required dependencies (libcurl, pthread)
* C++14 standard requirement

## External Dependencies

* C++14 standard library
* libcurl

## How It Works

Using *RESTinCurl* is simple.
You build a query and provide a functor that gets called when the request finishes (or fails).

Example:

```cpp
restincurl::Client client;
client.Build()->Get("https://api.example.com/")
    .WithCompletion([](const restincurl::Result& result) {
       std::clog << "It works!" << std::endl;
    })
    .Execute();
```

## Data

*RESTinCurl* is a very thin wrapper over libcurl. Libcurl reads and writes payload data from/to C callback functions while serving a request.
That provides a lot of flexibility, but it’s a bit awkward to implement. RESTinCurl gives you full flexibility:

* Use the C API with your own callback functions.
* Use a C++ template abstraction to read/write to a container.
* Or simply use `std::string` buffers.

For JSON payloads, strings are ideal, and RESTinCurl provides a simple and intuitive interface.

## Threading Model

In asynchronous mode (the default), each instance of a `Client` class will start one worker thread to handle all concurrent requests.
The worker thread starts with the first request and stops after a configurable timeout when idle.

Asynchronous requests return immediately, and an (optional) callback is invoked when the request finishes (or fails).
The callback is executed in the worker thread. You should return quickly from it and perform any heavy processing in another thread.

Since all IO happens in the worker thread, no data synchronization is usually required, resulting in better performance and simpler code.

## Coroutines

If you’re using C++20 coroutines, RESTinCurl provides two ready-to-use awaitable APIs:

### 1. `CoExecute()` – Plain C++20 awaitable

```cpp
#define RESTINCURL_ENABLE_ASYNC 1
#include "restincurl/restincurl.h"

auto foo = [&]() -> cppcoro::task<void> {
    // Build a request and co_await it directly:
    restincurl::Result r = co_await client.Build()
        ->Get("https://example.com/resource")
        .Option(CURLOPT_FOLLOWLOCATION, 1L)
        .CoExecute();  // generic awaitable

    std::cout << "HTTP status: " << r.http_response_code
              << ", body size: "  << r.body.size() << "\n";
    co_return;
};
```

* **`CoExecute()`** returns an awaiter that:

  * always suspends the coroutine,
  * enqueues the request on the RESTinCurl worker thread,
  * resumes when the HTTP response arrives,
  * and returns the completed `Result` from `await_resume()`.

* There’s also an lvalue overload, so you can write:

```cpp
  auto rb = client.Build()->Get("…");
  Result r = co_await rb.CoExecute();
```

### 2. `AsioAsyncExecute()` – Boost.Asio-compatible

If you’re already using Boost.Asio, you can integrate RESTinCurl directly into your `io_context`:

```cpp
#define RESTINCURL_ENABLE_ASIO 1
#define RESTINCURL_ENABLE_ASYNC 1

#include <boost/asio.hpp>
#include "restincurl/restincurl.h"

boost::asio::io_context ctx;
auto fut = boost::asio::co_spawn(ctx, [&]() -> boost::asio::awaitable<void> {
    restincurl::Result r = co_await client.Build()
        ->Get("https://example.com/resource")
        .Option(CURLOPT_FOLLOWLOCATION, 1L)
        .AsioAsyncExecute(boost::asio::use_awaitable);

    std::cout << "HTTP status: " << r.http_response_code
              << ", body size: "  << r.body.size() << "\n";
    co_return;
}, boost::asio::use_future);

ctx.run();
fut.get();
```

* **`AsioAsyncExecute(token)`** is a member template that calls `boost::asio::async_initiate` internally.
* You can pass any Asio completion token:

  * `boost::asio::use_future` → `std::future<Result>`
  * `boost::asio::yield_context` → stackful coroutine
  * `boost::asio::use_awaitable` → `boost::asio::awaitable<Result>`

With these two options you can choose **plain C++20 coroutines** or **Asio-integrated coroutines** — and enjoy fully asynchronous REST calls without boilerplate.

## Example from a Real App

Here is a method called when a user clicks a button in an iOS or Android app using this library.
It is called from the UI thread and returns immediately. The lambda function body runs in a worker thread:

```cpp
void AccountImpl::getBalance(IAccount::balance_callback_t callback) const {
    curl_.Build()->Get(getServerUrl())
        .BasicAuthentication(getHttpAuthName(), getHttpAuthPasswd())
        .Trace(APP_CURL_VERBOSE)
        .Option(CURLOPT_SSL_VERIFYPEER, 0L)
        .AcceptJson()
        .WithCompletion([this, callback](const restincurl::Result& result) {
            if (result.curl_code == 0 && result.http_response_code == 200) {
                LFLOG_IFALL_DEBUG("Fetched balance: " << result.body);
                if (callback) {
                    LFLOG_IFALL_DEBUG("Calling callback");
                    callback(result.body, {});
                    LFLOG_IFALL_DEBUG("Callback was called OK");
                } else {
                    LFLOG_ERROR << "No callback to call";
                }
            } else {
                // Failed.
                LFLOG_ERROR << "Failed to fetch balance. HTTP code: "
                            << result.http_response_code
                            << ", error reason: " << result.msg
                            << ", payload: " << result.body;
                if (callback) {
                    LFLOG_IFALL_DEBUG("Calling callback");
                    callback({}, assign(result, result.body));
                    LFLOG_IFALL_DEBUG("Callback was called OK");
                } else {
                    LFLOG_ERROR << "No callback to call";
                }
                return;
            }
        })
        .Execute();
}
```

The code above uses the [logfault](https://github.com/jgaa/logfault) C++ header-only logging library.

## More Examples

* A [test app](tests/app_test.cpp), making one request from `main()`.
* The [test cases](tests/general_tests.cpp) demonstrate most features.
* C++20 [general coroutine example](tests/coro-unifex/coro-unifex.cpp) shows how to use RESTinCurl with the Unifex coroutine library.
* C++20 [Asio coroutine example](tests/coro-asio/coro-asio.cpp) shows how to use RESTinCurl with Boost.Asio coroutines.

## Logging

Logging is essential for debugging applications. For normal applications, you can usually log to standard output or error.
On mobile, things are a little more involved: Android has its own logging API for NDK projects, and iOS has its own logging methods that are not exposed to C++.

RESTinCurl has a very simple logging feature that can write logs to `std::clog`, syslog, or Android's `__android_log_write()`. It may be all you need to debug your REST requests.

Just `#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1` to enable it.

If you want to use the Android NDK logger, also define `RESTINCURL_USE_ANDROID_NDK_LOG`.
If you want to use syslog, define `RESTINCURL_USE_SYSLOG`.

### Logfault

If you want full support for log files, iOS/macOS logging, Android, syslog, etc., you can use the [logfault](https://github.com/jgaa/logfault) C++ header-only logging library, and
`#include "logfault/logfault.h"` before `#include "restincurl/restincurl.h"`.
RESTinCurl will detect logfault automatically and adapt.

### Other Log Libraries

RESTinCurl uses two macros — `RESTINCURL_LOG(...)` and `RESTINCURL_LOG_TRACE(...)` — to generate log output.
If you use another logging library, or your own logging functions, you can redefine these macros to fit your needs.
You can check the default implementation in `restincurl.h` to see how it works. (Hint: it’s simple.)

## Further Reading

* The [tutorial](https://github.com/jgaa/RESTinCurl/blob/master/doc/tutorial.md)

