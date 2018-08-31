

#include "restincurl/restincurl.h"

#include "lest/lest.hpp"

using namespace std;
using namespace restincurl;

#define STARTCASE(name) { CASE(#name) { \
    clog << "================================"; \
    clog << "Test case: " << #name; \
    clog << "================================";

#define ENDCASE \
    clog << "============== ENDCASE ============="; \
}},


const lest::test specification[] = {

STARTCASE(TestSimpleGet)
{
    restincurl::Client client;

    string data;

    client.Build()->Get("http://localhost:3000")
        //.Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](auto result) {
            EXPECT(result == CURLE_OK);
            EXPECT(!data.empty());
        })
        .Execute();

} ENDCASE

STARTCASE(TestSimpleGetWithHttps)
{
    restincurl::Client client;

    string data;

    client.Build()->Get("https://google.com")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](auto result) {
            EXPECT(result == CURLE_OK);
            EXPECT(!data.empty());
        })
        .Execute();

} ENDCASE

}; //lest

int main( int argc, char * argv[] )
{
    return lest::run( specification, argc, argv );
}
