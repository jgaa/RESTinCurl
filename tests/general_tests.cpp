
#define RESTINCURL_ENABLE_ASYNC 1
#define RESTINCURL_ENABLE_DEFAULT_LOGGER 1
#define RESTINCURL_LOG_VERBOSE_ENABLE 1

#include <future>
#include <fstream>

#define RESTINCURL_IDLE_TIMEOUT_SEC 1
#include "restincurl/restincurl.h"

#include "TmpFile.h"

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

    bool callback_called = false;
    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .AcceptJson()
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            EXPECT(result.http_response_code == 200);
            EXPECT(!result.body.empty());
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);

} ENDCASE

STARTCASE(TestGetNoData)
{
    restincurl::Client client;

    bool callback_called = false;
    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .AcceptJson()
        .IgnoreIncomingData()
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            EXPECT(result.http_response_code == 200);
            EXPECT(result.body.empty());
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);

} ENDCASE


STARTCASE(TestGetWithBasicAuthentication)
{
    restincurl::Client client;

    bool callback_called = false;
    client.Build()->Get("http://localhost:3001/restricted/posts/1")
        .AcceptJson()
        .BasicAuthentication("alice", "12345")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            EXPECT(result.http_response_code == 200);
            EXPECT(!result.body.empty());
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

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    client.Close();

} ENDCASE

STARTCASE(TestOutOfScope)
{
    restincurl::Client client;
    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
        })
        .Execute();

} ENDCASE

STARTCASE(TestPost)
{
    string data;
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Post("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .StoreData(data)
        .WithJson()
        .SendData<string>("{\"test\":\"testes\"}")
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            clog << "POST response: " << data << endl;
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE

STARTCASE(TestPatch)
{
    string data;
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Patch("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .StoreData(data)
        .WithJson()
        .SendData<string>("{\"test\":\"testes\"}")
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            clog << "POST response: " << data << endl;
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE

STARTCASE(TestPut)
{
    string data;
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Put("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
        .StoreData(data)
        .WithJson()
        .SendData<string>("{\"test\":\"teste\"}")
        .Header("X-Client", "restincurl")
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            clog << "PUT response: " << data << endl;
            callback_called = true;
        })
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE

STARTCASE(TestHead)
{
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Head("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
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

STARTCASE(TestOptions)
{
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Options("http://localhost:3001/normal/manyposts")
        .Option(CURLOPT_VERBOSE, 1L)
        .AcceptJson()
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


STARTCASE(TestDelete)
{
    restincurl::Client client;
    bool callback_called = false;
    client.Build()->Delete("http://localhost:3001/normal/manyposts/42")
        .Option(CURLOPT_VERBOSE, 1L)
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

STARTCASE(TestThread)
{
#if RESTINCURL_ENABLE_ASYNC
    restincurl::Client client;
    EXPECT(client.HaveWorker() == false);

    {
        std::promise<void> promise;
        auto future = promise.get_future();

        client.Build()->Head("http://localhost:3001/normal/manyposts")
            .Option(CURLOPT_VERBOSE, 1L)
            .AcceptJson()
            .Header("X-Client", "restincurl")
            .WithCompletion([&](const Result& result) {
                EXPECT(result.curl_code == CURLE_OK);
                promise.set_value();
            })
            .Execute();

        future.wait();
    }

    EXPECT(client.HaveWorker() == true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT(client.HaveWorker() == false);

    bool callback_called = false;
    client.Build()->Head("http://localhost:3001/normal/manyposts")
            .Option(CURLOPT_VERBOSE, 1L)
            .AcceptJson()
            .Header("X-Client", "restincurl")
            .WithCompletion([&](const Result& result) {
                EXPECT(result.curl_code == CURLE_OK);
                callback_called = true;
            })
            .Execute();

    client.CloseWhenFinished();
    client.WaitForFinish();
    EXPECT(client.HaveWorker() == false);
    EXPECT(callback_called);

#endif
} ENDCASE


STARTCASE(TestUploadRawOk)
{
    TmpFile tmpfile;
    restincurl::Client client;
    bool callback_called = false;

    // Send directly as a raw data-stream
    client.Build()->Post("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("Content-Type", "application/octet-stream")
        .Header("X-Origin-File-Name", tmpfile.Name())
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            callback_called = true;
        })
        .SendFile(tmpfile.Name())
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE

STARTCASE(TestUploadRawNoFile)
{
    restincurl::Client client;
    bool callback_called = false;

    const std::string nofile = "/no-file/12345/if-this-exists-blame-yourself";


    // Send directly as a raw data-stream
    EXPECT_THROWS_AS(
    client.Build()->Post("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("Content-Type", "application/octet-stream")
        .Header("X-Origin-File-Name", nofile)
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            callback_called = true;
        })
        .SendFile(nofile)
        .Execute();
    , SystemException);

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(!callback_called);
} ENDCASE

STARTCASE(TestUploadMimeOk)
{
    TmpFile tmpfile;
    restincurl::Client client;
    bool callback_called = false;

    // Send directly as a raw data-stream
    client.Build()->PostMime("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("Content-Type", "application/octet-stream")
        .Header("X-Origin-File-Name", tmpfile.Name())
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code == CURLE_OK);
            callback_called = true;
        })
        .SendFileAsMimeData(tmpfile.Name(), "My-File", "MyFile.txt", "text/plain")
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE

STARTCASE(TestUploadMimeNoFile)
{
    restincurl::Client client;
    bool callback_called = false;

    const std::string nofile = "/no-file/12345/if-this-exists-blame-yourself";


    // Send directly as a raw data-stream

    client.Build()->PostMime("http://localhost:3001/upload_raw/")
        .Header("X-Client", "restincurl")
        .Header("Content-Type", "application/octet-stream")
        .Header("X-Origin-File-Name", nofile)
        .WithCompletion([&](const Result& result) {
            EXPECT(result.curl_code != CURLE_OK);
            callback_called = true;
        })
        .SendFileAsMimeData(nofile, "My-File", "MyFile.txt", "text/plain")
        .Execute();

#if RESTINCURL_ENABLE_ASYNC
    client.CloseWhenFinished();
    client.WaitForFinish();
#endif
    EXPECT(callback_called);
} ENDCASE


}; //lest

int main( int argc, char * argv[] )
{
    RESTINCURL_LOG("Running tests in thread " << std::this_thread::get_id());
    return lest::run( specification, argc, argv );
}
