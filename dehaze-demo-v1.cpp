#include <iostream>
#include <cstdio>
#include <unistd.h>

//#include <OpenCVInterfaces.h>
#include <HazyPixels.h>
#include <HazyPixelsSave.h>

using namespace std;

int main(int argc, char *argv[]){
	int opt;
	string file_name;
	bool show_info = false;
	int patch_size = 3;
	int r = 30;
	double eps = 1e-3;
	int file_type = 1;

	while((opt = getopt(argc, argv, "f:iw:r:e:t:")) != -1){
		switch(opt){
			case 'f':
				file_name = string(optarg);
				break;
			case 'i':
				show_info = true;
				break;
			case 'w':
				patch_size = atoi(optarg);
				break;
			case 'r':
				r = atoi(optarg);
				break;
			case 'e':
			{
				int times = atoi(optarg); eps = 1;
				for(int i = 0; i < times; i ++)
					eps /= 10;
				break;
			}
			case 't':
				file_type = atoi(optarg);
				break;
			case 'h':
				cout << "Usage: -f [file_name] -t [file_type: 1-image/ 2-video]" << endl;
				break;
			default:
				return 1;
		}
	}

	hazy_pixels hpixels(file_name.c_str());
	hpixels.pixelsPrintImageInfo();
	hpixels.pixelsSaveOriginalImage(file_name.c_str());

	return 0;
}
