#include "FreeImage.h"

FIBITMAP * FIInterfaceGenericLoader(const char * file_name, int flag){
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(file_name, 0);
	if(fif == FIF_UNKNOWN)
		fif = FreeImage_GetFIFFromFilename(file_name);

	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif) ){
		FIBITMAP *bitmap = FreeImage_Load(fif, file_name, 0);
		return bitmap;
	}
	return NULL;
}

void FIInterfaceUnLoader(FIBITMAP *bitmap){
	FreeImage_Unload(bitmap);
}

FIBITMAP * FIInterfaceGenerateBitmapEightBits(unsigned width, unsigned height){
	FIBITMAP *bitmap = FreeImage_Allocate(width, height, 8);
	return bitmap;
}

FIBITMAP * FIInterfaceGenerateBitmapColorBits(unsigned width, unsigned height){
	FIBITMAP *bitmap = FreeImage_Allocate(width, height, 24);
	return bitmap;
}
