
/* Simple application using the library.
 *
 * Just to get an idea about the size of the
 * binary the library produce.
 */

#include "restincurl/restincurl.h"

using namespace std;
using namespace restincurl;

int main( int argc, char * argv[]) {

    restincurl::Client client;
    string data;
    restincurl::InDataHandler<std::string> data_handler(data);

    client.Build()->Get("http://localhost:3001/normal/manyposts")
        .AcceptJson()
        .StoreData(data)
        .Header("X-Client", "restincurl")
        .WithCompletion([&](auto result) {
            clog << "In callback!" << endl;
        })
        .Execute();

    client.CloseWhenFinished();
    client.WaitForFinish();
}
