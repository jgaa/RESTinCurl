
/* Simple application using the library.
 *
 */
//#define RESTINCURL_USE_SYSLOG 0
#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1

#include <fstream>
#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

/*! Generate a temporary file so we have something to send.
 *
 *  Delete the file inb the destructor
 *
 *  Using C++14, so no std::filesystem...
 *
 */
struct TmpFile {
    TmpFile() {
        ofstream file(name_);
        for(int i = 0; i < 1000; i++) {
            file << "This is line #" << i << endl;
        }
    }

    ~TmpFile() {
        unlink(name_.c_str());
    }

    string Name() {
        return name_;
    }

private:
    string name_ = tmpnam(nullptr);
;};

int main( int argc, char * argv[]) {

    TmpFile tmpfile;

    restincurl::Client client;

    // Send directly as a raw data-stream
    client.Build()->Post("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("Content-Type", "application/octet-stream")
        .Header("X-Origin-File-Name", tmpfile.Name())
        .WithCompletion([&](const Result& result) {
            clog << "In callback! HTTP result code was " << result.http_response_code << endl;
            clog << "Data was " << result.body.size() << " bytes." << endl;
        })
        .SendFile(tmpfile.Name())
        .Execute();

    // Send as a MIME form-data attachment
    client.Build()->PostMime("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("X-Origin-File-Name", tmpfile.Name())
        .WithCompletion([&](const Result& result) {
            clog << "In callback! HTTP result code was " << result.http_response_code << endl;
            clog << "Data was " << result.body.size() << " bytes." << endl;
        })
        .SendFileAsMimeData(tmpfile.Name(), "My-File", "MyFile.txt", "text/plain")
        .Execute();

    // If the client goes out of scope, it will terminate all ongoing
    // requests, so we need to wait here for the request to finish.

    // Tell client that we want to close when the request is finish
    client.CloseWhenFinished();

    // Wait for the worker-thread in the client to quit
    client.WaitForFinish();
}
