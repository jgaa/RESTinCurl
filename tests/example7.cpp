

// Include before restincurl.h
#include <future>
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
    
    std::promise<void> holder;
    auto future = holder.get_future();

    json j;
    j["title"] = "Dolphins";
    j["body"] = "Thanks for all the fish";
    j["answer"] = 42;
    j["interperation"] = "Unknown in this universe";
    
    Client client;
    client.Build()->Post("http://jsonplaceholder.typicode.com/posts")
        .AcceptJson()        
        .WithJson(j.dump())
        .WithCompletion([&](const Result& result) {
            if (result.isOk()) {
                LFLOG_DEBUG << "Returned body was " << result.body;
                try {
                    const auto j = json::parse(result.body);
                    const auto id = j["id"].get<int>();
                    LFLOG_DEBUG << "The object was assigned id " << id;
                    
                    // Delete the object we just made
                    // Note that we just add the id to the path, without any use of macros
                    // or placeholders.
                    client.Build()->Delete("http://jsonplaceholder.typicode.com/posts/" + to_string(id))
                        .WithCompletion( [&] (const Result& result) {
                            if (result.isOk()) {
                                LFLOG_DEBUG << "Deleted the element. The returned body was " << result.body;
                            } else {
                                LFLOG_ERROR << "Delete failed: " << result.msg << endl
                                    << "HTTP code: " << result.http_response_code << endl;
                            }
                            
                            holder.set_value();
                        })
                        .Execute();
                        
                } catch (const std::exception& ex) {
                    LFLOG_ERROR << "Caught exception: " << ex.what();
                    holder.set_value();
                    return;
                }
                
            } else {
                LFLOG_ERROR << "Post failed: " << result.msg << endl
                    << "HTTP code: " << result.http_response_code << endl;
                holder.set_value();
            }
        })
        .Execute();

    future.get();
    client.CloseWhenFinished();
    client.WaitForFinish();
    
    // Done
}
