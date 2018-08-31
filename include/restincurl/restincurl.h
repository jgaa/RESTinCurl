#pragma once

#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <functional>
#include <chrono>
#include <vector>
#include <deque>
#include <iterator>
#include <iostream>

#include <assert.h>

#include <curl/curl.h>
#include <curl/easy.h>

/*
 * easy init: easyhandle = curl_easy_init();
 * curl_easy_setopt(handle, CURLOPT_URL, "http://domain.com/");
 * write callback:  size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp); (return size)
 * curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
 *  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &internal_struct);  (pointer to userp)
 * execute: success = curl_easy_perform(easyhandle);
 *  curl_easy_cleanup
 *
 *  read cb:  size_t function(char *bufptr, size_t size, size_t nitems, void *userp);
 *  curl_easy_setopt(easyhandle, CURLOPT_READFUNCTION, read_function);
 *  curl_easy_setopt(easyhandle, CURLOPT_READDATA, &filedata);
 *  Tell libcurl that we want to upload:
 *  curl_easy_setopt(easyhandle, CURLOPT_UPLOAD, 1L);
 *
 * auth:
 *  curl_easy_setopt(easyhandle, CURLOPT_USERPWD, "myname:thesecret");
 *  curl_easy_setopt(easyhandle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
 *
 *  CURLOPT_ERRORBUFFER to point libcurl to a buffer of yours where it'll store a human readable error message as well
 *  set the CURLOPT_VERBOSE option to 1. It'll cause the library to spew out the entire protocol details it sends
 *  Include headers in the normal body output with CURLOPT_HEADER set 1
 *
 * progress:
 * Switch on the progress meter by, oddly enough, setting CURLOPT_NOPROGRESS to zero. This option is set to 1 by default.
 * using CURLOPT_PROGRESSFUNCTION.
 * int progress_callback(void *clientp,
 *                      double dltotal,
 *                      double dlnow,
 *                      double ultotal,
 *                      double ulnow);
 *
 * struct curl_slist *headers=NULL;  init to NULL is important
    headers = curl_slist_append(headers, "Hey-server-hey: how are you?");
    headers = curl_slist_append(headers, "X-silly-content: yes");

    pass our list of custom made headers
    curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers);

    delete header: headers = curl_slist_append(headers, "Accept:");

     curl_slist_free_all(headers);  free the header list

     By making sure a request uses the custom header "Transfer-Encoding: chunked" when doing a non-GET HTTP operation, libcurl will switch over to "chunked" upload
 */


namespace restincurl {

    enum class RequestType { GET, PUT, POST, HEAD, DELETE, PATCH, OPTIONS, INVALID };
    using completion_fn_t = std::function<void (CURLcode result)>;

    class Exception : public std::runtime_error {
    public:
        Exception(const std::string& msg) : runtime_error(msg) {}
    };

    class CurlException : public Exception {
    public:
        CurlException(const std::string msg, const CURLcode err)
            : Exception(msg + '(' + std::to_string(err) + "): " + curl_easy_strerror(err))
            , err_{err}
            {}

        CURLcode getErrorCode() const noexcept { return err_; }

    private:
        const CURLcode err_;
    };

    class EasyHandle {
    public:
        using ptr_t = std::unique_ptr<EasyHandle>;
        using handle_t = decltype(curl_easy_init());

        EasyHandle()
        {
        }

        ~EasyHandle() {
            if (handle_) {
                curl_easy_cleanup(handle_);
            }
        }

        operator handle_t () { return handle_; }

    private:
        handle_t handle_ = curl_easy_init();
    };

    class Options {
    public:
        Options(EasyHandle& eh) : eh_{eh} {}

        template <typename T>
        Options& Set(const CURLoption& opt, const T& value) {
            const auto ret = curl_easy_setopt(eh_, opt, value);
            if (ret) {
                throw CurlException(
                    std::string("Setting option ") + std::to_string(opt), ret);
            }
            return *this;
        }

        Options& Set(const CURLoption& opt, const std::string& value) {
            return Set(opt, value.c_str());
        }

    private:
        EasyHandle& eh_;
    };

    template <typename T>
    struct InDataHandler {
        InDataHandler(T& data) : data_{data} {}

        static size_t write_callback(char *ptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata);
            InDataHandler *self = reinterpret_cast<InDataHandler *>(userdata);
            const auto bytes = size * nitems;
            if (bytes > 0) {
                std::copy(ptr, ptr + bytes, std::back_inserter(self->data_));
            }
            return bytes;
        }

        T& data_;
    };

    template <typename T>
    struct OutDataHandler {
        OutDataHandler() = default;
        OutDataHandler(const T& v) : data_{v} {}
        OutDataHandler(T&& v) : data_{std::move(v)} {}

        static size_t read_callback(char *bufptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata);
            OutDataHandler *self = reinterpret_cast<OutDataHandler *>(userdata);
            const auto bytes = size * nitems;
            auto out_bytes= std::min<size_t>(bytes, (self->data_.size() - self->sendt_bytes_));
            std::copy(self->data_.cbegin() + self->sendt_bytes_,
                      self->data_.cbegin() + out_bytes,
                      bufptr);
            self->sendt_bytes_ += out_bytes;
            return out_bytes;
        }

        T data_;
        size_t sendt_bytes_ = 0;
    };

    class Request {
    public:

        Request()
        : eh_{std::make_unique<EasyHandle>()}
        {
        }

        Request(EasyHandle::ptr_t&& eh)
        : eh_{std::move(eh)}
        {
        }

        ~Request() {
        }

        void Prepare(const RequestType rq) {
            request_type_ = rq;;
            SetRequestType();
        }

        void Execute(completion_fn_t fn) {
            const auto result = curl_easy_perform(*eh_);
            if (fn) {
                fn(result);
            }
        }

        EasyHandle& GetEasyHandle() noexcept { assert(eh_); return *eh_; }
        RequestType GetRequestType() noexcept { return request_type_; }

    private:
        void SetRequestType() {
            switch(request_type_) {
                case RequestType::GET:
                    curl_easy_setopt(*eh_, CURLOPT_HTTPGET, 1L);
                    break;
                case RequestType::PUT:
                    curl_easy_setopt(*eh_, CURLOPT_PUT, 1L);
                    break;
                case RequestType::POST:
                    curl_easy_setopt(*eh_, CURLOPT_POST, 1L);
                    break;
                case RequestType::HEAD:
                    curl_easy_setopt(*eh_, CURLOPT_NOBODY, 1L);
                    break;
                case RequestType::OPTIONS:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    break;
                case RequestType::PATCH:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "PATCH");
                    break;
                case RequestType::DELETE:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "DELETE");
                    break;
                default:
                    throw Exception("Unsupported request type" + std::to_string(static_cast<int>(request_type_)));
            }
        }

        EasyHandle::ptr_t eh_;
        RequestType request_type_ = RequestType::INVALID;
    };

    // Convenience interface
    // We can use different containers for default data handling, but for
    // json, strings are usually OK. We can still call SendData<>() and StoreData<>()
    // with different template parameters, or even set our own Curl compatible
    // read / write handlers.
    template <typename inT = std::string, typename outT = std::string>
    class RequestBuilder {
        // noop handler for incoming data
        static size_t write_callback(char *ptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata == nullptr);
            const auto bytes = size * nitems;
            return bytes;
        }
    public:
        using ptr_t = std::unique_ptr<RequestBuilder>;
        RequestBuilder()
        : request_{std::make_unique<Request>()}
        , options_{std::make_unique<Options>(request_->GetEasyHandle())}
        {}

        ~RequestBuilder() {
            if (headers_) {
                curl_slist_free_all(headers_);
            }
        }

    protected:
        RequestBuilder& Prepare(RequestType rt, const std::string& url) {
            assert(request_type_ == RequestType::INVALID);
            assert(!is_built_);
            request_type_  = rt;
            url_ = url;
            return *this;
        }

    public:
        RequestBuilder& Get(const std::string& url) {
            return Prepare(RequestType::GET, url);
        }

        RequestBuilder& Head(const std::string& url) {
            return Prepare(RequestType::HEAD, url);
        }

        RequestBuilder& Post(const std::string& url) {
            return Prepare(RequestType::POST, url);
        }

        RequestBuilder& Put(const std::string& url) {
            return Prepare(RequestType::PUT, url);
        }

        RequestBuilder& Delete(const std::string& url) {
            return Prepare(RequestType::DELETE, url);
        }

        RequestBuilder& Header(const char *value) {
            assert(value);
            assert(!is_built_);
            headers_ = curl_slist_append(headers_, value);
            return *this;
        }

        RequestBuilder& Header(const std::string& name,
                               const std::string& value) {
            const auto v = name + ": " + value;
            return Header(v.c_str());
        }

        RequestBuilder& WithJson() {
            return Header("Content-type: Application/json; charset=utf-8");
        }

        RequestBuilder& AcceptJson() {
            return Header("Accept: Application/json");
        }

        template <typename T>
        RequestBuilder& Option(const CURLoption& opt, const T& value) {
            assert(!is_built_);
            options_->Set(opt, value);
            return *this;
        }

        // Outgoing data
        template <typename T>
        RequestBuilder& SendData(OutDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_READFUNCTION, dh.read_callback);
            options_->Set(CURLOPT_READDATA, &dh);
            have_data_out_ = true;
        }

        RequestBuilder& SendData(inT data) {
            assert(!is_built_);
            default_out_handler_ = std::make_unique<OutDataHandler<outT>>(std::move(data));
            return SendData(*default_out_handler_);
        }

        // Incoming data
        template <typename T>
        RequestBuilder& StoreData(InDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_WRITEFUNCTION, dh.write_callback);
            options_->Set(CURLOPT_WRITEDATA, &dh);
            have_data_in_ = true;
        }

        // Store data in the provided string. It must exist until the transfer is complete.
        RequestBuilder& StoreData(inT& data) {
            assert(!is_built_);
            default_in_handler_ = std::make_unique<InDataHandler<inT>>(data);
            return StoreData(*default_in_handler_);
        }

        RequestBuilder& WithCompletion(completion_fn_t fn) {
            assert(!is_built_);
            completion_ = std::move(fn);
            return *this;
        }

        // Set Curl compatible read handler. You will probably don't need this.
        RequestBuilder& SetReadHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_READFUNCTION,handler);
            options_->Set(CURLOPT_READDATA, userdata);
            have_data_out_ = true;
        }

        // Set Curl compatible write handler. You will probably don't need this.
        RequestBuilder& SetWriteHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_WRITEFUNCTION,handler);
            options_->Set(CURLOPT_WRITEDATA, userdata);
            have_data_in_ = true;
        }

        void Build() {
            if (!is_built_) {
                // Set up data handlers
                if (!have_data_in_) {
                    options_->Set(CURLOPT_WRITEFUNCTION, write_callback);
                }

                if (have_data_out_) {
                    options_->Set(CURLOPT_UPLOAD, 1L);
                }

                // Set headers
                if (headers_) {
                    options_->Set(CURLOPT_HTTPHEADER, headers_);
                }

                // TODO: Set up timeout
                // TODO: Set up progress callback
                // TODO: Prepare the final url (we want nice request arguments)
                options_->Set(CURLOPT_URL, url_);

                // Prepare request
                request_->Prepare(request_type_);
                is_built_ = true;
            }
        }

        void Execute() {
            Build();

            // Execute curl request
            request_->Execute(std::move(completion_));
        }

    private:
        std::unique_ptr<Request> request_;
        std::unique_ptr<Options> options_;
        std::string url_;
        RequestType request_type_ = RequestType::INVALID;
        curl_slist *headers_ = nullptr;
        bool have_data_in_ = false;
        bool have_data_out_ = false;
        bool is_built_ = false;
        std::unique_ptr<OutDataHandler<outT>> default_out_handler_;
        std::unique_ptr<InDataHandler<inT>> default_in_handler_;
        completion_fn_t completion_;
    };

    class Client {
    public:
        Client() {
        }

        virtual ~Client() {

        }

        template<typename inT = std::string, typename outT = std::string>
        std::unique_ptr<RequestBuilder<inT, outT>> Build() {
            return std::make_unique<RequestBuilder<inT, outT>>();
        }
    };


} // namespace

