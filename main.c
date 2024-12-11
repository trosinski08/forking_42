#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)

#pragma pack(1)
struct bmp_header
{
	// Note: header
	i8  signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width; // in px
	u32 height; // in px
	u16 number_of_planes; // should be 1
	u16 bit_per_pixel; // 1, 4, 8, 16, 24 or 32
	u32 compression_type; // should be 0
	u32 compressed_image_size; // should be 0
	// Note: there are more stuff there but it is not important here
};

struct file_content
{
	i8*   data;
	u32   size;
};

struct file_content   read_entire_file(char* filename)
{
	char* file_data = 0;
	unsigned long	file_size = 0;
	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd >= 0)
	{
		struct stat input_file_stat = {0};
		stat(filename, &input_file_stat);
		file_size = input_file_stat.st_size;
		file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		PRINT_ERROR("Usage: decode <input_filename>\n");
		return 1;
	}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL)
	{
		PRINT_ERROR("Failed to read file\n");
		return 1;
	}
	// when we find, first row, position x+7 is message size pixel
	// position x+7 y-2 is the last pixel, so read in loop
	// made 3 threads, one to read every chanel, 4th one maybe to keep rest in order
	// collect chars and display message
	struct bmp_header* header = (struct bmp_header*) file_content.data;
	printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n", \
		header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);
	//function to loop through bpm file, we are lookin for a 7 pixels in a row (127, 188, 127, 0), returning header with x, y in bpm file position position
	u8 patern_pix[] = {127, 188, 217, 0};
	u8* pixel_data = (u8*) file_content.data + header->data_offset;
	size_t n_y = 0;
	size_t n_x = 0;
	for (size_t y = 0; y < header->height; y++)
	{
		for (size_t x = 0; x < header->width; x++)
        {
            u8* pixel = pixel_data + (y * header->width + x) * (header->bit_per_pixel / 8);
            // printf("Pixel at (%zu, %zu): B=%u, G=%u, R=%u\n", y, x, pixel[0], pixel[1], pixel[2]);
            if (pixel[0] == patern_pix[0] && pixel[1] == patern_pix[1] && pixel[2] == patern_pix[2])
            {
				// printf("Start of pattern found at (%zu, %zu)\n", y, x);
				n_y = y;
				n_x = x + 1;
			}
			
		}
	}
	u8* pixel_mark = pixel_data + (n_y * header->width + n_x) * (header->bit_per_pixel / 8);
	// printf("%u blue mark, %u red mark\n", pixel_mark[0], pixel_mark[2]);
	size_t size = pixel_mark[0] + pixel_mark[2];
 	char messagge[size];
	n_y -=2;
	n_x -=5;
	// printf("Start of printing (%zu, %zu)\n", n_y, n_x);
	u8* pixel_mess;
	for(size_t k = 0; k < size; k++)
	{
		for (size_t j = 0; j < 6; j++)
		{
			pixel_mess = pixel_data + (n_y * header->width + n_x) * (header->bit_per_pixel / 8);
			// printf("Start of printing (%zu, %zu)\n", n_y, n_x);
			for (size_t i = 0; i < 3; i++)
			{
				char c = pixel_mess[i%3];
				write(1, &c, 1);
				k++;
				if (k >= size)
				break;
			}
			n_x++;
		}
		n_y++;
	}
					
	return 0;

}