// from https://vulkan-tutorial.com/

#include "SVKConfig.h"
#include "SVKApp.h"

int main(int argc, char** argv) {
    SVKConfig config(argc, argv);
    SVKApp app(config);

    try {
        app.Initialize();
        app.Run();
        app.Cleanup();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
