

// Include before restincurl.h
#include "logfault/logfault.h"

#include "restincurl/restincurl.h"
#include "nlohmann/json.hpp"

#if defined(WIN32) && defined(ERROR)
// Microsoft's baby-compiler cannot resist the temptation to pollute the global name-space with defines...
#   undef ERROR
#endif // baby-compiler


using namespace std;
using namespace restincurl;
using namespace logfault;
using namespace nlohmann;

int main( int argc, char * argv[]) {

     // Use logfault for logging and log to std::clog on DEBUG level
    LogManager::Instance().AddHandler(
        make_unique<StreamHandler>(clog, logfault::LogLevel::DEBUGGING));

    // Create some data
    json j;
    j["title"] = "Dolphins";
    j["body"] = "Thanks for all the fish";
    j["answer"] = 42;
    j["interperation"] = "Unknown in this universe";
    
    Client client;
    client.Build()->Post("http://jsonplaceholder.typicode.com/posts")
    
        // Tell the server that we accept Json payloads
        .AcceptJson()
        
        // Tell the server that we have a Json body, and 
        // hand that body to the request.
        // json::dump() will serialize the data in j as a Json string.
        .WithJson(j.dump())
        
        // Call this lambda when the request is done
        .WithCompletion([&](const Result& result) {
            
            // Check if the request was successful
            if (result.isOk()) {
                LFLOG_DEBUG << "Returned body was " << result.body;
                try {
                    
                    // Parse the response body we got as Json.
                    const auto j = json::parse(result.body);
                    
                    // We expect to get an id for the object we just sent
                    LFLOG_DEBUG << "The object was assigned id " << j["id"].get<int>();
                    
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
