#include "my_opencv.h"
#include <cv.h>


/*	test the correctness of the function - my_BGR2HGX
	@HGX : the input image in HGX coler mode
	@BGR : the input image in BGR coler mode
	@row : row of the pixel which we want to test
	@col : column of the pixel which we want to test	
*/
void test_point_HGX_BGR( IplImage *HGX , IplImage *BGR , int row , int col )
{
	CvScalar s = cvGet2D( BGR , row , col );    
	printf("B=%f ,G=%f ,R=%f \n", s.val[0] ,s.val[1] ,s.val[2] );
	
	float H =  (s.val[2])*( 0.64871) + (s.val[1])*(-0.14682) + (s.val[0])*(-0.74674)  + 400;
	float G =  (s.val[2])*(-0.36161) + (s.val[1])*( 0.85855) + (s.val[0])*(-0.36349)  + 400;
	float X =  (s.val[2])*( 0.80497) + (s.val[1])*( 0.57450) + (s.val[0])*( 0.14823)  + 400;	
		
	printf("wH=%f ,wG=%f ,wX=%f \n", H , G , X );
		
	float *p = get_HGX_scalar_pt( row , col ,HGX );
	printf("rH=%f, rG=%f, rX=%f \n", p[0] , p[1] , p[2] );		
}

/*	return a pointer which point to the first channel scalar of our test point(row,col)
		@row : row of test point
		@col : column of test point
		@HGX : the input image in HGX color mode
*/
float *get_HGX_scalar_pt( int row , int col , IplImage * HGX )
{
	return ( (float *)(HGX->imageData + HGX->widthStep * row) + 3*col );
}

//将BGR空间转换到HGX空间
int my_BGR2HGX( IplImage *src , IplImage *dst )  
{  
	float *pdc = NULL ;
	uchar *psc = NULL ;

	for( int row = 0 ; row < src->height ; row++ )
	{
		//注意! widthStep是每行数据字节数大小.
		uchar *psr = (uchar *)(src->imageData + row * src->widthStep );
		float *pdr = (float *)(dst->imageData + row * dst->widthStep );
		
		for( int col = 0 ; col < src->width ; col++ )
		{			
			psc = psr + 3*col ;
			pdc = pdr + 3*col ;			
			//printf("A=%d, B=%d, C=%d \t" , psc[0] , psc[1] , psc[2] );
			pdc[0] = ( (psc[2])*( 0.64871) + (psc[1])*(-0.14682) + (psc[0])*(-0.74674) ) + 400;
			pdc[1] = ( (psc[2])*(-0.36161) + (psc[1])*( 0.85855) + (psc[0])*(-0.36349) ) + 400;
			pdc[2] = ( (psc[2])*( 0.80497) + (psc[1])*( 0.57450) + (psc[0])*( 0.14823) ) + 400;	
			//printf("Y=%f, G=%f, X=%f \n",pdc[0] , pdc[1] , pdc[2] );	
		}
	}
	return 0;  
}


