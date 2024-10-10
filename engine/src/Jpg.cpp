#include "Jpg.h"

bool JPG::Load(TextureData *texture, FileRef<JPG> jpgFile)
{
	struct jpeg_decompress_struct cInfo;

	jpeg_error_mgr	jerr;
	cInfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cInfo);

	unsigned long	jpgFileSize = (unsigned long)jpgFile.FileSize();
	jpeg_mem_src(&cInfo, (u8*)*jpgFile, jpgFileSize);
	
	Decompress(&cInfo, texture);

	jpeg_destroy_decompress(&cInfo);

	return true;
}

void JPG::Decompress(jpeg_decompress_struct *cInfo, TextureData *texture)
{
	jpeg_read_header(cInfo, TRUE);
	
	jpeg_start_decompress(cInfo);

	texture->m_width = cInfo->image_width;
	texture->m_height = cInfo->image_height;
	texture->m_bpp = cInfo->num_components << 3;
	u32	rowSpan = texture->m_width * cInfo->num_components;
	texture->m_pData = new u8[rowSpan * texture->m_height];

	if( texture->m_bpp == 24 )
	{
		texture->m_type	= GL_RGB;
	}
	else
	{
		texture->m_type	= GL_RGBA;
	}
	
	u8	**rowPtr = new u8*[texture->m_height];
	for(u32 i = 0; i < texture->m_height; i++)
	{
		rowPtr[i] = &(texture->m_pData[i*rowSpan]);
	}
	
	int	rowsRead = cInfo->output_height - 1;
	while(cInfo->output_scanline < cInfo->output_height) 
	{
		rowsRead -= jpeg_read_scanlines(cInfo, &rowPtr[rowsRead], cInfo->output_height - rowsRead);
	}
	delete [] rowPtr;
	
	jpeg_finish_decompress(cInfo);
}