# Rest in Curl - Modern C++ header only library wrapping around libcurl

C++ is fun. Boost::asio is lots of fun. Unfortunately, in some projects, they cannot be used without introducing great pain. That's why we need the *restincurl* library.

This is a very simple library that use the C library libcurl to perform HTTP client operations on web servers. It is designed to TARGET rest api's using HTTP and (probably) json. Restincurl does not provide json itself. In our examples, we use the excellent [nlohmann/json](https://github.com/nlohmann/json) header only library.

## Design Goals

- We use libcurl to keep the code size and dependencies at a minimum (for mobile and iot). No boost libraries in this project.
- IO must be asynchronous, allowing any realistic numbers of REST requests from a single worker thread.
- We must be able to shut down immediately, killing the worker thread at any time and cleaning up.
- The library must use resources in a sane way, and silently shut down the worker thread and curl connection pool when it has been idle for a while.



## Todo's

- [ ] Ensure thread safety with OpenSSL
- [v] Set up a shared worker-thread
- [v] Add a queue to add requests
- [ ] Add a pipe to signal the worker-thread via select()
- [ ] Make some sort of timer, so that we clean up and stop the thread after #time
- [ ] Set default timeout on requests
- [ ] Test on IOS
- [ ] Test on Android
- [ ] How does libraries usually shut down on ios and Android when the app is killed?
- [ ] Make sure connection re-use it utilized
