# REST in Curl - Modern C++ header only library wrapper around libcurl

C++ is fun. Boost::asio is lots of fun. Unfortunately, in some projects, they cannot be used without introducing great pain. That's why we need the *RESTinCurl* library.
For desktop and server projects, I personally prefer the much more interesting [restc-cpp](https://github.com/jgaa/restc-cpp) project.

*RESTinCurl* is a very simple library that use the C library libcurl to perform HTTP client operations on web servers. It's targeting REST api's using HTTP and (probably) json. Restincurl does not provide json by itself. In our examples, we use the excellent [nlohmann/json](https://github.com/nlohmann/json) header only library.

## Design Goals

- We use libcurl to keep the code size and dependencies at a minimum (for mobile and iot). No boost libraries in this project.
- IO must be asynchronous, allowing any realistic numbers of REST requests from a single worker thread.
- We must be able to shut down immediately, killing the worker thread at any time and cleaning up.
- The library must use resources in a sane way, and silently shut down the worker thread and curl connection pool when it has been idle for a while.

## How it works

Using *RESTinCurl* is simple.

You build a query, and a functor that gets called when the request finish (or fail).

Example:

```C++
restincurl::Client client;
std::string data;
client.Build()->Get("https://api.example.com/")
    .AcceptJson()
    .StoreData(data)
    .Header("X-Client", "restincurl")
    .Trace()
    .WithCompletion([&](const Result& result) {
        clog << "In callback! HTTP result code was " << result.http_response_code << endl;
    })
    .Execute();
```

Here use `AcceptJson` to tell the server that we accept json payloads. We ask *RESTinCurl* to
put whatever data that comes from the server to the string `data`. We add a header. We enable Tracing,
which means that *RESTinCurl* will log verbose output of it's internal workings (if we enable logging).
Then we provide a lambda function to be called when the request is done. And finally, we call
`Execute` to send the request.

An instance of `Client` contains a worker-thread that will handle all the clients requests. The processing
is asynchronous, and due to libcurl's efficiency, we can work with thousands of concurrent requests in
that thread.

## Data

*RESTinCurl* is a very thin wrapper over libcurl. When you set up a request that returns data, you need to
supply a data-buffer for the data. For json payloads, a std::string is fine. Since the data-buffer must
stay in scope until the asynchronous request is complete, I often use `std::shared_ptr<std::string>` types and
capture an instance of the `shared_ptr` in the lambda. That way I don't have to further track the lifetime of the
data-buffer in code.

## Example from a real app

Here is a method called when a user click on a button in an IOS or Android app using this library.
It's called from the UI thread and returns immediately. When the request is finished, the
callback supplied to `getBalance` is called.

```C++
void AccountImpl::getBalance(balance_callback_t callback) const {
    const auto address = getAddress();
    const string url = get_rest_url() + "api.v.1.0/balance?address="s + address;
    auto data = make_shared<string>(); // We get the body from the rest request here
    curl_.Build()->Get(url)
        .Trace(true)
        .Option(CURLOPT_SSL_VERIFYPEER, 0L)
        .StoreData(*data)
        .AcceptJson()
        .WithCompletion([this, data, callback](const restincurl::Result& result) {
            if (result.curl_code == 0 && result.http_response_code == 200) {
                LFLOG_IFALL_DEBUG("Fetched balance: " << *data);
                callback(*data, {});
            } else {
                // Failed.
                LFLOG_IFALL_DEBUG("Failed to fetch balance: " << *data);
                callback({}, result));
            }
        })
        .Execute();
}
```

## More examples

Here is a [test-app](tests/app_test.cpp), doing one request from main()

The [test-cases](tests/general_tests.cpp) shows most of the features in use.

## Logging

Use these macros to enable logging:

- **`RESTINCURL_ENABLE_DEFAULT_LOGGER`** Log to `std::clog`
- **`RESTINCURL_USE_SYSLOG`** Log to syslog (Linux / Unix)

If you use [logfault](https://github.com/jgaa/logfault), you may find this code useful:

```C++
#define RESTINCURL_LOG(msg) LFLOG_IFALL_TRACE("restincurl: " << msg)
#include "restincurl/restincurl.h"
```

## Todo's
- [ ] Ensure thread safety with OpenSSL
- [x] Make some sort of timer, so that we clean up and stop the thread after #time
- [x] Make sure connection re-use it utilized
- [ ] Queue new requests if we reach RESTINCURL_MAX_CONNECTIONS concurrent connections
- [ ] Write complete documentation
