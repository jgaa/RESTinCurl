#include <iostream>
#include "restincurl/restincurl.h"

int main() {
    std::cout << "Testing RESTinCurl::RESTinCurl target..." << std::endl;
    
    // Test basic functionality
    try {
        restincurl::Client client;
        std::cout << "✓ Client created successfully" << std::endl;
        
        // Test that we can create a builder (this tests the include paths work)
        auto builder = client.Build();
        std::cout << "✓ Builder created successfully" << std::endl;
        
        std::cout << "✓ All target tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
        return 1;
    }
}