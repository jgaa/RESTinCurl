# REST in Curl - Modern C++ header only library wrapper around libcurl

C++ is fun. Boost::asio is lots of fun. Unfortunately, in some projects, they cannot be used without introducing great pain. That's why we need 
the [*RESTinCurl*](https://github.com/jgaa/RESTinCurl) library.
For desktop and server projects, I personally prefer the much more interesting [restc-cpp](https://github.com/jgaa/restc-cpp) project.

*RESTinCurl* is a very simple library that use the C library libcurl to perform HTTP client operations on web servers. It's targeting REST api's using HTTP and (probably) json. Restincurl does not provide json by itself. In my own projects, I use the excellent [nlohmann/json](https://github.com/nlohmann/json) header only library.

## Design Goals

- We use libcurl to keep the code size and dependencies at a minimum (for mobile and IOT). No boost library dependencies.
- IO must be asynchronous, allowing any realistic numbers of REST requests from a single worker thread.
- We must be able to shut down immediately, killing the worker thread at any time and cleaning up.
- The library must use resources in a sane way, and silently shut down the worker thread and curl connection pool when it has been idle for a while.
- Utilize C++14 to keep simple things simple.

## Features

- Very simple to use. You don't need to understand the inner workings of libcurl to use RESTinCurl!
- Limited use of C++ templates to keep the binary size down. The library is geared towards mobile/IOT use-cases.
- Supports all standard HTTP request types.
- Supports synchronous and asynchronous requests (using the same API).
- Exposes a request-builder class to make it dead simple to express requests.
- Flexible logging.
- Hides libcurl's awkward data callbacks and let's you work with std::string's in stead (if you want to).
- Exposes all libcurl's options to you, via convenience methods, or directly.
- Implements it's own asynchronous event-loop, and expose only a simple, modern, intuitive API to your code.
- One instance use only one worker-thread. The thread is started on demand and stopped when the instance has been idle for a while.
- Tuned towards REST/Json use-cases (but still usable for general/binary HTTP requests).
- Supports sending files as raw data or MIME attachments.

## How to use it in your C++ project

You can add it to you project as a git sub-module, download it as part of your
projects build (from CMake or whatever magic you use), or you can just
download the header and copy it to your project (if you copy it, you should
pay attention to new releases in case there are any security fixes).

The project have a `CMakeLists.txt`. This used to build and run the tests
with cmake . It's not required in order to use the library. All yo need to do
is to include the header-file.

## External dependencies

- the C++14 standard library
- libcurl

## How it works

Using *RESTinCurl* is simple.

You build a query, and provide a functor that gets called when the request finish (or fail).

Example:

```C++
restincurl::Client client;
client.Build()->Get("https://api.example.com/")
    .WithCompletion([](const restincurl::Result& result) {
       std::clog << "It works!" << std::endl;
    })
    .Execute();

```

## Data

*RESTinCurl* is a very thin wrapper over libcurl. Libcurl reads and writes payload data from / to
C callback functions while serving a request. That gives a lot of flexibility, but it's a bit awkward
to implement. RESTinCurl gives you full flexibility in that you can use the C API for this, by providing
your own callback functions, you can use a C++ template abstraction that reads/writes to a container, 
or you can simply use std::string buffers. For json payloads, strings are perfect, and RESTinCurl 
provides a very simple and intuitive interface. 

## Threading model

In asynchronous mode (the default mode of operation), each instance of a Client class will
start one worker-thread that will deal with all concurrent requests performed by that
instance. The worker-thread is started when the first request is made, and it will be
terminated after a configurable timeout-period when the client is idle. 

Asynchronous requests returns immediately, and an (optional) callback will be called when the 
request finish (or fails). The callback is called from the worker-thread. In the callback you should
return immediately, and do any time-consuming processing in another thread. 

Since all IO happens in the worker-thread, no data synchronization should normally be required, 
leading to better performance and simpler code. 

## Example from a real app

Here is a method called when a user click on a button in an IOS or Android app using this library.
It's called from the UI thread and returns immediately. The lambda function body is called from 
a worker-thread.

```C++
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
                    LFLOG_ERROR << "Failed to fetch balance. http code: " << result.http_response_code
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

The code above use the [logfault](https://github.com/jgaa/logfault) C++ header only log-library.

## More examples

Here is a [test-app](tests/app_test.cpp), doing one request from main()

The [test-cases](tests/general_tests.cpp) shows most of the features in use.

## Logging

Logging is essential for debugging applications. For normal applications, you can normally
get away with logging to standard output or standard error. On mobile, things are little more
involved. Android has it's own weird logging library for NDK projects, and IOS has it's own 
logging methods that are not even exposed to C++! 

RESTinCurl has a very simple logging feature that can write logs to std::clog, syslog or 
Android's `__android_log_write()`. It may be all you need to debug your REST requests. 

Just `#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1` to enable it.

If you want to use the Android NDK logger, also `#define RESTINCURL_USE_ANDROID_NDK_LOG`. 

If you want to use syslog, also `#define RESTINCURL_USE_SYSLOG`.


**Logfault**

If you want full support for log files, IOS/Macos logging, Android, Syslog etc, you can 
get the [logfault](https://github.com/jgaa/logfault) C++ header only log-library, and 
`#include "logfault/logfault.h"` before you `#include "restincurl/restincurl.h"`. 
RESTinCurl will detect that you use logfault and adapt automatically. 

**Other log libraries**

RESTinCurl use two macros, `RESTINCURL_LOG(...)` and `RESTINCURL_LOG_TRACE(...)` to generate log
content. If you use another log library, or your own logging functions, you can define those two
macros to fit your needs. You can examine the code for the default implementation in `restincurl.h`
to see how that can be done. Hint: It's simple. 

## Todo's
- [x] Ensure thread safety with OpenSSL
- [ ] Validate OpenSSL therad safety
- [x] Make some sort of timer, so that we clean up and stop the thread after #time
- [x] Make sure connection re-use it utilized
- [x] Queue new requests if we reach RESTINCURL_MAX_CONNECTIONS concurrent connections
- [x] Write complete documentation
- [x] Write tutorial
- [x] Test cases for non-async mode (RESTINCURL_ENABLE_ASYNC=0)
- [ ] CloseWhenFinished() must allow new requests to enter the queue and to be completed
- [ ] Verify that DELETE is implemented correctly.
- [ ] Add correctly encoded query parameters from the RequestBuilder
- [ ] Port to windows
- [ ] Set up CI with some tests on Ububtu LTS, Debian Stable, Windows, macos
- [ ] Add Android to CI
- [ ] Add IOS to CI


## Further reading

- You may want to look at the [tutorial](https://github.com/jgaa/RESTinCurl/blob/master/doc/tutorial.md)
