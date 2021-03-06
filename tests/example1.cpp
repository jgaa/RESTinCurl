
// We don't use asynchronous IO in this program, so no need to add the code for that. 
#define RESTINCURL_ENABLE_ASYNC 0

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    // Construct the curl Client
    Client client;

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
        .ExecuteSynchronous();
        
    // We are done!
}
