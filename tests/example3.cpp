

#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1
#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {
    
    Client client;

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
