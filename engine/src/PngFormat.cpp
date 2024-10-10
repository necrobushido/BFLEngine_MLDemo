#include "PngFormat.h"

#include "png.h"

bool PNG::Load(TextureData *texture, FileRef<PNG> pngFile)
{
	png_image	image;

	memset(&image, 0, sizeof(image));
	image.version = PNG_IMAGE_VERSION;

	//if( png_image_begin_read_from_file(&image, argv[1]) != 0 )
	if( png_image_begin_read_from_memory(&image, *pngFile, pngFile.FileSize()) )
	{
		/* Set the format in which to read the PNG file; this code chooses a
		* simple sRGB format with a non-associated alpha channel, adequate to
		* store most images.
		*/
		image.format = PNG_FORMAT_RGBA;

		/* Now allocate enough memory to hold the image in this format; the
		* PNG_IMAGE_SIZE macro uses the information about the image (width,
		* height and format) stored in 'image'.
		*/
		png_bytep	buffer = new u8[PNG_IMAGE_SIZE(image)];
		Assert(buffer);

		/* If enough memory was available read the image in the desired format
		* then write the result out to the new file.  'background' is not
		* necessary when reading the image because the alpha channel is
		* preserved; if it were to be removed, for example if we requested
		* PNG_FORMAT_RGB, then either a solid background color would have to
		* be supplied or the output buffer would have to be initialized to the
		* actual background of the image.
		*
		* The fourth argument to png_image_finish_read is the 'row_stride' -
		* this is the number of components allocated for the image in each
		* row.  It has to be at least as big as the value returned by
		* PNG_IMAGE_ROW_STRIDE, but if you just allocate space for the
		* default, minimum, size using PNG_IMAGE_SIZE as above you can pass
		* zero.
		*
		* The final argument is a pointer to a buffer for the colormap;
		* colormaps have exactly the same format as a row of image pixels (so
		* you choose what format to make the colormap by setting
		* image.format).  A colormap is only returned if
		* PNG_FORMAT_FLAG_COLORMAP is also set in image.format, so in this
		* case NULL is passed as the final argument.  If you do want to force
		* all images into an index/color-mapped format then you can use:
		*
		*    PNG_IMAGE_COLORMAP_SIZE(image)
		*
		* to find the maximum size of the colormap in bytes.
		*/
		if( png_image_finish_read(&image, NULL/*background*/, buffer, 0/*row_stride*/, NULL/*colormap*/) != 0 )
		{
			//	success
			texture->m_width = image.width;
			texture->m_height = image.height;
			texture->m_bpp = PNG_IMAGE_PIXEL_SIZE(image.format) << 3;

			Assert(texture->m_bpp == 24 || texture->m_bpp == 32);

			if( texture->m_bpp == 24 )
			{
				texture->m_type	= GL_RGB;
			}
			else
			{
				texture->m_type	= GL_RGBA;
			}

			texture->m_pData = buffer;

			texture->FlipImageVertically();
		}
		else
		{
			//	something went wrong
			Assert(0);
		}
	}
	else
	{
		//	something went wrong
		Assert(0);
	}

	return true;
}