

#define RESTINCURL_MAX_CONNECTIONS 3
#define RESTINCURL_ENABLE_ASYNC 1
#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1
#define RESTINCURL_LOG_VERBOSE_ENABLE 1

#include <future>

#include "restincurl/restincurl.h"

#include "lest/lest.hpp"

using namespace std;
using namespace restincurl;

#define STARTCASE(name) { CASE(#name) { \
    clog << "================================" << endl; \
    clog << "Test case: " << #name << endl; \
    clog << "================================" << endl;

#define ENDCASE \
    clog << "============== ENDCASE =============" << endl; \
}},


const lest::test specification[] = {


STARTCASE(QueueTests)
{
    restincurl::Client client;

    string data;

    restincurl::InDataHandler<std::string> data_handler(data);

    size_t callback_called = {}, num_requests = 16;
    for (auto i = 0 ; i < num_requests; ++i) {
        client.Build()->Get("http://localhost:3001/normal/posts")
            .AcceptJson()
            .StoreData(data)
            .Header("X-Client", "restincurl")
            .WithCompletion([&](const Result& result) {
                EXPECT(result.curl_code == CURLE_OK);
                EXPECT(result.http_response_code == 200);
                EXPECT(!data.empty());
                ++callback_called;
                EXPECT(client.GetNumActiveRequests() <= RESTINCURL_MAX_CONNECTIONS);
            })
            .Execute();
    }

    client.CloseWhenFinished();
    client.WaitForFinish();
    EXPECT(callback_called == num_requests);

} ENDCASE

}; //lest

int main( int argc, char * argv[] )
{
    RESTINCURL_LOG("Running tests in thread " << std::this_thread::get_id());
    return lest::run( specification, argc, argv );
}
