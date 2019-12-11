#include "ValkanCudaApp.h"

int main(int argc, char** argv) {
	execution_path = argv[0];
	std::string image_filename = "lenaRGB.ppm";

	if (checkCmdLineFlag(argc, (const char**)argv, "file")) {
		getCmdLineArgumentString(argc, (const char**)argv, "file",
			(char**)&image_filename);
	}

	vulkanImageCUDA app(1920,1080);

	try {
		// This app only works on ppm images
		//app.loadImageData(image_filename);
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
