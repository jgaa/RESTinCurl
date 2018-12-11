
// Include before restincurl.h
#include "logfault/logfault.h"

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;
using namespace logfault;

int main( int argc, char * argv[]) {

    // Use logfault for logging and log to std::clog on DEBUG level
    LogManager::Instance().AddHandler(
        make_unique<StreamHandler>(clog, LogLevel::DEBUGGING));

    Client client;

    for(auto i = 0; i < 10; i++) {
        client.Build()->Get("https://google.com")
            .Option(CURLOPT_FOLLOWLOCATION, 1L)
            .WithCompletion([&](const Result& result) {
                LFLOG_INFO << "In callback! HTTP result code was " 
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
