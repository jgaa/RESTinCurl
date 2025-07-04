// tests/coro-unifex/coro-unifex.cpp

#define RESTINCURL_ENABLE_ASYNC 1

#include <iostream>
#include <exception>

#include <unifex/task.hpp>
#include <unifex/sync_wait.hpp>

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

// A simple coroutine returning a unifex::task<Result>
unifex::task<Result> do_request(Client& client) {
    // co_await your generic awaitable
    Result r = co_await client.Build()
                   ->Get("https://example.com")
                   .Option(CURLOPT_FOLLOWLOCATION, 1L)
                   .CoExecute();
    co_return r;
}

int main() {
    Client client;

    try {
        // Run the coroutine to completion on this thread
        auto res = unifex::sync_wait(do_request(client));
        assert(res);

        cout << "HTTP status: " << res->http_response_code << "\n"
             << "Body size:   " << res->body.size() << " bytes\n"
             << "Body:\n"
             << res->body << "\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
