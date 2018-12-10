# RESTinCurl Tutorial

Let start by making a very simple program that simply fetches
the content on Google.com and prints some status info and the
raw HTML payload Google returned.

**Fetch Google.com synchronously**
```C++
// We don't use asynchronous IO in this program, so no need to add the code for that. 
#define RESTINCURL_ENABLE_ASYNC 0

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    // Construct an instance of the curl Client
    restincurl::Client client;

    // Get a RequestBuilder instance by calling Build(), and specify that we 
    // will require a GET request by calling RequestBuilder::Get()
    client.Build()->Get("https://google.com")
    
        // Google will always redirect, so we must allow libcurl 
        // to follow redirects.
        .Option(CURLOPT_FOLLOWLOCATION, 1L)
        
        // Add a lambda function to be called when the request is complete
        .WithCompletion([&](const Result& result) {
            clog << "In callback! HTTP result code was " << result.http_response_code << endl
                << "Body size was " << result.body.size() << " bytes." << endl 
                << "Body: " << endl << result.body << endl;
        })
        
        // Execute the request using the current thread.
        .ExecuteSynchronous();
        
    // We are done!
}

```

This program may output something like:

```
In callback! HTTP result code was 200
Body size was 50223 bytes.
Body: 
<!doctype html><html itemscope="" ...
```

Now, let's do the same again, but this time using the asynchronous
mode. In this example, the main thread will declare the request,
and then wait while the client start a worker-thread and use
that to execute the request and call the lambda function (our
completion callback). 

**Fetch Google.com asynchronously**
```C++
#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    // Construct the curl Client
    restincurl::Client client;

    // Get a RequestBuilder by calling Build() and specify that we 
    // will require a GET request by calling RequestBuilder::Get()
    client.Build()->Get("https://google.com")
    
        // Google will always redirect, so we must allow libcurl 
        // to follow redirects.
        .Option(CURLOPT_FOLLOWLOCATION, 1L)
        
        // Add a lambda function to be called when the request is complete
        .WithCompletion([&](const Result& result) {
            clog << "In callback! HTTP result code was " << result.http_response_code << endl
                << "Body size was " << result.body.size() << " bytes." << endl 
                << "Body: " << endl << result.body << endl;
        })
        
        // Execute the request in the current thread.
        .Execute();
        
    // If the client goes out of scope, it will terminate all ongoing
    // requests, so we need to wait here for the request to finish.

    // Tell client that we want to close when the request is finish
    client.CloseWhenFinished();

    // Wait for the worker-thread in the client to quit
    client.WaitForFinish();
        
    // We are done!
}
```

The output will be the same as before.

Note that the only difference in the request is that we called `RequestBuilder::Execute()` in 
stead of `RequestBuilder::ExecuteSynchronous()`.

Then at the end of the file we have two statements to stop the event-loop in Client,
and to wait for the Client to finish up. Since our test program don't have it's own
event-loop (unlike most desktop and mobile applications) we need to make sure the 
curl client don't go out of scope while it's still active.

How about fetching Google.com 10 times?

**Fetch google.com 10 times in parallell**
```C++
#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1
#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {
    
    restincurl::Client client;

    for(auto i = 0; i < 10; i++) {
        client.Build()->Get("https://google.com")
            .Option(CURLOPT_FOLLOWLOCATION, 1L)
            .WithCompletion([&](const Result& result) {
                clog << "In callback! HTTP result code was " 
                    << result.http_response_code << endl
                    << "Body size was " << result.body.size() 
                    << " bytes." << endl;
            })
            .Execute();
    } // loop
        
    client.CloseWhenFinished();
    client.WaitForFinish();
        
    // We are done!
}
```

In this example we enabled the built-in log functionality (``#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1``) to
get some information about what RESTinCurl does written to
`std::clog`.

The output from running the program reads:

```
One time initialization of curl.
EasyHandle created: 0x55f5f52405b0
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f5245e00
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f524b650
Preparing connect to: https://google.com
Starting thread EasyHandle created: 0x55f5f5250ea0140719191344896

Preparing connect to: https://google.com
EasyHandle created: 0x55f5f52566f0
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f525bf40
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f5261790
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f5266fe0
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f526c830
Preparing connect to: https://google.com
EasyHandle created: 0x55f5f5272080
Preparing connect to: https://google.com
Finishing request with easy-handle: 0x55f5f525bf40; with result: 0 expl: 'No error'; with msg: 1
Complete: http code: 200
In callback! HTTP result code was 200
Body size was 50203 bytes.
Cleaning easy-handle 0x55f5f525bf40
Finishing request with easy-handle: 0x55f5f52405b0; with result: 0 expl: 'No error'; with msg: 1
...
```

**Fetch google.com 10 times in parallel with proper logging**
```C++
// Include before restincurl.h
#include "logfault/logfault.h"

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;
using namespace logfault;

int main( int argc, char * argv[]) {

    // Use logfault for logging and log to std::clog on DEBUG level
    LogManager::Instance().AddHandler(
        make_unique<StreamHandler>(clog, LogLevel::DEBUGGING));

    restincurl::Client client;

    for(auto i = 0; i < 10; i++) {
        client.Build()->Get("https://google.com")
            .Option(CURLOPT_FOLLOWLOCATION, 1L)
            .WithCompletion([&](const Result& result) {
                LFLOG_INFO << "In callback! HTTP result code was " 
                    << result.http_response_code << endl
                    << "Body size was " << result.body.size() 
                    << " bytes." << endl;
            })
            .Execute();
    } // loop
        
    client.CloseWhenFinished();
    client.WaitForFinish();
        
    // We are done!
}
```

In this example we introduced [logfault](https://github.com/jgaa/logfault) for logging. 
Logfault was written to accompany RESTinCurl for a commercial mobile project (a crypto-wallet)
and it's natively supported by RESTinCurl.

The output now reads:
```
2018-12-10 09:57:59.017 EET DEBUGGING 139690903103872 restincurl: One time initialization of curl.
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bb8010
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bbd860
2018-12-10 09:57:59.018 EET DEBUGGING 139690903099136 restincurl: Starting thread 139690903099136
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bc30b0
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bc8900
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bce150
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bd39a0
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.018 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bd91f0
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7bdea40
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7be4290
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: EasyHandle created: 0x5591e7be9ae0
2018-12-10 09:57:59.019 EET DEBUGGING 139690903103872 restincurl: Preparing connect to: https://google.com
2018-12-10 09:57:59.353 EET DEBUGGING 139690903099136 restincurl: Finishing request with easy-handle: 0x5591e7bbd860; with result: 0 expl: 'No error'; with msg: 1
2018-12-10 09:57:59.353 EET DEBUGGING 139690903099136 restincurl: Complete: http code: 200
2018-12-10 09:57:59.353 EET INFO 139690903099136 In callback! HTTP result code was 200
Body size was 50210 bytes.

2018-12-10 09:57:59.353 EET DEBUGGING 139690903099136 restincurl: Cleaning easy-handle 0x5591e7bbd860
2018-12-10 09:57:59.371 EET DEBUGGING 139690903099136 restincurl: Finishing request with easy-handle: 0x5591e7be4290; with result: 0 expl: 'No error'; with msg: 1
2018-12-10 09:57:59.371 EET DEBUGGING 139690903099136 restincurl: Complete: http code: 200
2018-12-10 09:57:59.371 EET INFO 139690903099136 In callback! HTTP result code was 200
Body size was 50218 bytes.

2018-12-10 09:57:59.371 EET DEBUGGING 139690903099136 restincurl: Cleaning easy-handle 0x5591e7be4290
2018-12-10 09:57:59.378 EET DEBUGGING 139690903099136 restincurl: Finishing request with easy-handle: 0x5591e7bc8900; with result: 0 expl: 'No error'; with msg: 1
```

## Using REST API's

Fetching Google.com is OK, but that's not what RESTinCurl was designed for. It's written to 
make it simple to access REST API's from C++ on mobile / IOT platforms. 

So, let's use a public facing REST API [http://jsonplaceholder.typicode.com/posts](http://jsonplaceholder.typicode.com/posts) 
mock service to see how to work with Json data.

The data returned will look like this (just that there will be more records):
```json
[
  {
    "userId": 1,
    "id": 1,
    "title": "sunt aut facere repellat provident occaecati excepturi optio reprehenderit",
    "body": "quia et suscipit\nsuscipit recusandae consequuntur expedita et cum\nreprehenderit molestiae ut ut quas totam\nnostrum rerum est autem sunt rem eveniet architecto"
  },
  {
    "userId": 1,
    "id": 2,
    "title": "qui est esse",
    "body": "est rerum tempore vitae\nsequi sint nihil reprehenderit dolor beatae ea dolores neque\nfugiat blanditiis voluptate porro vel nihil molestiae ut reiciendis\nqui aperiam non debitis possimus qui neque nisi nulla"
  }
]
```

So, let's query this REST API and process the received data.

```C++
// Include before restincurl.h
#include "logfault/logfault.h"

#include "restincurl/restincurl.h"
#include "nlohmann/json.hpp"

using namespace std;
using namespace restincurl;
using namespace logfault;
using namespace nlohmann;

int main( int argc, char * argv[]) {

     // Use logfault for logging and log to std::clog on DEBUG level
    LogManager::Instance().AddHandler(
        make_unique<StreamHandler>(clog, logfault::LogLevel::TRACE));

    restincurl::Client client;

    client.Build()->Get("http://jsonplaceholder.typicode.com/posts")
    
        // Tell the server that we accept Json payloads
        .AcceptJson()
        .WithCompletion([&](const Result& result) {
            if (result.curl_code == 0 && result.http_response_code == 200) {
                
                try {
                    
                    // Parse the response body we got as Json.
                    const auto j = json::parse(result.body);
                    
                    // j should be a Json array, so we can query for size().
                    LFLOG_INFO << "We received " << j.size() << " elements.";
                    
                    // Loop over the array and extract each Json element
                    for(const auto& r : j) {
                        
                        // Print the id and title for each element
                        LFLOG_INFO << "  -> " << r["id"].get<int>() << ' ' << r["title"].get<string>();
                    }
                    
                } catch (const std::exception& ex) {
                    LFLOG_ERROR << "Caught exception: " << ex.what();
                    return;
                }
                
            } else {
                LFLOG_ERROR << "Request failed: " << result.msg << endl
                    << "HTTP code: " << result.http_response_code << endl;
            }
        })
        .Execute();

        
    client.CloseWhenFinished();
    client.WaitForFinish();
    
    // Done
}
```

Here we are using the excellent nlohmann/json Json library to parse and extract data from the raw
Json payload we received from the server. 

The output may look something like:
```
2018-12-10 10:23:30.970 EET DEBUGGING 140499094301056 restincurl: One time initialization of curl.
2018-12-10 10:23:30.971 EET DEBUGGING 140499094301056 restincurl: EasyHandle created: 0x55d7c51c2010
2018-12-10 10:23:30.971 EET DEBUGGING 140499094301056 restincurl: Preparing connect to: http://jsonplaceholder.typicode.com/posts
2018-12-10 10:23:30.971 EET DEBUGGING 140499094296320 restincurl: Starting thread 140499094296320
2018-12-10 10:23:31.097 EET DEBUGGING 140499094296320 restincurl: Finishing request with easy-handle: 0x55d7c51c2010; with result: 0 expl: 'No error'; with msg: 1
2018-12-10 10:23:31.097 EET DEBUGGING 140499094296320 restincurl: Complete: http code: 200
2018-12-10 10:23:31.100 EET INFO 140499094296320 We reveived 100 records.
2018-12-10 10:23:31.100 EET INFO 140499094296320   -> 1 sunt aut facere repellat provident occaecati excepturi optio reprehenderit
2018-12-10 10:23:31.100 EET INFO 140499094296320   -> 2 qui est esse
2018-12-10 10:23:31.100 EET INFO 140499094296320   -> 3 ea molestias quasi exercitationem repellat qui ipsa sit aut
2018-12-10 10:23:31.100 EET INFO 140499094296320   -> 4 eum et est occaecati
2018-12-10 10:23:31.100 EET INFO 140499094296320   -> 5 nesciunt quas odio
...
2018-12-10 10:23:31.103 EET INFO 140499094296320   -> 96 quaerat velit veniam amet cupiditate aut numquam ut sequi
2018-12-10 10:23:31.103 EET INFO 140499094296320   -> 97 quas fugiat ut perspiciatis vero provident
2018-12-10 10:23:31.103 EET INFO 140499094296320   -> 98 laboriosam dolor voluptates
2018-12-10 10:23:31.103 EET INFO 140499094296320   -> 99 temporibus sit alias delectus eligendi possimus magni
2018-12-10 10:23:31.103 EET INFO 140499094296320   -> 100 at nam consequatur ea labore ea harum
2018-12-10 10:23:31.103 EET DEBUGGING 140499094296320 restincurl: Cleaning easy-handle 0x55d7c51c2010
2018-12-10 10:23:31.103 EET DEBUGGING 140499094296320 restincurl: Exiting thread 140499094296320

```

