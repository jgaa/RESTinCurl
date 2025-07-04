
// We don't use asynchronous IO in this program, so no need to add the code for that.
#define RESTINCURL_ENABLE_ASYNC 1
#define RESTINCURL_ENABLE_ASIO 1

#include <boost/asio.hpp>

#include "restincurl/restincurl.h"

static_assert(__cplusplus >= 202002L,
              "This example requires C++20 or later. Please use a C++20 compatible compiler.");

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    // Construct the curl Client
    Client client;

    boost::asio::io_context ctx;
    auto future = boost::asio::co_spawn(ctx, [&]() -> boost::asio::awaitable<void> {
        // Get a RequestBuilder by calling Build() and specify that we
        // will require a GET request by calling RequestBuilder::Get()
        auto result = co_await client.Build()->Get("https://example.com")

            // Google will always redirect, so we must allow libcurl
            // to follow redirects.
            .Option(CURLOPT_FOLLOWLOCATION, 1L)

            // Call the coroutine version of Execute
            .AsioAsyncExecute(boost::asio::use_awaitable);

        clog << "In co-routine! HTTP result code was " << result.http_response_code << endl
             << "Body size was " << result.body.size() << " bytes." << endl
             << "Body: " << endl << result.body << endl;

    }, boost::asio::use_future);

    ctx.run();

    try {
        future.get();
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
