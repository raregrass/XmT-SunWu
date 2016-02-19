#define _CRT_SECURE_NO_DEPRECATE
#include<Windows.h>
#include <tchar.h>
#include <cv.h> 
#include <highgui.h>  
#include "my_opencv.h"

/* �ֱ��ʣ�
	WIDTH		HEIGHT	
	1280		720
	640			480	
*/

#define WIDTH	640
#define HEIGHT	480
#define WIDTH_IGNORE_OFFSET 10
#define HEIGHT_IGNORE_OFFSET 10

CvCapture *capture;  //MUST BE FREE    
IplImage *frame; //����ͷ��ȡ��ԭʼ����	 MUST BE FREE
IplImage *mask = cvCreateImage ( cvSize(WIDTH,HEIGHT) , IPL_DEPTH_8U , 1 ); //��ֵ��ͼ�� for ����ץȡ  MUST BE FREE


int draw_contours( int area_del_threshold , int rec_del_threshold ,  int need_contours_number , int *get_flag );	//��������
CvMemStorage* contours_storage = cvCreateMemStorage(0); //�����洢�ռ�  MUST BE FREE
CvSeq *contours; //����
CvSeq *sort_contours[6] ={ NULL }; //�������ź�˳�������ֵ : from max to min , attention: must hanndle the number below 4
CvBox2D contour_rectangle; // ��������С��Χ������
CvPoint arm_center[6] ;	// ���������ĵ� 


int get_convex_hull( int contour_number );
CvSeq *hull;//͹��  
CvMemStorage *defect_storage = cvCreateMemStorage(0);//ȱ�ݴ洢�ռ�  MUST BE FREE
CvSeq *defect;//ȱ������
CvMemStorage *palm_storage = cvCreateMemStorage(0);//����������洢�ռ� MUST BE FREE
CvSeq *palm = cvCreateSeq( CV_SEQ_ELTYPE_POINT , sizeof(CvSeq) , sizeof(CvPoint) , palm_storage ); //�������������� 


int finger_tip( int contour_number ) ;
CvMemStorage *finger_storage = cvCreateMemStorage(0);//ָ��洢�ռ� MUST BE FREE
CvSeq *finger_seq = cvCreateSeq( CV_SEQ_ELTYPE_POINT , sizeof(CvSeq) , sizeof(CvPoint) , finger_storage );//ָ������  


void hand( int contour_number );
float radius; //��С��ΧԲ�뾶  
CvPoint2D32f min_circle_center; //��С��ΧԲ����  
CvPoint min_circle_center2; //��С��ΧԲ����תint����
int palm_size[5]; //������ƴ�С���� 
int palm_size_count = 0;  //initialize 0
bool is_palm_count_full = false;   
CvPoint palm_position[5];  //�������λ������   
int palm_position_count = 0;  //initialize 0
bool is_palm_position_full = false; 


int is_get_binary = 0; //�Ƿ��Ѿ�ȷ����ֵ����ֵ
int is_get_contours = 0; //�Ƿ��Ѿ��õ�contours

int low_threshold1 = 0;
int high_threshold1 = 0;
int low_threshold2 = 0;
int high_threshold2 = 0;
int low_threshold3 = 0;
int high_threshold3 = 0;
int area_del_threshold = 500;
int rec_del_threshold = 0; 
int hand_number = 1; 

int real_contours_number = 0;

const int USER = 0x0400;
const int WM_mouse1 = USER + 100;
const int WM_mouse2 = USER + 101;
const int WM_mouse3 = USER + 102;
const int WM_mouse4 = USER + 103;
const int WM_keyboard1 = USER + 104;
const int WM_keyboard2 = USER + 105;
const int WM_keyboard3 = USER + 106;
const int WM_keyboard4 = USER + 107;

int WM_mouse[6] = { WM_mouse1 , WM_mouse2 , WM_mouse3 , WM_mouse4 , 0 , 0 };
int WM_keyboard[6] = { WM_keyboard1 , WM_keyboard2 , WM_keyboard3 , WM_keyboard4 , 0 , 0 }; 

int send_message( CvPoint palm_center , int radius , int finger_count );
CvPoint real_finger_tip[10]={0};


int main( void ) 
{              
	capture = cvCreateCameraCapture(0);

	cvSetCaptureProperty(capture , CV_CAP_PROP_FRAME_WIDTH, WIDTH);   //���û���߶�Ϊ640
	
	cvSetCaptureProperty(capture , CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);	//���û�����Ϊ480

	cvNamedWindow( "Main_cam" ,CV_WINDOW_AUTOSIZE ); //�������Ӵ�

	cvNamedWindow( "Binary_cam" , CV_WINDOW_AUTOSIZE ); //����Binary image����

	//cvNamedWindow( "Test1_cam" , CV_WINDOW_AUTOSIZE );

	//cvNamedWindow( "Test2_cam" , CV_WINDOW_AUTOSIZE );

	cvNamedWindow( "Track_bar" , CV_WINDOW_AUTOSIZE );

	// Use the trackbar to change the threshold of binary image processing
	//cvCreateTrackbar( "low_Threshold1" , "Track_bar" , &low_threshold1 , 256 , NULL );
	cvCreateTrackbar( "high_Threshold1" , "Track_bar" , &high_threshold1 , 256 , NULL );
	//cvCreateTrackbar( "low_Threshold2" , "Track_bar" , &low_threshold2 , 256 , NULL );
	cvCreateTrackbar( "high_Threshold2" , "Track_bar" , &high_threshold2 , 256 , NULL );
	//cvCreateTrackbar( "low_Threshold3" , "Track_bar" , &low_threshold3 , 256 , NULL );
	cvCreateTrackbar( "high_Threshold3" , "Track_bar" , &high_threshold3 , 256 , NULL );

	cvCreateTrackbar( "area_del_threshold" , "Track_bar" , &area_del_threshold , 10000 , NULL );
	cvCreateTrackbar( "rec_del_threshold" , "Track_bar" , &rec_del_threshold , 10 , NULL );
	cvCreateTrackbar( "hand_number" , "Track_bar" , &hand_number , 5 , NULL );

    while(1) //��ȡ����ͷӰ���ѭ��  
    { 
		frame = cvQueryFrame( capture );//��ȡһ֡
		//cvShowImage( "Main_cam" , frame );

		if( !frame ) break;
		
		assert( 0 ==
			binary_image_process( frame , mask , high_threshold1 , high_threshold2 , high_threshold3 , &is_get_binary ) 
		);
		cvShowImage( "Binary_cam" , mask );
		
		if( !is_get_binary )
		{
			goto label ; 
		}
		
		assert( 0 == 
		draw_contours(  area_del_threshold , rec_del_threshold , hand_number , &is_get_contours )
		);

		//calculate the number of contours that we really get.
		real_contours_number = 0;
		for( int i=0 ; i < 6 ; i++){
			if( sort_contours[i] != NULL )
			{
				real_contours_number++;
				continue;
			}
			break;
		}
		cvShowImage( "Main_cam" , frame );
		//printf( "the real number of contours is : %d \n", real_contours_number );
		
		if( !is_get_contours )
		{
			goto label ; 
		}		
    
	
		
		
		//we use complicated operation only if we have a contour.
		//if we have the only one contour ,it must be the max_contour ,which is sort_contours[0]. 
		if( sort_contours[0] )  
        { 
			/*contour_rectangle =  cvMinAreaRect2( contours , 0 ); 
			arm_center.x = cvRound( contour_rectangle.center.x ); 
			arm_center.y = cvRound( contour_rectangle.center.y ); 
			cvCircle( frame , arm_center , 10 , CV_RGB(255,255,255) , -1 , 8 , 0 ); */
			
			//�����������������ĵ�  
			for( int i=0 ; i < real_contours_number ; i++ ){
				contour_rectangle =  cvMinAreaRect2( sort_contours[i] , 0 ); 
				arm_center[i].x = cvRound( contour_rectangle.center.x ); 
				arm_center[i].y = cvRound( contour_rectangle.center.y ); 
				cvCircle( frame , arm_center[i] , 10 , CV_RGB(255,255,255) , -1 , 8 , 0 ); 
			}	
			
			//ȡ��͹��,����ָ�� 
			for( int i=0 ; i < real_contours_number ; i++ ){								
				get_convex_hull( i ); 
				finger_tip( i );
				hand( i );
				cvClearSeq( hull ); //���͹������
				cvClearSeq( defect );
				//cvClearMemStorage( defect_storage );
				//cvClearMemStorage( palm_storage );
				//cvClearMemStorage( finger_storage );
			}				
        }  

		cvShowImage( "Main_cam" , frame );	//��ʾԭʼͼ��

label:	//cvClearMemStorage( contours_storage );
		//cvClearMemStorage( defect_storage );
		//cvClearMemStorage( palm_storage );
		//cvClearMemStorage( finger_storage );
		if( cvWaitKey(1) == 27 || !frame ) 
        { 
              break; 
        } 

	}

	cvReleaseCapture( &capture );
	cvReleaseImage( &mask );	

	cvReleaseMemStorage( &contours_storage );
	cvReleaseMemStorage( &defect_storage );
	cvReleaseMemStorage( &palm_storage );
	cvReleaseMemStorage( &finger_storage );

	cvDestroyWindow( "Main_cam" );
	cvDestroyWindow( "Binary_cam" );	
	//cvDestroyWindow( "Test1_cam" ); 
	//cvDestroyWindow( "Test2_cam" );
	cvDestroyWindow( "Track_bar" );
	
	return 0;
	
}


#if 1
//	Draw the contours that are all over our threshold.
int draw_contours( int area_del_threshold , int rec_del_threshold , int need_contours_number , int *get_flag )
{
	// When this function operate our binary image-mask , it will change the mask image.
	cvFindContours( mask , contours_storage , &contours, sizeof(CvContour) ,                                        
                   CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0) ); 
     
	/*CvSeq *max_contour = NULL ;
	CvSeq *fourth_contour = NULL ;
	CvSeq *fourth_contour = NULL ; 
	CvSeq *fourth_contour = NULL ;*/

	CvSeq *temp_contour = contours ;
	for( int i=0 ; i < 6 ; i++ ){
		sort_contours[i] = NULL ;
	}

/*	
	double max_area = 0;  
   
    for(  ; contours != 0 ; contours = contours->h_next )    
    {    
        double tmparea = fabs( cvContourArea(contours) );  

		if( tmparea < area_del_threshold )     
        {    
            //contours = contours->h_next ; // ɾ�����С���趨ֵ������  
            continue;  
        }    

        if( tmparea > max_area )    
        {    
            max_area = tmparea; 
			sort_contours[0] = contours ;
        }		
	
		compare_count++; 	
		//������������
		//cvDrawContours( frame , contours , CV_RGB(0,0,255) , CV_RGB(0,0,255) , 0 , 2 , CV_AA , cvPoint(0,0) );      		 
    }  
*/
   
	double max_area = 0 ;
	double second_area = 0;
	double third_area = 0 ;
	double fourth_area = 0;
	switch ( need_contours_number )
	{
	case 4:
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > max_area ){    
				max_area = contours->total; 
				sort_contours[0] = contours ;
			}		  		 
		}   
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > second_area && contours->total != max_area ){    
				second_area = contours->total; 
				sort_contours[1] = contours ;
			}		  		 
		} 
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > third_area && contours->total != max_area && contours->total != second_area ){    
				third_area = contours->total; 
				sort_contours[2] = contours ;
			}		  		 
		}
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > fourth_area && contours->total != max_area && contours->total != second_area && contours->total != third_area ){    
				fourth_area = contours->total; 
				sort_contours[3] = contours ;
			}		  		 
		}
		contours = temp_contour;
		cvDrawContours( frame , sort_contours[0] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[1] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[2] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[3] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		break;
	case 3:
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > max_area ){    
				max_area = contours->total; 
				sort_contours[0] = contours ;
			}		  		 
		}   
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > second_area && contours->total != max_area ){    
				second_area = contours->total; 
				sort_contours[1] = contours ;
			}		  		 
		} 
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > third_area && contours->total != max_area && contours->total != second_area ){    
				third_area = contours->total; 
				sort_contours[2] = contours ;
			}		  		 
		}
		contours = temp_contour;
		cvDrawContours( frame , sort_contours[0] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[1] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[2] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		break;
	case 2:
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > max_area ){    
				max_area = contours->total; 
				sort_contours[0] = contours ;
			}		  		 
		}   
		contours = temp_contour;
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > second_area && contours->total != max_area ){    
				second_area = contours->total; 
				sort_contours[1] = contours ;
			}		  		 
		} 
		contours = temp_contour;
		cvDrawContours( frame , sort_contours[0] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		cvDrawContours( frame , sort_contours[1] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		break;
	case 0:
		break;
	default:
		for(  ; contours != 0 ; contours = contours->h_next ){    
			if( contours->total < area_del_threshold ){    
				continue;  
			}  
			if( contours->total > max_area ){    
				max_area = contours->total; 
				sort_contours[0] = contours ;
			}		  		 
		}   
		contours = temp_contour;
		cvDrawContours( frame , sort_contours[0] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
		break;
	}

/*	
	for(  ; contours != 0 ; contours = contours->h_next )    
    {    
		if( contours->total < area_del_threshold )     
        {    
            //contours = contours->h_next ; // ɾ�����С���趨ֵ������  
            continue;  
        }    

       CvBox2D aRect = cvMinAreaRect2( contours , 0 );   
        if ( ( aRect.size.height/aRect.size.width ) < rec_del_threshold )    
        {    
            //contours = contours->h_next ; //ɾ���߿����С���趨ֵ������  
            continue;  
        }  

        if( contours->total > max_area )    
        {    
            max_area = contours->total; 
			sort_contours[0] = contours ;
        }		
	
		compare_count++; 	
		//������������
		//cvDrawContours( frame , contours , CV_RGB(0,0,255) , CV_RGB(0,0,255) , 0 , 2 , CV_AA , cvPoint(0,0) );      		 
    }   
	cvDrawContours( frame , sort_contours[0] , CV_RGB(255,0,0) , CV_RGB(255,0,0) , 0 , 2 , CV_AA , cvPoint(0,0) );
*/ 

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


#if 1
// Here we will get the hull contour and get our ZhiFeng? 
int get_convex_hull( int contour_number ) 
{ 
	CvPoint pt0 , pt; //��͹���������õ���ʱ��

	CvPoint hull_p; //��ѡȱ�ݵ�

	hull = cvConvexHull2( sort_contours[contour_number] , 0 , CV_CLOCKWISE , 0 ); 
    
	pt0 = **CV_GET_SEQ_ELEM( CvPoint* , hull , hull->total - 1 ); 
    
	//��ѭ����͹������β����
	for(int i = 0; i < hull->total; i++ ) 
    { 
		pt = **CV_GET_SEQ_ELEM( CvPoint*, hull, i );  
		/*
			���ܣ�����������������߶�
			����ԭ�ͣ�void cvLine( CvArr* img, CvPoint pt1, CvPoint pt2, CvScalar color, int thickness=1, int line_type=8, int shift=0 );
				@img : ͼ��
				@pt1 : �߶εĵ�һ���˵�
				@pt2 : �߶εĵڶ����˵�
				@color : �߶ε���ɫ
				@thickness : �߶εĴ�ϸ�̶�
				@line_type : �߶ε�����
							8 (or 0)	- 8-connected line��8�ڽ�)������
							4		- 4-connected line (4�ڽ�)������
							CV_AA	- antialiased ����
				@shift �� ������С����λ��
		*/
		cvLine( frame, pt0, pt, CV_RGB(128 , 128 , 128) , 2 , 8 , 0 );  //���� Arm ��������Ϊ��ɫ  
		pt0 = pt; 
    }

	//Ѱ��ȱ��                  
    defect = cvConvexityDefects( sort_contours[contour_number] , hull , defect_storage );   
	
	for(int i=0 ; i < defect->total ; i++ ) 
    {
		/* 
			d ָ��һ��CvConvexityDefect�ṹ��
			typedef struct CvConvexityDefect
			{
				CvPoint *start;		// ȱ�ݿ�ʼ�������� 
				CvPoint *end;			// ȱ�ݽY���������� 
				CvPoint *depth_point;	// ȱ���о���͹����Զ��������(�ȵ�) 
				float depth;			// �ȵ׾���͹�ε����
			} CvConvexityDefect;
		*/
		CvConvexityDefect *d = (CvConvexityDefect*) cvGetSeqElem( defect , i ); 
        
		 //�������10��ȱ��,������Ϊ���Ǳ�ѡȱ�ݵ�
		if( d->depth > 10 )  
		{ 
			// hull_p��ʾָ�������ȱ��
			hull_p.x = d->depth_point->x ; 
            hull_p.y = d->depth_point->y ; 
            
			// ������ѡȱ�ݵ�
			/*
				The function draws a simple or filled circle with a given center and radius.
				void cvCircle(CvArr* img, CvPoint center, int radius, CvScalar color, int thickness=1, int lineType=8, int shift=0)	
					Parameters:
					img �C Image where the circle is drawn
					center �C Center of the circle
					radius �C Radius of the circle
					color �C Circle color
					thickness �C Thickness of the circle outline if positive, otherwise this indicates that a filled circle is to be drawn
					lineType �C Type of the circle boundary, see Line description
					shift �C Number of fractional bits in the center coordinates and radius value	
			*/
			cvCircle( frame , hull_p , 5 , CV_RGB(255,255,255) , -1 , CV_AA , 0 ) ; 
            
			//��ָ������洢��palm������
			cvSeqPush( palm , &hull_p ); 
		} 
	} 
	return 0;
}
#endif


#if 1
// Catch the finger tips
int finger_tip( int contour_number )
{     
    int dot_product ; //�������
	float length1 , length2 ; // ����ģ��
	float angle ; //�����н�cosineֵ
	CvPoint vector1 , vector2 ; //����
	CvPoint *p1 , *p2 , *p ; //����ʱʹ�õ���ʱ��

	CvPoint finger_tip[100] = {0}; //ָ������
	int tip_location[100] = {0};  //ָ����contour���index
	int count = 0; //ָ�����
	
	CvPoint p_hull ; // hull��ɵ�
    
	/*
	//��ʾ���е� hull ��
	for(int i = 0; i < hull->total; i++ ) 
    { 
		p_hull = **CV_GET_SEQ_ELEM( CvPoint*, hull, i );  
		cvCircle( frame , p_hull , 4 , CV_RGB(0,0,255) , -1 , 8 , 0 ); //��ʾ���е�
    }*/
	
	for( int i=0 ; i < sort_contours[contour_number]->total ; i++ ) 
    { 
		p1 = (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , i ); 
        p  = (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , (i+20) % sort_contours[contour_number]->total ); 
        p2 = (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , (i+40) % sort_contours[contour_number]->total ); 
       
		vector1.x = p->x - p1->x; 
        vector1.y = p->y - p1->y; 
        vector2.x = p->x - p2->x; 
        vector2.y = p->y - p2->y; 
       
		dot_product = ( vector1.x * vector2.x ) + ( vector1.y * vector2.y );  
 
        length1 = sqrtf( (vector1.x * vector1.x) + (vector1.y * vector1.y) );  
        length2 = sqrtf( (vector2.x * vector2.x) + (vector2.y * vector2.y) ); 
        
		angle = fabs( dot_product / (length1*length2) );     
 
		int tmp = 0;
        if( angle < 0.2 ) 
        { 
			//��ʾ���е� hull ��
			for( int i = 0; i < hull->total; i++ ) { 
				p_hull = **CV_GET_SEQ_ELEM( CvPoint*, hull, i );  
				if( p_hull.x >= (p->x - 10) && p_hull.x <= (p->x + 10) 
					&& p_hull.y >= (p->y -10) && p_hull.y <= (p->y + 10) )
				{
					tmp = 1;
					break;
				}
			}
			if( 1 == tmp ){
				//cvCircle( frame , p_hull , 4 , CV_RGB(255,255,255) , -1 , 8 , 0 ); //��ʾ����ָ���ѡ��
				finger_tip[count] = p_hull ; 
				tip_location[count] = i + 20 ; 
				count = count + 1 ; 
			}
		}
	}	

	for( int i=0 ; i < count ; i++ ) 
    { 
        if( ( tip_location[i] - tip_location[i-1]) > 50 ) //tip_location ���鱣���ѡ��������contour���index
        { 
	        if( finger_tip[i].x >= (WIDTH - WIDTH_IGNORE_OFFSET) || finger_tip[i].y >= (HEIGHT - HEIGHT_IGNORE_OFFSET) 
				|| finger_tip[i].x <= WIDTH_IGNORE_OFFSET || finger_tip[i].y <= HEIGHT_IGNORE_OFFSET) 
	        { 
	            cvCircle( frame , finger_tip[i] , 6 , CV_RGB(0,0,255) , -1 , 8 , 0 );  //��Ļ��Ե�㱻��Ϊ���ֱ۱߽�㣺��ɫ                  
	        }  
	        else
	        { 
	          	//cvCircle( frame , finger_tip[i] , 6 , CV_RGB(0,255,0) , -1 , 8 , 0 ); 
	        	//cvLine( frame , finger_tip[i] , arm_center , CV_RGB(255,0,0) , 1 , CV_AA , 0 ); 
	          	cvSeqPush( finger_seq , &finger_tip[i] ); 
	        } 
        } 
    } 
	return 0;
} 
#endif


#if 1
// Here we will calculate the hand position by some method
void hand( int contour_number ) 
{ 
	int use_ave_palm = 1; 
    
	//if the points in palm sequence is less than 2 , we will add an additional point into the sequence
	//if start
	if( palm->total <= 2 ) 
    { 
		use_ave_palm = 0 ;             
        CvFont Font1 = cvFont( 3 , 3 ); 
        cvPutText( frame , "NO finger!" , cvPoint(10,50) , &Font1 , CV_RGB(255,0,0) );   

        CvPoint *temp , *additional=NULL , *palm_temp ; 
        
		CvMemStorage *palm2_storage = cvCreateMemStorage(0);	// MUST BE FREE
        CvSeq *palm2 = cvCreateSeq( CV_SEQ_ELTYPE_POINT , sizeof(CvSeq) , sizeof(CvPoint) , palm2_storage ); //�洢���������������   
          
        for( int i=0; i < palm->total ; i++ ) 
        { 
			palm_temp = (CvPoint*)cvGetSeqElem( palm , i ); 
               
			for( int j=1 ; j < sort_contours[contour_number]->total ; j++ ) 
			{ 
				temp =  (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , j );   
 
				if( temp->y == palm_temp->y && temp->x == palm_temp->x ) 
				{ 
					additional = (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , 
										(int)( j + ((sort_contours[contour_number]->total)/2 ) ) % (sort_contours[contour_number]->total) ); 

					if( additional->y <= palm_temp->y ) 
						cvCircle( frame , *additional , 10 , CV_RGB(0,0,255) , -1 , 8 , 0 );  
 
					cvSeqPush(palm2,additional);                   
				}                                 
			}              
		} 
        for( int i=0 ; i < palm2->total ; i++ ) 
        { 
           temp = (CvPoint*)cvGetSeqElem( palm2 , i );  
           cvSeqPush( palm , temp );     
        } 
          
        for( int i=1 ; i < sort_contours[contour_number]->total ; i++ ) 
        { 
             temp =  (CvPoint*)cvGetSeqElem( sort_contours[contour_number] , 1 );   
             if( temp->y <= additional->y )
				 additional = temp;         
        } 
        cvCircle( frame , *additional , 10 , CV_RGB(0,0,255) , -1 , 8 , 0 ); 
		//cvReleaseMemStorage( &palm2_storage );
        cvSeqPush( palm , additional ); 
	} 
	//if end

    //����ƽ��λ��   
	cvMinEnclosingCircle( palm , &min_circle_center , &radius ); 
    min_circle_center2.x = cvRound( min_circle_center.x ); 
    min_circle_center2.y = cvRound( min_circle_center.y ); 
      
    if( use_ave_palm )
	{ 
		CvPoint ave_Palm_Center , dis_temp ; 
		int length_temp , radius2 ; 
		
		ave_Palm_Center.x = 0; 
		ave_Palm_Center.y = 0; 
		for( int i=0 ; i < palm->total ; i++ ) // ����ƽ��λ��  
		{ 
			CvPoint *temp = (CvPoint*)cvGetSeqElem( palm , i );  
 
			ave_Palm_Center.x += temp->x; 
			ave_Palm_Center.y += temp->y;         
		} 
		ave_Palm_Center.x = (int)( ave_Palm_Center.x / palm->total ); 
		ave_Palm_Center.y = (int)( ave_Palm_Center.y / palm->total ); 
		
		radius2 = 0; 
		for( int i=0 ; i < palm->total ; i++ ) //����ƽ������ 
		{ 
			CvPoint *temp = (CvPoint*)cvGetSeqElem( palm , i );  
 
			dis_temp.x = temp->x - ave_Palm_Center.x;       
			dis_temp.y = temp->y - ave_Palm_Center.y;   
 
			length_temp =  sqrtf( (dis_temp.x * dis_temp.x) + (dis_temp.y*dis_temp.y) ); 
			radius2 += length_temp; 
		}
		radius2 = (int)( radius2 / palm->total ); 
		radius = ( (0.5)*radius + (0.5)*radius2 ); 
		
		min_circle_center2.x = ( (0.5)*min_circle_center2.x + (0.5)*ave_Palm_Center.x ); 
		min_circle_center2.y = ( (0.5)*min_circle_center2.y + (0.5)*ave_Palm_Center.y ); 
	} 

/*	//�����յ�ƽ������λ�� 
    palm_position[palm_position_count].x = cvRound( min_circle_center2.x ); 
    palm_position[palm_position_count].y = cvRound( min_circle_center2.y ); 
    palm_position_count = ( palm_position_count + 1 ) % 3; 
	if( is_palm_position_full ) 
    { 
		float xtemp=0,ytemp=0; 
        for( int i=0 ; i < 3 ; i++ ) 
        { 
           xtemp += palm_position[i].x;    
           ytemp += palm_position[i].y;           
        }         
        min_circle_center2.x = cvRound(xtemp/3);  
        min_circle_center2.y = cvRound(ytemp/3);     
	} 
    if( palm_position_count == 2 && is_palm_position_full == false ) 
    { 
        is_palm_position_full = true;                        
    } */
    cvCircle( frame , min_circle_center2 , 10 , CV_RGB(0,255,255) , 4 , 8 , 0 );  //���������жϵ�palm����
  	       
/*    //��ƽ�������ƴ�С  
	palm_size[ palm_size_count ] = cvRound( radius ); 
    palm_size_count = ( palm_size_count + 1 ) % 3; 
    if( is_palm_count_full ) 
    { 
		float temp_count = 0; 
        for( int i=0 ; i < 3 ; i++ ) 
        { 
           temp_count += palm_size[i];              
        }           
        radius = temp_count/3;      
    } 
    if( palm_size_count == 2 && is_palm_count_full == false ) 
    { 
        is_palm_count_full = true;                        
	} */
    cvCircle( frame , min_circle_center2 , cvRound(radius) , CV_RGB(255,0,0) , 2 , 8 , 0 );  //����������С��ΧԲ
    cvCircle( frame , min_circle_center2 , cvRound(radius*1.3) , CV_RGB(200,100,200) , 1 , 8 , 0 );  //����ָ���ų�Բ

    int finger_count = 0;  //���ձ�ȷ��Ϊָ��ĸ���������ʹ����������ж�
    float finger_length;  
    CvPoint tip_vector ;  //ָ�⵽�����ĵ����� 
	CvPoint *point;  
       
	for( int i=0 ; i < finger_seq->total ; i++ ) 
    { 
		point = (CvPoint*)cvGetSeqElem( finger_seq , i ); 
        tip_vector.x = point->x - min_circle_center2.x; 
        tip_vector.y = point->y - min_circle_center2.y; 
        finger_length = sqrtf( (tip_vector.x * tip_vector.x) + (tip_vector.y * tip_vector.y) ); 

		if( (int)finger_length > cvRound(radius*1.3) ) 
        { 
			real_finger_tip[finger_count] = *point ;
			finger_count += 1;    
			
            cvCircle( frame , *point , 6 , CV_RGB(0,255,0) , -1 , 8 , 0 );  //���������ж���ָ��
        }        
	}    
	send_message(  min_circle_center2 , radius , finger_count );

    cvClearSeq( finger_seq );  
    cvClearSeq( palm ); 
}
#endif

bool is_move_count_full = false;
int move_count = 0;
CvPoint move_point[3] = {0};
int move_flag = 0;

// Here we handle the message process
int send_message( CvPoint palm_center , int radius , int finger_count )
{
	HWND hWnd = FindWindow( NULL , _T("�˶�ʶ���") );

	if( hWnd != NULL )
	{
		if( 1 == real_contours_number ) //�������ģʽ
		{
			int x = 0 ;
			int y = 0 ;
			switch (finger_count){
			case 2:
				/*for( int i=0 ; i < finger_count ; i++ ){
					x += real_finger_tip[i].x ;
					y += real_finger_tip[i].y ;
				}
				x = x / 2;
				y = y / 2;
				SendMessage( hWnd , WM_mouse[0] , (int)x , (int)y );*/
				
				SendMessage( hWnd , WM_keyboard[0] , 12 , 0 );
				return 0 ;
			case 5:
				/*for( int i=0 ; i < finger_count ; i++ ){
					x += real_finger_tip[i].x ;
					y += real_finger_tip[i].y ;
				}
				x = x / 5;
				y = y / 5;
				SendMessage( hWnd , WM_mouse[0] , (int)x , (int)y );*/
				
				SendMessage( hWnd , WM_keyboard[0] , 15 , (int)radius );
				return 0 ;
			case 4:
				/*for( int i=0 ; i < finger_count ; i++ ){
					x += real_finger_tip[i].x ;
					y += real_finger_tip[i].y ;
				}
				x = x / 4;
				y = y / 4;				
				move_count = (move_count + 1) % 5;
				if( is_move_count_full == false && move_count == 4 ){
					is_move_count_full = true;
				}
				if( is_move_count_full == true ){
					
					if( move_flag == 0 ){
						move_point[0].x = x ;
						move_point[0].y = y ;
						move_flag = 1;
					}
					if( move_flag == 1 ){
						move_point[1].x = x;
						move_point[1].y = y;
						if( move_point[1].x <= move_point[0].x ){
							SendMessage( hWnd , WM_keyboard[0] , 14 , 0 );
							goto label;
						}
						if( move_point[1].x > move_point[0].x ){
							SendMessage( hWnd , WM_keyboard[0] , 14 , 0 );
							goto label;
						}
						if( move_point[1].y <= move_point[0].y ){
							SendMessage( hWnd , WM_keyboard[0] , 14 , 3 );
							goto label;
						}
						if( move_point[1].y > move_point[0].y ){
							SendMessage( hWnd , WM_keyboard[0] , 14 , 2 );
							goto label;
						}
	label:					move_flag = 0;
					}
					is_move_count_full = false;
				}
				*/
				SendMessage( hWnd , WM_keyboard[0] , 14 , 0 );
				//SendMessage( hWnd , WM_mouse[0] , (int)x , (int)y );
				return 0 ;
			case 3:
				SendMessage( hWnd , WM_keyboard[0] , 13 , 0 );
				return 0;
			/*case 1:
				//SendMessage( hWnd , WM_keyboard[0] , 0 , 0 );
				SendMessage( hWnd , WM_mouse[0] , (int)real_finger_tip[0].x , (int)real_finger_tip[0].y );
				return 0 ;*/
			default:
				//SendMessage( hWnd , WM_keyboard[0] , 0 , 0 );
				SendMessage( hWnd , WM_mouse[0] , (int)palm_center.x , (int)palm_center.y );
				return 0;
			}
		}
	}
	else{	
		printf("No client !");
	}  
	return 0 ;
}






#if 0
//	Draw the contours that are all over our threshold.
int draw_contours( )
{
	// When this function operate our binary image-mask , it will change the mask image.
	cvFindContours( mask , contours_storage , &contours, sizeof(CvContour) ,                                        
                   CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0) );                                    

	while( contours && contours->total <= 600 )                                                              
	{                                                                                                    
		contours = contours->h_next ;                                                                   
	}                                                                                                      
    //ֻ����һ������
	cvDrawContours( frame , contours , CV_RGB(100,100,100) , CV_RGB(0,255,0) , 0 , 2 , CV_AA , cvPoint(0,0) );  

	return 0;
}
#endif
