__kernel void rotation(read_only image2d_t orig_im, write_only image2d_t new_im, sampler_t grid_sampler, 
                        int height, int width, int new_height, int new_width, float angle) 
{
    
// Saving the global ids in variables

    int i = get_global_id(0);
    int j = get_global_id(1);

// Calculating the new centers

    int center_height = round(((height + 1.) / 2) - 1);
    int center_width = round(((width + 1.) / 2) - 1);

    int new_center_height = round(((new_height + 1.) / 2) - 1);
    int new_center_width = round(((new_width + 1.) / 2) - 1);

// Calculating the new coordinates

    int x = new_width - 1 - i - new_center_width;
    int y = new_height - 1 - j - new_center_height;

    int new_x = center_width - round(x*cos(angle) + y*sin(angle))-1;
    int new_y = center_height - round(-x*sin(angle) + y*cos(angle))-1;

// Creating the 2D image objects
// Saving the original coordinates in the new ones

    float4 pixel = read_imagef(orig_im, grid_sampler, (int2)(new_y,new_x));
    //if(0 < new_x && new_x < width && 0 < new_y && new_y < height)
    //{
    write_imagef(new_im, (int2)(i,j), pixel);
    //}

}

//#########################################################################

__kernel void resize_gamma(read_only image2d_t orig_im, write_only image2d_t new_im, sampler_t grid_sampler, 
                        int width, int height, int new_width, int new_height, float gamma_val) 
{

// Saving the global ids in variables

    int i = get_global_id(0);
    int j = get_global_id(1);

// Calculating the new coordinates

    int new_x = round(width*i/new_width*1.0);
    int new_y = round(height*j/new_height*1.0);

// Creating the 2D image objects
// Saving the original coordinates in the new ones

    float4 pixel = read_imagef(orig_im, grid_sampler, (int2)(new_x,new_y));
    write_imagef(new_im, (int2)(i,j), pow(pixel, gamma_val));
    

}
