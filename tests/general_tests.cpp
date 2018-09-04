
#define RESTINCURL_ENABLE_ASYNC 1

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

STARTCASE(TestSimpleGet)
{
    restincurl::Client client;

    string data;

    restincurl::InDataHandler<std::string> data_handler(data);

    bool callback_called = false;
    client.Build()->Get("http://localhost:3001/normal/manyposts")
        //.Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        //.StoreData(data_handler)
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            EXPECT(result.http_response_code == 200);
            EXPECT(!data.empty());
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);

} ENDCASE

STARTCASE(TestSimpleGetWithHttps)
{
    restincurl::Client client;

    string data;

    restincurl::InDataHandler<std::string> data_handler(data);

    bool callback_called = false;
    client.Build()->Get("https://google.com")
        //.Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        //.StoreData(data_handler)
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);

} ENDCASE

STARTCASE(TestAbort)
{
    // Test abort
    restincurl::Client client;
    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .Header("X-Client", "restincurl")
        .Execute();

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    client.Close();
    client.WaitForFinish();

} ENDCASE

}; //lest

int main( int argc, char * argv[] )
{
    return lest::run( specification, argc, argv );
}
