
#include <cv.h>

//RGB转HGX
int my_BGR2HGX( IplImage *src , IplImage *dst );

/*	test the correctness of the function - my_BGR2HGX
	@HGX : the input image in HGX coler mode
	@BGR : the input image in BGR coler mode
	@row : row of the pixel which we want to test
	@col : column of the pixel which we want to test	
*/
void test_point_HGX_BGR( IplImage *HGX , IplImage *BGR , int row , int col );

/*	return a pointer which point to the first channel scalar of our test point(row,col)
		@row : row of test point
		@col : column of test point
		@HGX : the input image in HGX color mode
*/
float *get_HGX_scalar_pt( int row , int col , IplImage * src );

//获取二值化处理的阈值
int pre_binary_image_process( IplImage *frame , IplImage *binary_image , int threshold[] , int *flag );

//将原图像转换为二值图像
int binary_image_process( IplImage *frame , IplImage *binary_image , int threshold1 , int threshold2 , int threshold3 , int *is_get_flag );

/*	Otsu for the area depended by given points : point1 -> point2
		@src : source image with 3 channels
		@want_channel : the channel that you want to calculate the Otsu threshold
		@point1 : the point1 that indicate the rectangle-begin-point
		@point2 : the point2 that indicate the rectangle-end-point 
		@return : the threshold we want in INT type
	note : we are just able to calculate the 8-depth image
*/
int get_threshold_Otsu( IplImage *src  , int want_channel , CvPoint point1 , CvPoint point2 );
