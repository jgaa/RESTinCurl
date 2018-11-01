#pragma once

/*
    MIT License

    Copyright (c) 2018 Jarle Aase

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    On Github: https://github.com/jgaa/RESTinCurl
*/

#include <algorithm>
#include <atomic>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <assert.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

// Max concurrent connections
#ifndef RESTINCURL_MAX_CONNECTIONS
#   define RESTINCURL_MAX_CONNECTIONS 32L
#endif

#ifndef RESTINCURL_ENABLE_ASYNC
#   define RESTINCURL_ENABLE_ASYNC 1
#endif

// How long to wait for the next request before the idle worker-thread is stopped.
// This will delete curl's connection-cache and cause a new thread to be created
// and new connections to be made if there are new requests at at later time.
#ifndef RESTINCURL_IDLE_TIMEOUT_SEC
#   define RESTINCURL_IDLE_TIMEOUT_SEC 60
#endif

#ifndef RESTINCURL_LOG_VERBOSE_ENABLE
#   define RESTINCURL_LOG_VERBOSE_ENABLE 0
#endif

#if defined(_LOGFAULT_H) && !defined (RESTINCURL_LOG) && !defined (RESTINCURL_LOG_TRACE)
#   define RESTINCURL_LOG(msg)    LFLOG_DEBUG << "restincurl: " << msg
#   if RESTINCURL_LOG_VERBOSE_ENABLE
#       define RESTINCURL_LOG_TRACE(msg) LFLOG_IFALL_TRACE("restincurl: " << msg)
#   endif // RESTINCURL_LOG_VERBOSE_ENABLE
#endif

#if defined(RESTINCURL_USE_SYSLOG) || defined(RESTINCURL_USE_ANDROID_NDK_LOG)
#   ifdef RESTINCURL_USE_SYSLOG
#       include <syslog.h>
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
#       include <android/log.h>
#   endif
#   include <sstream>
#   define RESTINCURL_LOG(msg) ::restincurl::Log(restincurl::LogLevel::DEBUG).Line() << msg
#endif

#ifndef RESTINCURL_ENABLE_DEFAULT_LOGGER
#   define RESTINCURL_ENABLE_DEFAULT_LOGGER 0
#endif

#ifndef RESTINCURL_LOG
#   if RESTINCURL_ENABLE_DEFAULT_LOGGER
#       define RESTINCURL_LOG(msg) std::clog << msg << std::endl
#   else
#       define RESTINCURL_LOG(msg)
#   endif
#endif

#ifndef RESTINCURL_LOG_TRACE
#   if RESTINCURL_LOG_VERBOSE_ENABLE
#       define RESTINCURL_LOG_TRACE(msg) RESTINCURL_LOG(msg)
#   else
#       define RESTINCURL_LOG_TRACE(msg)
#   endif
#endif

namespace restincurl {

#if defined(RESTINCURL_USE_SYSLOG) || defined(RESTINCURL_USE_ANDROID_NDK_LOG)
    enum class LogLevel { DEBUG };

    class Log {
    public:
        Log(const LogLevel level) : level_{level} {}
        ~Log() {
#   ifdef RESTINCURL_USE_SYSLOG
            static const std::array<int, 1> syslog_priority = { LOG_DEBUG };
            static std::once_flag syslog_opened;
            std::call_once(syslog_opened, [] {
                openlog(nullptr, 0, LOG_USER);
            });
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
            static const std::array<int, 1> android_priority = { ANDROID_LOG_DEBUG };
#   endif
            const auto msg = out_.str();

#   ifdef RESTINCURL_USE_SYSLOG
            syslog(syslog_priority.at(static_cast<int>(level_)), "%s", msg.c_str());
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
            __android_log_write(android_priority.at(static_cast<int>(level_)),
                                "restincurl", msg.c_str());
#   endif
        }

        std::ostringstream& Line() { return out_; }

private:
        const LogLevel level_;
        std::ostringstream out_;
    };
#endif



    using lock_t = std::lock_guard<std::mutex>;

    struct Result {
        Result() = default;
        Result(const CURLcode& code) {
            curl_code = code;
            msg = curl_easy_strerror(code);
        }

        CURLcode curl_code = {};
        long http_response_code = {};
        std::string msg;
    };

    enum class RequestType { GET, PUT, POST, HEAD, DELETE, PATCH, OPTIONS, INVALID };
    using completion_fn_t = std::function<void (const Result& result)>;

    class Exception : public std::runtime_error {
    public:
        Exception(const std::string& msg) : runtime_error(msg) {}
    };

    class SystemException : public Exception {
    public:
        SystemException(const std::string& msg, const int e) : Exception(msg + " " + strerror(e)), err_{e} {}

        int getErrorCode() const noexcept { return err_; }

    private:
        const int err_;
    };

    class CurlException : public Exception {
    public:
        CurlException(const std::string msg, const CURLcode err)
            : Exception(msg + '(' + std::to_string(err) + "): " + curl_easy_strerror(err))
            , err_{err}
            {}

         CurlException(const std::string msg, const CURLMcode err)
            : Exception(msg + '(' + std::to_string(err) + "): " + curl_multi_strerror(err))
            , err_{err}
            {}

        int getErrorCode() const noexcept { return err_; }

    private:
        const int err_;
    };

    class EasyHandle {
    public:
        using ptr_t = std::unique_ptr<EasyHandle>;
        using handle_t = decltype(curl_easy_init());

        EasyHandle() {
            RESTINCURL_LOG("EasyHandle created: " << handle_);
        }

        ~EasyHandle() {
            Close();
        }

        void Close() {
            if (handle_) {
                RESTINCURL_LOG("Cleaning easy-handle " << handle_);
                curl_easy_cleanup(handle_);
                handle_ = nullptr;
            }
        }

        operator handle_t () const noexcept { return handle_; }

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

    struct DataHandlerBase {
        virtual ~DataHandlerBase() = default;
    };

    template <typename T>
    struct InDataHandler : public DataHandlerBase{
        InDataHandler(T& data) : data_{data} {
            RESTINCURL_LOG_TRACE("InDataHandler address: " << this);
        }

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
    struct OutDataHandler : public DataHandlerBase {
        OutDataHandler() = default;
        OutDataHandler(const T& v) : data_{v} {}
        OutDataHandler(T&& v) : data_{std::move(v)} {}

        static size_t read_callback(char *bufptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata);
            OutDataHandler *self = reinterpret_cast<OutDataHandler *>(userdata);
            const auto bytes = size * nitems;
            auto out_bytes = std::min<size_t>(bytes, (self->data_.size() - self->sendt_bytes_));
            std::copy(self->data_.cbegin() + self->sendt_bytes_,
                      self->data_.cbegin() + (self->sendt_bytes_ + out_bytes),
                      bufptr);
            self->sendt_bytes_ += out_bytes;

            RESTINCURL_LOG_TRACE("Sent " << out_bytes << " of total " << self->data_.size() << " bytes.");
            return out_bytes;
        }

        T data_;
        size_t sendt_bytes_ = 0;
    };

    class Request {
    public:
        using ptr_t = std::unique_ptr<Request>;

        Request()
        : eh_{std::make_unique<EasyHandle>()}
        {
        }

        Request(EasyHandle::ptr_t&& eh)
        : eh_{std::move(eh)}
        {
        }

        ~Request() {
            if (headers_) {
                curl_slist_free_all(headers_);
            }
        }

        void Prepare(const RequestType rq, completion_fn_t completion) {
            request_type_ = rq;
            SetRequestType();
            completion_ = std::move(completion);
        }

        // Synchronous execution.
        void Execute() {
            const auto result = curl_easy_perform(*eh_);
            CallCompletion(result);
        }

        void Complete(CURLcode cc, const CURLMSG& /*msg*/) {
            CallCompletion(cc);
        }

        EasyHandle& GetEasyHandle() noexcept { assert(eh_); return *eh_; }
        RequestType GetRequestType() noexcept { return request_type_; }

        void SetDefaultInHandler(std::unique_ptr<DataHandlerBase> ptr) {
            default_in_handler_ = std::move(ptr);
        }

        void SetDefaultOutHandler(std::unique_ptr<DataHandlerBase> ptr) {
            default_out_handler_ = std::move(ptr);
        }

        using headers_t = curl_slist *;
        headers_t& GetHeaders() {
            return headers_;
        }

    private:
        void CallCompletion(CURLcode cc) {
            Result result(cc);

            curl_easy_getinfo (*eh_, CURLINFO_RESPONSE_CODE,
                               &result.http_response_code);
            RESTINCURL_LOG("Complete: http code: " << result.http_response_code);
            if (completion_) {
                completion_(result);
            }
        }

        void SetRequestType() {
            switch(request_type_) {
                case RequestType::GET:
                    curl_easy_setopt(*eh_, CURLOPT_HTTPGET, 1L);
                    break;
                case RequestType::PUT:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
                    curl_easy_setopt(*eh_, CURLOPT_UPLOAD, 1L);
                    break;
                case RequestType::POST:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
                    curl_easy_setopt(*eh_, CURLOPT_UPLOAD, 0L);
                    curl_easy_setopt(*eh_, CURLOPT_POST, 1L);
                    break;
                case RequestType::HEAD:
                    curl_easy_setopt(*eh_, CURLOPT_NOBODY, 1L);
                    break;
                case RequestType::OPTIONS:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    break;
                case RequestType::PATCH:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
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
        completion_fn_t completion_;
        std::unique_ptr<DataHandlerBase> default_out_handler_;
        std::unique_ptr<DataHandlerBase> default_in_handler_;
        headers_t headers_ = nullptr;
    };

#if RESTINCURL_ENABLE_ASYNC

    class Signaler {
        enum FdUsage { FD_READ = 0, FD_WRITE = 1};

    public:
        using pipefd_t = std::array<int, 2>;

        Signaler() {
            auto status = pipe(pipefd_.data());
            if (status) {
                throw SystemException("pipe", status);
            }
            for(auto fd : pipefd_) {
                int flags = 0;
                if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
                    flags = 0;
                fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            }
        }

        ~Signaler() {
            for(auto fd : pipefd_) {
                close(fd);
            }
        }

        void Signal() {
            char byte = {};
            RESTINCURL_LOG_TRACE("Signal: Signaling!");
            if (write(pipefd_[FD_WRITE], &byte, 1) != 1) {
                throw SystemException("write pipe", errno);
            }
        }

        int GetReadFd() { return pipefd_[FD_READ]; }

        bool WasSignalled() {
            bool rval = false;
            char byte = {};
            while(read(pipefd_[FD_READ], &byte, 1) > 0) {
                RESTINCURL_LOG_TRACE("Signal: Was signalled");
                rval = true;
            }

            return rval;
        }

    private:
        pipefd_t pipefd_;
    };

    class Worker {
        class WorkerThread {
        public:
            WorkerThread(std::function<void ()> && fn)
            : thread_{std::move(fn)} {}

            ~WorkerThread() {
                if (thread_.get_id() == std::this_thread::get_id()) {
                    // Allow the thread to finish exiting the lambda
                    thread_.detach();
                }
            }

            void Join() const {
                std::call_once(joined_, [this] {
                    const_cast<WorkerThread *>(this)->thread_.join();
                });
            }

            bool Joinable() const {
                return thread_.joinable();
            }

            operator std::thread& () { return thread_; }

        private:
            std::thread thread_;
            mutable std::once_flag joined_;
        };

    public:
        Worker() = default;

        ~Worker() {
            if (thread_ && thread_->Joinable()) {
                Close();
                Join();
            }
        }

        void PrepareThread() {
            // Must only be called when the mutex_ is acquired!
            assert(!mutex_.try_lock());
            if (abort_ || done_) {
                return;
            }
            if (!thread_) {
                thread_ = std::make_shared<WorkerThread>([&] {
                    try {
                        RESTINCURL_LOG("Starting thread " << std::this_thread::get_id());
                        Init();
                        Run();
                        Clean();
                    } catch (const std::exception& ex) {
                        RESTINCURL_LOG("Worker: " << ex.what());
                    }
                    RESTINCURL_LOG("Exiting thread " << std::this_thread::get_id());

                    lock_t lock(mutex_);
                    thread_.reset();
                });
            }
            assert(!abort_);
            assert(!done_);
        }

        static std::unique_ptr<Worker> Create() {
            return std::make_unique<Worker>();
        }

        void Enqueue(Request::ptr_t req) {
            RESTINCURL_LOG_TRACE("Queuing request ");
            lock_t lock(mutex_);
            PrepareThread();
            queue_.push_back(std::move(req));
            Signal();
        }

        void Join() const {
            decltype(thread_) thd;

            {
                lock_t lock(mutex_);
                if (thread_ && thread_->Joinable()) {
                    thd = thread_;
                }
            }

            if (thd) {
                thd->Join();
            }
        }

        // Let the current transfers complete, then quit
        void CloseWhenFinished() {
            {
                lock_t lock(mutex_);
                close_pending_ = true;
            }
            Signal();
        }

        // Shut down now. Abort all transfers
        void Close() {
            {
                lock_t lock(mutex_);
                abort_ = true;
            }
            Signal();
        }

        // Check if the run loop has finished.
        bool IsDone() const {
            lock_t lock(mutex_);
            return done_;
        }

        bool HaveThread() const noexcept {
            lock_t lock(mutex_);
            if (thread_) {
                return true;
            }
            return false;
        }

        size_t GetNumActiveRequests() const {
            lock_t lock(mutex_);
            return ongoing_.size();
        }

    private:
        void Signal() {
            signal_.Signal();
        }

        void Dequeue() {
            decltype(queue_) tmp;

            {
                lock_t lock(mutex_);
                if ((queue_.size() + ongoing_.size()) <= RESTINCURL_MAX_CONNECTIONS) {
                    tmp = std::move(queue_);
                    pending_entries_in_queue_ = false;
                } else {
                    auto remains = std::min<size_t>(RESTINCURL_MAX_CONNECTIONS - ongoing_.size(), queue_.size());
                    if (remains) {
                        auto it = queue_.begin();
                        RESTINCURL_LOG_TRACE("Adding only " << remains << " of " << queue_.size()
                            << " requests from queue: << RESTINCURL_MAX_CONNECTIONS=" << RESTINCURL_MAX_CONNECTIONS);
                        while(remains--) {
                            assert(it != queue_.end());
                            tmp.push_back(std::move(*it));
                            ++it;
                        }
                        queue_.erase(queue_.begin(), it);
                    } else {
                        assert(ongoing_.size() == RESTINCURL_MAX_CONNECTIONS);
                        RESTINCURL_LOG_TRACE("Adding no entries from queue: RESTINCURL_MAX_CONNECTIONS="
                            << RESTINCURL_MAX_CONNECTIONS);
                    }
                    pending_entries_in_queue_ = true;
                }
            }

            for(auto& req: tmp) {
                assert(req);
                const auto& eh = req->GetEasyHandle();
                RESTINCURL_LOG_TRACE("Adding request: " << eh);
                ongoing_[eh] = std::move(req);
                const auto mc = curl_multi_add_handle(handle_, eh);
                if (mc != CURLM_OK) {
                    throw CurlException("curl_multi_add_handle", mc);
                }
            }
        }

        void Init() {
            if ((handle_ = curl_multi_init()) == nullptr) {
                throw std::runtime_error("curl_multi_init() failed");
            }

            curl_multi_setopt(handle_, CURLMOPT_MAXCONNECTS, RESTINCURL_MAX_CONNECTIONS);
        }

        void Clean() {
            if (handle_) {
                RESTINCURL_LOG_TRACE("Calling curl_multi_cleanup: " << handle_);
                curl_multi_cleanup(handle_);
                handle_ = nullptr;
            }
        }

        bool EvaluateState(const bool transfersRunning, const bool doDequeue) const noexcept {
            lock_t lock(mutex_);

            RESTINCURL_LOG_TRACE("Run loop: transfers_running=" << transfersRunning
                << ", do_dequeue=" << doDequeue
                << ", close_pending_=" << close_pending_);

            return !abort_ && (transfersRunning || !close_pending_);
        }

        auto GetNextTimeout() const noexcept {
            return std::chrono::steady_clock::now()
                + std::chrono::seconds(RESTINCURL_IDLE_TIMEOUT_SEC);
        }

        void Run() {
            int transfers_running = -1;
            fd_set fdread = {};
            fd_set fdwrite = {};
            fd_set fdexcep = {};
            bool do_dequeue = true;
            auto timeout = GetNextTimeout();

            while (EvaluateState(transfers_running, do_dequeue)) {

                if (do_dequeue) {
                    Dequeue();
                    do_dequeue = false;
                }

                /* timeout or readable/writable sockets */
                const bool initial_ideling = transfers_running == -1;
                curl_multi_perform(handle_, &transfers_running);
                if ((transfers_running == 0) && initial_ideling) {
                    transfers_running = -1; // Let's ignore close_pending_ until we have seen a request
                }

                // Shut down the thread if we have been idling too long
                if (transfers_running <= 0) {
                    if (timeout < std::chrono::steady_clock::now()) {
                        RESTINCURL_LOG("Idle timeout. Will shut down the worker-thread.");
                        break;
                    }
                } else {
                    timeout = GetNextTimeout();
                }

                int numLeft = {};
                while (auto m = curl_multi_info_read(handle_, &numLeft)) {
                    assert(m);
                    auto it = ongoing_.find(m->easy_handle);
                    if (it != ongoing_.end()) {
                        RESTINCURL_LOG("Finishing request with easy-handle: "
                            << (EasyHandle::handle_t)it->second->GetEasyHandle()
                            << "; with result: " << m->data.result << " expl: '" << curl_easy_strerror(m->data.result)
                            << "'; with msg: " << m->msg);

                        try {
                            it->second->Complete(m->data.result, m->msg);
                        } catch(const std::exception& ex) {
                            RESTINCURL_LOG("Complete threw: " << ex.what());
                        }
                        if (m->msg == CURLMSG_DONE) {
                            curl_multi_remove_handle(handle_, m->easy_handle);
                        }
                        it->second->GetEasyHandle().Close();
                        ongoing_.erase(it);
                    } else {
                        RESTINCURL_LOG("Failed to find easy_handle in ongoing!");
                        assert(false);
                    }
                }

                {
                    lock_t lock(mutex_);
                    // Avoid using select() as a timer when we need to exit anyway
                    if (abort_ || (!transfers_running && close_pending_)) {
                        break;
                    }
                }

                auto next_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout - std::chrono::steady_clock::now());
                long sleep_duration = std::max<long>(1, next_timeout.count());
                /* extract sleep_duration value */


                FD_ZERO(&fdread);
                FD_ZERO(&fdwrite);
                FD_ZERO(&fdexcep);

                int maxfd = -1;
                if (transfers_running > 0) {
                    curl_multi_timeout(handle_, &sleep_duration);
                    if (sleep_duration < 0) {
                        sleep_duration = 1000;
                    }

                    /* get file descriptors from the transfers */
                    const auto mc = curl_multi_fdset(handle_, &fdread, &fdwrite, &fdexcep, &maxfd);
                    RESTINCURL_LOG_TRACE("maxfd: " << maxfd);
                    if (mc != CURLM_OK) {
                        throw CurlException("curl_multi_fdset", mc);
                    }

                    if (maxfd == -1) {
                        // Curl want's us to revisit soon
                        sleep_duration = 50;
                    }
                } // active transfers

                struct timeval tv = {};
                tv.tv_sec = sleep_duration / 1000;
                tv.tv_usec = (sleep_duration % 1000) * 1000;

                const auto signalfd = signal_.GetReadFd();

                RESTINCURL_LOG_TRACE("Calling select() with timeout of "
                    << sleep_duration
                    << " ms. Next timeout in " << next_timeout.count() << " ms. "
                    << transfers_running << " active transfers.");

                FD_SET(signalfd, &fdread);
                maxfd = std::max(signalfd,  maxfd) + 1;

                const auto rval = select(maxfd, &fdread, &fdwrite, &fdexcep, &tv);
                RESTINCURL_LOG_TRACE("select(" << maxfd << ") returned: " << rval);

                if (rval > 0) {
                    if (FD_ISSET(signalfd, &fdread)) {
                        RESTINCURL_LOG_TRACE("FD_ISSET was true: ");
                        do_dequeue = signal_.WasSignalled();
                    }

                }
                if (pending_entries_in_queue_) {
                    do_dequeue = true;
                }
            } // loop


            lock_t lock(mutex_);
            if (close_pending_ || abort_) {
                done_ = true;
            }
        }

        bool close_pending_ {false};
        bool abort_ {false};
        bool done_ {false};
        bool pending_entries_in_queue_ = false;
        decltype(curl_multi_init()) handle_ = {};
        mutable std::mutex mutex_;
        std::shared_ptr<WorkerThread> thread_;
        std::deque<Request::ptr_t> queue_;
        std::map<EasyHandle::handle_t, Request::ptr_t> ongoing_;
        Signaler signal_;
    };
#endif // RESTINCURL_ENABLE_ASYNC

    // Convenience interface
    // We can use different containers for default data handling, but for
    // json, strings are usually OK. We can still call SendData<>() and StoreData<>()
    // with different template parameters, or even set our own Curl compatible
    // read / write handlers.
    class RequestBuilder {
        // noop handler for incoming data
        static size_t write_callback(char *ptr, size_t size, size_t nitems, void *userdata) {
            const auto bytes = size * nitems;
            return bytes;
        }

        static int debug_callback(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp) {

            std::string msg;
            switch(type) {
            case CURLINFO_TEXT:
                msg = "==> Info: ";
                break;
            case CURLINFO_HEADER_OUT:
                msg =  "=> Send header: ";
                break;
            case CURLINFO_DATA_OUT:
                msg = "=> Send data: ";
                break;
            case CURLINFO_SSL_DATA_OUT:
                msg = "=> Send SSL data: ";
                break;
            case CURLINFO_HEADER_IN:
                msg = "<= Recv header: ";
                break;
            case CURLINFO_DATA_IN:
                msg = "<= Recv data: ";
                break;
            case CURLINFO_SSL_DATA_IN:
                msg = "<= Recv SSL data: ";
                break;
            case CURLINFO_END: // Don't seems to be used
                msg = "<= End: ";
                break;
            }

            std::copy(data, data + size, std::back_inserter(msg));
            RESTINCURL_LOG(handle << " " << msg);
            return 0;
        }

    public:
        using ptr_t = std::unique_ptr<RequestBuilder>;
        RequestBuilder(
#if RESTINCURL_ENABLE_ASYNC
            Worker& worker
#endif
        )
        : request_{std::make_unique<Request>()}
        , options_{std::make_unique<class Options>(request_->GetEasyHandle())}
#if RESTINCURL_ENABLE_ASYNC
        , worker_(worker)
#endif
        {}

        ~RequestBuilder() {
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

        RequestBuilder& Patch(const std::string& url) {
            return Prepare(RequestType::PATCH, url);
        }

        RequestBuilder& Delete(const std::string& url) {
            return Prepare(RequestType::DELETE, url);
        }

        RequestBuilder& Options(const std::string& url) {
            return Prepare(RequestType::OPTIONS, url);
        }

        RequestBuilder& Header(const char *value) {
            assert(value);
            assert(!is_built_);
            request_->GetHeaders() = curl_slist_append(request_->GetHeaders(), value);
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

        RequestBuilder& Trace(bool enable = true) {
            if (enable) {
                Option(CURLOPT_DEBUGFUNCTION, debug_callback);
                Option(CURLOPT_VERBOSE, 1L);
            }
            return *this;
        }

        // Set to -1 to disable
        RequestBuilder& RequestTimeout(const long timeout) {
            request_timeout_ = timeout;
            return *this;
        }

        // Set to -1 to disable
        RequestBuilder& ConnectTimeout(const long timeout) {
            connect_timeout_ = timeout;
            return *this;
        }

        // Outgoing data
        template <typename T>
        RequestBuilder& SendData(OutDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_READFUNCTION, dh.read_callback);
            options_->Set(CURLOPT_READDATA, &dh);
            have_data_out_ = true;
            return *this;
        }

        template <typename T>
        RequestBuilder& SendData(T data) {
            assert(!is_built_);
            auto handler = std::make_unique<OutDataHandler<T>>(std::move(data));
            auto& handler_ref = *handler;
            request_->SetDefaultOutHandler(std::move(handler));
            return SendData(handler_ref);
        }

        // Incoming data
        template <typename T>
        RequestBuilder& StoreData(InDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_WRITEFUNCTION, dh.write_callback);
            options_->Set(CURLOPT_WRITEDATA, &dh);
            have_data_in_ = true;
            return *this;
        }

        // Store data in the provided string. It must exist until the transfer is complete.
        template <typename T>
        RequestBuilder& StoreData(T& data) {
            assert(!is_built_);
            auto handler = std::make_unique<InDataHandler<T>>(data);
            auto& handler_ref = *handler;
            request_->SetDefaultInHandler(std::move(handler));
            return StoreData(handler_ref);
        }

        RequestBuilder& WithCompletion(completion_fn_t fn) {
            assert(!is_built_);
            completion_ = std::move(fn);
            return *this;
        }

        // Set Curl compatible read handler. You will probably don't need this.
        RequestBuilder& SetReadHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_READFUNCTION, handler);
            options_->Set(CURLOPT_READDATA, userdata);
            have_data_out_ = true;
            return *this;
        }

        // Set Curl compatible write handler. You will probably don't need this.
        RequestBuilder& SetWriteHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_WRITEFUNCTION,handler);
            options_->Set(CURLOPT_WRITEDATA, userdata);
            have_data_in_ = true;
            return *this;
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

                if (request_timeout_ >= 0) {
                    options_->Set(CURLOPT_TIMEOUT_MS, request_timeout_);
                }

                if (connect_timeout_ >= 0) {
                    options_->Set(CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_);
                }

                // Set headers
                if (request_->GetHeaders()) {
                    options_->Set(CURLOPT_HTTPHEADER, request_->GetHeaders());
                }

                // TODO: Prepare the final url (we want nice, correctly encoded request arguments)
                options_->Set(CURLOPT_URL, url_);
                RESTINCURL_LOG("Preparing connect to: " << url_);

                // Prepare request
                request_->Prepare(request_type_, std::move(completion_));
                is_built_ = true;
            }
        }

        void ExecuteSynchronous() {
            Build();
            request_->Execute();
        }

#if RESTINCURL_ENABLE_ASYNC
        void Execute() {
            Build();
            worker_.Enqueue(std::move(request_));
        }
#endif

    private:
        std::unique_ptr<Request> request_;
        std::unique_ptr<class Options> options_;
        std::string url_;
        RequestType request_type_ = RequestType::INVALID;
        bool have_data_in_ = false;
        bool have_data_out_ = false;
        bool is_built_ = false;
        completion_fn_t completion_;
        long request_timeout_ = 10000L; // 10 seconds
        long connect_timeout_ = 3000L; // 1 second
#if RESTINCURL_ENABLE_ASYNC
        Worker& worker_;
#endif
    };


    class Client {

    public:
        // Set init to false if curl is already initialized by unrelated code
        Client(const bool init = true) {
            if (init) {
                static std::once_flag flag;
                std::call_once(flag, [] {
                    RESTINCURL_LOG("One time initialization of curl.");
                    curl_global_init(CURL_GLOBAL_DEFAULT);
                });
            }
        }

        virtual ~Client() {
#if RESTINCURL_ENABLE_ASYNC
            if (worker_) {
                try {
                    worker_->Close();
                } catch (const std::exception& ex) {
                    RESTINCURL_LOG("~Client(): " << ex.what());
                }
            }
#endif
        }

        std::unique_ptr<RequestBuilder> Build() {
            return std::make_unique<RequestBuilder>(
#if RESTINCURL_ENABLE_ASYNC
                *worker_
#endif
            );
        }

#if RESTINCURL_ENABLE_ASYNC
        void CloseWhenFinished() {
            worker_->CloseWhenFinished();
        }

        void Close() {
            worker_->Close();
        }

        void WaitForFinish() {
            worker_->Join();
        }

        bool HaveWorker() const {
            return worker_->HaveThread();
        }

        size_t GetNumActiveRequests() {
            return worker_->GetNumActiveRequests();
        }
#endif

    private:
#if RESTINCURL_ENABLE_ASYNC
        std::unique_ptr<Worker> worker_ = std::make_unique<Worker>();
#endif
    };


} // namespace

