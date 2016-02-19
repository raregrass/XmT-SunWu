#include <cv.h>  
#include <highgui.h>
#include "my_opencv.h"
  
#define THRESHOLD_CR 134
#define THRESHOLD_CB 125


int binary_image_process( IplImage *frame , IplImage *binary_image , int threshold1 , int threshold2 , int threshold3 , int *get_flag )
{
	//cvSmooth( frame , frame , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	//IplImage *HGX = cvCreateImage ( cvSize(640,480) , IPL_DEPTH_32F, 3 ); // HGX空间图像 for 轮廓抓取
	//my_BGR2HGX( frame , HGX );
	//test_point_HGX_BGR( HGX , frame , 120 , 200 );
	
	IplImage* ycrcb = cvCreateImage( cvGetSize(frame) , 8 , 3 );

	cvCvtColor( frame , ycrcb , CV_BGR2YCrCb );
	
	for(int i=0 ; i <  ycrcb->height ; i++ )	//二值化
    { 
        uchar *row = (uchar *)(ycrcb->imageData) + i * ycrcb->widthStep;
		for(int j=0 ; j <  ycrcb->width ; j++ ) 
        { 
			uchar *p = row + 3*j ;
			//if( *(p+1) > threshold2 && *(p+2) < threshold3 )//||
			if(	*(p) < threshold1 )  
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 255; // 白色
			}
			else 
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 0;	// 黑色
			}
		} 
	} 

	//创建用于腐蚀的核函数
	IplConvKernel *element = cvCreateStructuringElementEx( 4 , 4 , 0 , 0 , CV_SHAPE_RECT );

	cvErode( binary_image , binary_image , element , 1 );	// Erotion

    cvDilate( binary_image , binary_image , NULL , 1 );  // Dilation
	
	cvReleaseStructuringElement( &element );  
	
	cvReleaseImage( &ycrcb );

	//cvSmooth( binary_image , binary_image , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	//cvReleaseImage( &HGX );
	
	if( 1 == *get_flag )
	{
		return 0;
	}
	if( 'a' == cvWaitKey(1) )
	{
		*get_flag = 1;
	}
	return 0;
}




#if 0
extern IplImage *frame; // Main image   of main.cpp
extern IplImage *mask; // Binary image   of main.cpp

extern CvMemStorage* contours_storage ; //轮廓存储空间  of main.cpp
extern CvSeq *contours; //轮廓  of main.cpp


//	Draw the contours that are all over our threshold.
int draw_contours( int area_del_threshold , int rec_del_threshold , int *get_flag )
{
	/*
	// When this function operate our binary image-mask , it will change the mask image.
	cvFindContours( mask , contours_storage , &contours, sizeof(CvContour) ,                                        
                   CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0) );                                    

	while( contours && contours->total <= 600 )                                                              
	{                                                                                                    
		contours = contours->h_next ;                                                                   
	}                                                                                                      
    //只画出一个轮廓
	cvDrawContours( frame , contours , CV_RGB(100,100,100) , CV_RGB(0,255,0) , 0 , 2 , CV_AA , cvPoint(0,0) );  
	*/

	// When this function operate our binary image-mask , it will change the mask image.
	cvFindContours( mask , contours_storage , &contours, sizeof(CvContour) ,                                        
                   CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0) ); 
     
    CvSeq *temp_contour = contours;

	CvSeq *max_contour = contours ;
	
    double max_area = 0;  
   
    int compare_count = 0; //被删除之后用于面积比较的轮廓个数，后来可以用于计算百分比

	for(  ; contours != 0 ; contours = contours->h_next )    
    {    
        double tmparea = fabs( cvContourArea(contours) );  

		if( tmparea < area_del_threshold )     
        {    
            //contours = contours->h_next ; // 删除面积小于设定值的轮廓  
            continue;  
        }    
/*        CvBox2D aRect = cvMinAreaRect2( contours , 0 );   
        if ( ( aRect.size.height/aRect.size.width ) < rec_del_threshold )    
        {    
            //contours = contours->h_next ; //删除高宽比例小于设定值的轮廓  
            continue;  
        }    */
        if( tmparea > max_area )    
        {    
            max_area = tmparea; 
			max_contour = contours ;
        }		
		compare_count++; 	
		cvDrawContours( frame , contours , CV_RGB(0,0,255) , CV_RGB(0,0,255) , 0 , 2 , CV_AA , cvPoint(0,0) );      		 
    }    

    contours = temp_contour;  
    int count = 0;  

/*  for( ; contours != 0 ; contours = contours->h_next )  
    {    
        count++;  
        double tmp_area = fabs( cvContourArea(contours) ); 

        if( tmp_area == max_area )    
        {    
            cvDrawContours( frame , contours , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) ); 
        }   
	}    
*/    
	cvDrawContours( frame , max_contour , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );

	contours = max_contour;

	//printf("The total number of contours is: %d , compare_count is %d \ n", count , compare_count );  

	if( 1 == *get_flag )
	{
		return 0;
	}
	if( 'b' == cvWaitKey(1) )
	{
		*get_flag = 1;
	}
	return 0;
}
#endif


#if 0

/*	Otsu for the area depended by given points : point1 -> point2
		@src : source image with 3 channels
		@want_channel : the channel that you want to calculate the Otsu threshold
		@point1 : the point1 that indicate the rectangle-begin-point
		@point2 : the point2 that indicate the rectangle-end-point 
		@return : the threshold we want in INT type
	note : we are just able to calculate the 8-depth image
*/
int get_threshold_Otsu( IplImage *src  , int want_channel , CvPoint point1 , CvPoint point2 )
{
	assert( src->nChannels == 3 );
	assert( want_channel < 3 && want_channel >= 0 );

	int height = point2.y - point1.y;
	int width  = point2.x - point1.x ;
	assert( height >= 0 && width >= 0 );
	
	//histogram
	float histogram[256] = {0}; //数组初始化全0
	
	switch ( want_channel )
	{
		case 0 :	
				for(int i=point1.y ; i < point2.y ; i++ ) 
				{
					unsigned char* p = (unsigned char*)src->imageData + src->widthStep * i + point1.x;
					for( int j = point1.x ; j < point2.x ; j++ ) 
					{
						histogram[*p]++;
						p+=3;
					}
				}
				break;
		case 1 :	
				for(int i=point1.y ; i < point2.y ; i++ ) 
				{
					unsigned char* p = (unsigned char*)src->imageData + src->widthStep * i + point1.x + 1;
					for( int j = point1.x ; j < point2.x ; j++ ) 
					{
						histogram[*p]++;
						p+=3;
					}
				}
				break;
		case 2:
				for(int i=point1.y ; i < point2.y ; i++ ) 
				{
					unsigned char* p = (unsigned char*)src->imageData + src->widthStep * i + point1.x +2;
					for( int j = point1.x ; j < point2.x ; j++ ) 
					{
						histogram[*p]++;
						p+=3;
					}
				}
				break;
		default: return -1;
	}

	//normalize histogram
	int size = height * width ;
	for( int i = 0 ; i < 256 ; i++ ) 
	{
		histogram[i] = histogram[i] / size;
	}

	//average pixel value
	//avgValue = a% * 0 + b% * 1 + c% * 2 + ... + z% * 255 
	//即期望
	float avgValue = 0 ;
	for(int i=0 ; i < 256 ; i++ ) 
	{
		avgValue += i * histogram[i];
	}

	int threshold;	
	float maxVariance = 0; //最大类间方差
	float w = 0 , u = 0; //零阶矩  一阶矩 零阶矩就是平均值，一阶矩就是随机变量的期望，二阶矩就是随机变量平方的期望
	for(int i = 0 ; i < 256 ; i++ ) 
	{
		w += histogram[i];
		u += i*histogram[i];

		float t = avgValue*w - u ;
		float variance = t*t / (w*(1-w));
		if( variance > maxVariance ) 
		{
			maxVariance = variance;
			threshold = i;
		}
	}
	return threshold;
}

#endif


#if 0 
int pre_binary_image_process( IplImage *frame , IplImage *binary_image , int threshold[] , int *flag )
{	
	IplImage* ycrcb = cvCreateImage( cvGetSize(frame) , 8 , 3 );

	cvCvtColor( frame , ycrcb , CV_BGR2YCrCb );
	
	int threshold1 = get_threshold_Otsu( ycrcb , 0 , cvPoint(0,0) , cvPoint(639,439) );
	int threshold2 = get_threshold_Otsu( ycrcb , 1 , cvPoint(0,0) , cvPoint(639,439) );
	int threshold3 = get_threshold_Otsu( ycrcb , 2 , cvPoint(0,0) , cvPoint(639,439) );
	printf("i = %d j = %d k = %d \n",threshold1 , threshold2 , threshold3 );
	
	for(int i=0 ; i <  ycrcb->height ; i++ )	//二值化, 取Cr分量阈值为threshold2，取Cb分量阈值为threshold3
    { 
        uchar *row = (uchar *)(ycrcb->imageData) + i * ycrcb->widthStep;
		for(int j=0 ; j <  ycrcb->width ; j++ ) 
        { 
			uchar *p = row + 3*j ;
			if( *(p+1) > threshold2 && *(p+2) < threshold3 )  
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 255; // 255是白色
			}
			else 
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 0;	// 0是黑色
			}
		} 
	} 
	
	//cvSmooth( binary_image , binary_image , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	cvErode( binary_image , binary_image , NULL , 1 );	// Erotion
    cvDilate( binary_image , binary_image , NULL , 1 );  // Dilation

	cvReleaseImage( &ycrcb );
	cvShowImage( "Binary_cam", binary_image );

	if( cvWaitKey(1) == 'y' )
	{
		*threshold = threshold1 ; 
		*(threshold+1) = threshold2 ;
		*(threshold+2) = threshold3 ; 
		*flag = 1;
	}
	return 0;
}

#endif


#if 0

int binary_image_process( IplImage *frame , IplImage *binary_image , int threshold1 , int threshold2 , int threshold3 )
{
	//cvSmooth( frame , frame , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	//IplImage *HGX = cvCreateImage ( cvSize(640,480) , IPL_DEPTH_32F, 3 ); // HGX空间图像 for 轮廓抓取
	//my_BGR2HGX( frame , HGX );
	//test_point_HGX_BGR( HGX , frame , 120 , 200 );
	
	IplImage* HSV = cvCreateImage( cvGetSize(frame) , 8 , 3 );

	cvCvtColor( frame , HSV , CV_BGR2HSV );
	
	for(int i=0 ; i <  HSV->height ; i++ )	//二值化
    { 
        uchar *row = (uchar *)(HSV->imageData) + i * HSV->widthStep;
		for(int j=0 ; j <  HSV->width ; j++ ) 
        { 
			uchar *p = row + 3*j ;
			//if( *(p) <= threshold1 && *(p+1) >= threshold2 )  
			if( *(p+2) >= threshold3 )
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 0; // 255是白色
			}
			else 
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 255 ;	// 0是黑色
			}
		} 
	} 

	//创建用于腐蚀的核函数
	IplConvKernel *element = cvCreateStructuringElementEx( 4 , 4 , 0 , 0 , CV_SHAPE_RECT );

	cvErode( binary_image , binary_image , NULL , 1 );	// Erotion

    cvDilate( binary_image , binary_image , NULL , 1 );  // Dilation
	
	cvReleaseStructuringElement( &element );  
	
	//cvSmooth( binary_image , binary_image , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	//cvReleaseImage( &HSV );
	
	cvReleaseImage( &HSV );

	return 0;
}

int pre_binary_image_process( IplImage *frame , IplImage *binary_image , int threshold[] , int *flag )
{	
	IplImage* HSV = cvCreateImage( cvGetSize(frame) , 8 , 3 );

	cvCvtColor( frame , HSV , CV_BGR2HSV );
	
	int threshold1 = get_threshold_Otsu( HSV , 0 , cvPoint(0,0) , cvPoint(639,439) );
	int threshold2 = get_threshold_Otsu( HSV , 1 , cvPoint(0,0) , cvPoint(639,439) );
	int threshold3 = get_threshold_Otsu( HSV , 2 , cvPoint(0,0) , cvPoint(639,439) );
	printf("i = %d j = %d k = %d \n", threshold1 , threshold2 , threshold3 );
	
	for(int i=0 ; i <  HSV->height ; i++ )	//二值化, 取H分量阈值为threshold1，取S分量阈值为threshold2
    { 
        uchar *row = (uchar *)(HSV->imageData) + i * HSV->widthStep;
		for(int j=0 ; j <  HSV->width ; j++ ) 
        { 
			uchar *p = row + 3*j ;
			if( *(p) <= threshold1 && *(p+1) >= threshold2 )  
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 255; // 255是白色
			}
			else 
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 0;	// 0是黑色
			}
		} 
	} 
	
	//cvSmooth( binary_image , binary_image , CV_GAUSSIAN , 3 , 0 , 0 );  //高斯模糊
	cvErode( binary_image , binary_image , NULL , 1 );	// Erotion
    cvDilate( binary_image , binary_image , NULL , 1 );  // Dilation

	cvReleaseImage( &HSV );
	cvShowImage( "Binary_cam", binary_image );

	if( cvWaitKey(1) == 'y' )
	{
		*threshold = threshold1 ; 
		*(threshold+1) = threshold2 ;
		*(threshold+2) = threshold3 ; 
		*flag = 1;
	}
	return 0;
}

#endif

#if 0
		  
IplImage *HGX = cvCreateImage ( cvSize(640,480) , IPL_DEPTH_32F, 3 ); // HGX空间图像 for 轮廓抓取 
	
int binary_image_process( IplImage *frame , IplImage *binary_image )
{
	
	//cvSmooth( frame , frame , CV_GAUSSIAN , 21 , 0 , 0 );  //高斯模糊

	my_BGR2HGX( frame , HGX );

	test_point_HGX_BGR( HGX , frame , 120 , 200 );

	
	//Please do your job below here
	
	binary_processing( frame , binary_image );

		

	//创建用于腐蚀的核函数
	IplConvKernel * element = cvCreateStructuringElementEx( 5 , 5 , 0 , 0 , CV_SHAPE_RECT );

	cvErode( binary_image , binary_image , element , 1 );	// Erotion

    cvDilate( binary_image , binary_image , NULL , 1 );  // Dilation
	
	cvReleaseStructuringElement( &element );  
	
	cvReleaseImage( &HGX );

	//cvSmooth( binary_image , binary_image , CV_GAUSSIAN , 21 , 0 , 0 );  //高斯模糊

	return 0;
}



//二值化处理函数
int binary_processing( IplImage *frame , IplImage *binary_image , int threshold )
{
	
	for(int i=0 ; i <  frame->height ; i++ )	//二值化, 取HSV之V值介于0~100之间
    { 
        uchar *row = (uchar *)(frame->imageData) + i * frame->widthStep;
		for(int j=0 ; j <  frame->width ; j++ ) 
        { 
			uchar *p = row + 3*j + 1;
			if( *p > threshold )  
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 255; // 255是白色
			}
			else 
			{
				binary_image->imageData[ i * (binary_image->widthStep) + j ] = 0;	// 0是黑色
			}
		} 
	} //二值化結束
	return 0;
}



#endif