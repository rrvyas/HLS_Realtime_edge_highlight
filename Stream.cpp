#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
//#include "inc.h"


#define WIDTH 1280
#define HEIGHT 720


typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void stream_gray_conv(pixel_stream &src, pixel_stream &dst, uint8_t bypass)
{  //, uint8_t bypass
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE s_axilite port=bypass    
//#pragma HLS INTERFACE s_axilite port=c
//#pragma HLS INTERFACE s_axilite port=r
#pragma HLS PIPELINE II=1

    static uint32_t Gr,R,G,B; 
    static int32_t X; // final convoluted gray scale value :: uint32
    static uint32_t i; // pixel count
    static uint32_t L; // Line Number
    static pixel_data p_out;
    static pixel_data g_in;
    static pixel_data k;
    static pixel_data t[4][1282];  //0 - 1281
    static int32_t A[3][3] = {{0,-1,0},{-1,4,-1},{0,-1,0}}; // Filter coefficients - Laplace filter

//-----------------------------------------

    pixel_data p_in; // input stream
    src >> p_in;
    

//-------------------------------------------

if(bypass==0)  // if bypass = 0 - process the pixels
{
    R = ((p_in.data)&0x000000FF);
    G = (((p_in.data)&0x0000FF00)>>8);
    B = (((p_in.data)&0x00FF0000)>>16);
    Gr = (B*0.07 + G*0.72 +  R* 0.21); // 8 BIT NUMBER- type cast it to 32 bit format
     
    if(Gr>255)
	Gr=255;

    g_in = p_in;
    g_in.data = (((Gr<<24)&0xFF000000)|p_in.data);  //store the grey value along with the RGB in the same 32 bit value as MSB
    p_in = g_in; //(Gr B G R)
    

//------------------------------------------


	if (p_in.user) // CHeck starting of the frame
		i = L = 0;



    if(L<2)
    {
        t[L+1][i+1] = p_in; // store the input pixel in the second and third row of the array -
        // send the same pixel input to the output stream for 2 lines so that the previous frame last two lines can be managed
        k = p_in;
        k.user = 0; // put 'user' signal as 0 to avoid the system reading the garbage values as the starting of the frame
        p_out = k;
    }
    else // if input stream line is more than 2 lines
    {

        X = A[0][0]*(int32_t)(((t[(L-2)%4][i+2].data)&0xFF000000)>>24) + A[0][1]*(int32_t)(((t[(L-2)%4][i+1].data)&0xFF000000)>>24) + A[0][2]*(int32_t)(((t[(L-2)%4][i].data)&0xFF000000)>>24)
           +A[1][0]*(int32_t)(((t[(L-1)%4][i+2].data)&0xFF000000)>>24) + A[1][1]*(int32_t)(((t[(L-1)%4][i+1].data)&0xFF000000)>>24) + A[1][2]*(int32_t)(((t[(L-1)%4][i].data)&0xFF000000)>>24)
           +A[2][0]*(int32_t)(((t[(L)%4][i+2].data)&0xFF000000)>>24) + A[2][1]*(int32_t)(((t[(L)%4][i+1].data)&0xFF000000)>>24) + A[2][2]*(int32_t)(((t[(L)%4][i].data)&0xFF000000)>>24) ;
        
        p_out = t[(L-1)%4][i+1]; // Send the 2nd row of the Array- i.e the convoluted pixel
        if(X < 100) //>200// if edge is at the pixel
        {
            p_out.data = 255;//255;//(t[(L-1)%4][i+1].data)&0x000000FF;// send the data with edge as red
        }
       
        t[(L+1)%4][i+1] = p_in; // store the data in the 4th row of the array with modulus

    }

//----------------------------------------------

    dst << p_out; // output the data
    
//----------------------------------------------   

}
else  // if bypass not equal to 0- send the pixel as it is to the output port
{
    dst << p_in;
}

	if (p_in.last)
	{
		i = 0;
		L++;
	}
	else
		i++;
}
