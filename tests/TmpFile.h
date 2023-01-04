#pragma once

#include <sys/types.h>
#include <unistd.h>


#include <fstream>
#include <sstream>

/*! Generate a temporary file so we have something to send.
 *
 *  Delete the file in the destructor
 *
 *  Using C++14, so no std::filesystem...
 *
 */
struct TmpFile {

    static std::string generateTmpname()
    {
        std::ostringstream o;

        const auto * tmp = getenv("TEMP");

        o << '/' << (tmp ? tmp : "tmp") << "/RestInCurl_upload_test." << getpid()
          << ".fu";

        return o.str();
    }

    TmpFile() {
        std::ofstream file(name_);
        for(int i = 0; i < 1000; i++) {
            file << "This is line #" << i << std::endl;
        }
    }

    ~TmpFile() {
        unlink(name_.c_str());
    }

    std::string Name() {
        return name_;
    }

private:
    std::string name_ = // cant use tmpnam(nullptr) even in test-cases because of ... morons
            generateTmpname();
};

