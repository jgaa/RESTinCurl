

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
        make_unique<StreamHandler>(clog, logfault::LogLevel::DEBUGGING));

    Client client;

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
