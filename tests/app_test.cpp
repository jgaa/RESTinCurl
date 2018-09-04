
/* Simple application using the library.
 *
 * Just to get an idea about the size of the
 * binary the library produce.
 */
#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    restincurl::Client client;
    string data;
    restincurl::InDataHandler<std::string> data_handler(data);

    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .AcceptJson()
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            clog << "In callback! HTTP result code was " << result.http_response_code << endl;
        })
        .Execute();

    client.CloseWhenFinished();
    client.WaitForFinish();
}
