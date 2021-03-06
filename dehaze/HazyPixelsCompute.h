#include "GeneralMatrix.h"
#include "GuidedImageFilter.h"


int hazy_pixel_cmp(const void* a, const void *b){
	return ((hazy_pixel*)a)->dc_value - ((hazy_pixel*)b)->dc_value;
}

int hazy_pixel_inst_cmp(const void *a, const void *b){
	hazy_pixel *p1 = (hazy_pixel*) a;
	hazy_pixel *p2 = (hazy_pixel*) b;
	int p1_inst =(int)( 0.2126*p1->color_tuple->rgbred + 0.0722*p1->color_tuple->rgbblue + 0.7152*p1->color_tuple->rgbgreen);
	int p2_inst =(int)( 0.2126*p2->color_tuple->rgbred + 0.0722*p2->color_tuple->rgbblue + 0.7152*p2->color_tuple->rgbgreen);
	return p1_inst - p2_inst;
}

/**
 * pixelsBuildValueArray: 
 * use guided filter to change the t_value
 */
void hazy_pixels::pixelsBuildtValueArray(int r, double eps)
{
#ifdef FREE_IMAGE_SUPPORT
	unsigned hei = this->hazy_height, wid = this->hazy_width;
	printf("%d %d\n", hei, wid);
	int arr_size = hei * wid;
	this->t_value_arr = (double *) malloc(sizeof(double) * arr_size);
	double *gray_I_arr = (double *) malloc(sizeof(double) * arr_size);

	//Pass 1: Set the array
	for(unsigned x = 0; x < wid; x++){
		for(unsigned y = 0; y < hei; y++){
			unsigned t_idx = DEF_XYtoIdx(x, y, hei);

			rgbtuple *tuple = pixelsGetRGBTupleByCoord(x, y);
			byte Ired = (byte) tuple->rgbred;
			byte Igreen = (byte) tuple->rgbgreen;
			byte Iblue = (byte) tuple->rgbblue;
			
			this->t_value_arr[t_idx] = (double)pixelsGetOriginaltValueByCoord(x, y);
			gray_I_arr[t_idx] = (double)(Ired * 30 + Igreen * 59 + Iblue * 11 + 50) / 100;
		}
	}

	//Pass 2: Transfer it into a general_matrix
	general_matrix<double> t_value_mat(wid, hei, this->t_value_arr);
	general_matrix<double> gray_I_mat(wid, hei, gray_I_arr);

	general_matrix<double> q;
	guidedfilter(gray_I_mat, t_value_mat, r, eps, q);

	memcpy(this->t_value_arr, q.GetMatrixArray(), arr_size * sizeof(double));
	double max_t_value = 0.0;
	for(int idx = 0; idx < arr_size; idx ++)
		if( this->t_value_arr[idx] > max_t_value )
			max_t_value = this->t_value_arr[idx];
	if(max_t_value > 1)
		for(int idx = 0; idx < arr_size; idx ++)
			this->t_value_arr[idx] = (this->t_value_arr[idx] / max_t_value) * 1; 
#elif OPENCV_SUPPORT
	//Pass 0: Set the basic info.

	unsigned hei = this->hazy_height, wid = this->hazy_width;
	int arr_size = hei * wid;
	this->t_value_arr = (double *) malloc(sizeof(double) * arr_size);
	
	/**
	 * Here I compute the guided image filter only use the gray image, because I'm lazy :<
	 */
	double *gray_I_arr = (double *) malloc(sizeof(double) * arr_size);

	//Pass 1: Set the array, both the [t_value_arr] and the [gray_I_arr]
	for(unsigned x = 0; x < hei; x++){
		for(unsigned y = 0; y < wid; y++){
			unsigned t_idx = DEF_XYtoIdx(x, y, wid);
			/*
			rgbtuple *tuple = pixelsGetRGBTupleByCoord(x, y);
			byte Ired = (byte) tuple->rgbred;
			byte Igreen = (byte) tuple->rgbgreen;
			byte Iblue = (byte) tuple->rgbblue;
			*/
			//For the OpenCV version, the hazy_pixel array has been totally built
			//So here I only need to access to the array

			byte Ired = (byte) this->pixel_array[t_idx].color_tuple->rgbred;
			byte Igreen = (byte) this->pixel_array[t_idx].color_tuple->rgbgreen;
			byte Iblue = (byte) this->pixel_array[t_idx].color_tuple->rgbblue;

			this->t_value_arr[t_idx] = (double)pixelsGetOriginaltValueByCoord(x, y);
			gray_I_arr[t_idx] = (double)(Ired * 30 + Igreen * 59 + Iblue * 11 + 50) / 100;
		}
	}

	//Pass 2: Transfer it into a general_matrix
	general_matrix<double> t_value_mat(hei, wid, this->t_value_arr);
	general_matrix<double> gray_I_mat(hei, wid, gray_I_arr);

	general_matrix<double> q;
	guidedfilter(gray_I_mat, t_value_mat, r, eps, q);

	memcpy(this->t_value_arr, q.GetMatrixArray(), arr_size * sizeof(double));
#endif
}

/**
 * pixelsGetOriginaltValueByCoord 
 * This one calculates the formula (12) in the paper
 */

double hazy_pixels::pixelsGetOriginaltValueByCoord(unsigned x, unsigned y){
	unsigned half_patch_size = (unsigned)this->hazy_patch_size / 2;
	unsigned start_posx, start_posy, end_posx, end_posy;
	if(this->hazy_patch_size % 2 == 0)
		half_patch_size --;

	if(x < half_patch_size){ start_posx = 0; end_posx = x + half_patch_size; }
	else{ start_posx = x - half_patch_size; end_posx = x + half_patch_size; }
	if(y < half_patch_size){ start_posy = 0; end_posy = y + half_patch_size; }
	else{ start_posy = y - half_patch_size; end_posy = y + half_patch_size; }

	if(end_posx > this->hazy_height){
		end_posx = this->hazy_height;
		start_posx = (start_posx > half_patch_size) ?
					 start_posx - half_patch_size :
					 0;
	}
	if(end_posy > this->hazy_width){
		end_posy = this->hazy_width;
		start_posy = (start_posy > half_patch_size) ?
					 start_posy - half_patch_size :
					 0;
	}

	double tValue = 1.0; 
	for(unsigned tmp_posx = start_posx; tmp_posx < end_posx; tmp_posx++){
		for(unsigned tmp_posy = start_posy; tmp_posy < end_posy; tmp_posy++){
		#ifdef FREE_IMAGE_SUPPORT
			rgbtuple * _rgbtuple = (rgbtuple*) malloc(sizeof(rgbtuple));
			_rgbtuple = pixelsGetRGBTupleByCoord(tmp_posx, tmp_posy);
		#elif OPENCV_SUPPORT
			int Idx = tmp_posx * this->hazy_width + tmp_posy;
			rgbtuple * _rgbtuple = this->pixel_array[Idx].color_tuple;
		#endif

			/*
			printf("(%u %u): ", tmp_posx, tmp_posy);
			printf("[%u %u %u]", _rgbtuple->rgbred,
								 _rgbtuple->rgbgreen,
							     _rgbtuple->rgbblue);
			*/
			double _tValue = (double)_rgbtuple->rgbred/this->A_value->rgbred;
			_tValue = DEF_MIN((double)_rgbtuple->rgbgreen/this->A_value->rgbgreen,
							  _tValue);
			_tValue = DEF_MIN((double)_rgbtuple->rgbblue/this->A_value->rgbblue, 
							  _tValue);
			tValue = DEF_MIN(tValue, _tValue);
		}
	}

	//printf("%lf\n", 1-0.95*tValue);
	
	return 1 - 0.95*tValue;
}


void hazy_pixels::pixelsSetImageAtmosphereLightValue(){
	//0: Build a cp for pixel_array
	hazy_pixel * pixel_array_cp = (hazy_pixel *) malloc(sizeof(hazy_pixel) * 
														this->hazy_width *
														this->hazy_height);
	memcpy(pixel_array_cp, this->pixel_array, sizeof(hazy_pixel) * 
												this->hazy_width *
												this->hazy_height);


	//1: sort the pixel_array
	qsort(this->pixel_array, this->hazy_width *
							 this->hazy_height,
							 sizeof(this->pixel_array[0]),
							 hazy_pixel_cmp);

	unsigned array_upper_bound = (unsigned)(0.001 *
											(double) this->hazy_width * 
												   	 this->hazy_height);

	if(array_upper_bound == 0) array_upper_bound = DEF_MIN(10, 
													this->hazy_width *
													this->hazy_height);

	//2.1 Average
	/*
	this->A_value = (rgbtuple *) malloc(sizeof(rgbtuple));
	for(unsigned i = 0; i < array_upper_bound; i++){
		this->A_value->rgbred += this->pixel_array[i].color_tuple->rgbred;
		this->A_value->rgbgreen += this->pixel_array[i].color_tuple->rgbgreen;
		this->A_value->rgbblue += this->pixel_array[i].color_tuple->rgbblue;
	}

	this->A_value->rgbred = (int)((double)this->A_value->rgbred / array_upper_bound);
	this->A_value->rgbblue = (int)((double)this->A_value->rgbblue / array_upper_bound);
	this->A_value->rgbgreen = (int)((double)this->A_value->rgbgreen / array_upper_bound);
	*/
	//2.2 Max Intensity
	qsort(this->pixel_array, array_upper_bound,
							 sizeof(this->pixel_array[0]),
							 hazy_pixel_inst_cmp);

	this->A_value = (rgbtuple *) malloc(sizeof(rgbtuple));
	this->A_value->rgbred = this->pixel_array[0].color_tuple->rgbred;
	this->A_value->rgbblue = this->pixel_array[0].color_tuple->rgbblue;
	this->A_value->rgbgreen = this->pixel_array[0].color_tuple->rgbgreen;
	/*
	printf("A: [%u %u %u]\n",	this->A_value->rgbred,
								this->A_value->rgbgreen,
								this->A_value->rgbblue);
	*/
	memcpy(this->pixel_array, pixel_array_cp, sizeof(hazy_pixel) * 
												this->hazy_width *
												this->hazy_height);
	return ;
}


byte hazy_pixels::pixelsGetDarkChannelByCoord(unsigned x, unsigned y){
	// default is floor
	unsigned half_patch_size = (unsigned)this->hazy_patch_size / 2;
	unsigned start_posx, start_posy, end_posx, end_posy;
	if(this->hazy_patch_size % 2 == 0)
		half_patch_size --;

	if(x < half_patch_size){ start_posx = 0; end_posx = x + half_patch_size; }
	else{ start_posx = x - half_patch_size; end_posx = x + half_patch_size; }
	if(y < half_patch_size){ start_posy = 0; end_posy = y + half_patch_size; }
	else{ start_posy = y - half_patch_size; end_posy = y + half_patch_size; }

	if(end_posx > this->hazy_height){
		end_posx = this->hazy_height;
		start_posx = (start_posx > half_patch_size) ?
					 start_posx - half_patch_size :
					 0;
	}
	if(end_posy > this->hazy_width){
		end_posy = this->hazy_width;
		start_posy = (start_posy > half_patch_size) ?
					 start_posy - half_patch_size :
					 0;
	}

	byte darkChannelValue = DEF_COLOR_VALUE_MAX-1; 
	int Idx = 0;
	for(unsigned tmp_posx = start_posx; tmp_posx < end_posx; tmp_posx++){
		for(unsigned tmp_posy = start_posy; tmp_posy < end_posy; tmp_posy++){
		#ifdef FREE_IMAGE_SUPPORT
			rgbtuple * _rgbtuple = (rgbtuple*) malloc(sizeof(rgbtuple));
			_rgbtuple = pixelsGetRGBTupleByCoord(tmp_posx, tmp_posy);
		#elif OPENCV_SUPPORT
			rgbtuple * _rgbtuple = this->pixel_array[Idx].color_tuple;
			Idx++;
		#endif

			/*
			printf("(%u %u): ", tmp_posx, tmp_posy);
			printf("[%u %u %u]", _rgbtuple->rgbred,
								 _rgbtuple->rgbgreen,
							     _rgbtuple->rgbblue);
			*/
			byte _darkChannelValue = _rgbtuple->rgbred;
			_darkChannelValue = DEF_MIN(_rgbtuple->rgbgreen, _darkChannelValue);
			_darkChannelValue = DEF_MIN(_rgbtuple->rgbblue, _darkChannelValue);
			darkChannelValue = DEF_MIN(darkChannelValue, _darkChannelValue);
		}
	}
	return darkChannelValue;
}

void hazy_pixels::pixelsCalculate(int r, double eps){

	//std::cout << "INFO: Enter Computation" << std::endl;
	this->pixelsSetDarkChannelValue();
	this->pixelsSetImageAtmosphereLightValue();
	this->pixelsBuildtValueArray(r, eps);

	double *scaledAnsValueArray = (double*) malloc( sizeof(double) *
													this->hazy_width *
													this->hazy_height *
													3);

	//Pass 1: Pre Calculate
	double tmp_Max[3] = { 0.0, 0.0, 0.0 };
	for(unsigned x = 0; x < this->hazy_height; x++){
		for(unsigned y = 0; y < this->hazy_width; y++){
			unsigned t_idx = DEF_XYtoIdx(x, y, this->hazy_width);
			double tValue = t_value_arr[t_idx];

			tValue = DEF_MAX(tValue, 0.1);
			
			rgbtuple * tuple = this->pixel_array[t_idx].color_tuple;
			
			byte Ired = (byte) tuple->rgbred;
			byte Igreen = (byte) tuple->rgbgreen;
			byte Iblue = (byte) tuple->rgbblue;

			//Change to [0,1]
			double scaledAvalue[3] = {(double) this->A_value->rgbred,
									  (double) this->A_value->rgbgreen,
									  (double) this->A_value->rgbblue};
			double scaledIvalue[3] = {(double) Ired,
									  (double) Igreen,
									  (double) Iblue};
			double scaledAnsValue[3] = {0.0, 0.0, 0.0};
			
			for(int idx = 0; idx < 3; idx++){

				scaledAnsValue[idx] = (scaledIvalue[idx] - scaledAvalue[idx]) / 
									  tValue + 
									  scaledAvalue[idx];
				if(scaledAnsValue[idx] > tmp_Max[idx])
					tmp_Max[idx] = scaledAnsValue[idx];
			}

			unsigned ArrayIdx = DEF_XYZtoIdx(x, y, 0, this->hazy_width, 3); 

			scaledAnsValueArray[ArrayIdx] = scaledAnsValue[0];
			scaledAnsValueArray[ArrayIdx + 1] = scaledAnsValue[1];
			scaledAnsValueArray[ArrayIdx + 2] = scaledAnsValue[2];
		}
	}

	cv::Mat saved_image_mat(this->hazy_height, this->hazy_width,  CV_8UC3);
	
	for(unsigned x = 0; x < this->hazy_height; x++){
		for(unsigned y = 0; y < this->hazy_width; y++){
			unsigned ArrayIdx = DEF_XYZtoIdx(x, y, 0, this->hazy_width, 3);
			for(int idx = 0; idx < 3; idx++){
				scaledAnsValueArray[ArrayIdx + idx] /= tmp_Max[idx];	
			}
			//printf("%f %f %f\n", scaledAnsValue[0], scaledAnsValue[1], scaledAnsValue[2]);
			/*	
			printf("[%u %u %u]\n", (unsigned)(scaledAnsValue[0] * 255),
								   (unsigned)(scaledAnsValue[1] * 255),
								   (unsigned)(scaledAnsValue[2] * 255));
			*/	

			if( scaledAnsValueArray[ArrayIdx] < 0 ) scaledAnsValueArray[ArrayIdx] = 0;
			if( scaledAnsValueArray[ArrayIdx+1] < 0 ) scaledAnsValueArray[ArrayIdx+1] = 0;
			if( scaledAnsValueArray[ArrayIdx+2] < 0 ) scaledAnsValueArray[ArrayIdx+2] = 0;

			saved_image_mat.at<cv::Vec3b>(x,y)[2] = (byte) ( scaledAnsValueArray[ArrayIdx] * 255 );
			saved_image_mat.at<cv::Vec3b>(x,y)[1] = (byte) ( scaledAnsValueArray[ArrayIdx + 1] * 255);
			saved_image_mat.at<cv::Vec3b>(x,y)[0] = (byte) ( scaledAnsValueArray[ArrayIdx + 2] * 255);

		}
	}

	this->CV_IMAGE_RES_MAT = saved_image_mat;
}
