﻿#include "fileformats.h"
#include "Utilities.h"

bool WINAPI DllMain(HINSTANCE base, DWORD reason, void* parameters)
{
	if (reason == DLL_PROCESS_ATTACH)
		thomas::logger::prepare_log();
	else if (reason == DLL_PROCESS_DETACH)
		thomas::logger::close_log();

	return true;
}

bool save_cache_to_bitmap(const std::string& filename, thomas::vxlfile& vxl, thomas::palette& palette, const size_t index = 0)
{
	if (!vxl.is_loaded() || !palette.is_loaded())
		return false;

	std::ofstream file(filename + ".bmp", std::ios::out | std::ios::binary);
	if (!file)
		return false;

	thomas::vxlfile::cache_storage cache = vxl.frame_cache(index);
	thomas::vxlfile::cache_storage shadow = vxl.shadow_cache(index);

	BITMAPFILEHEADER file_header;
	BITMAPINFOHEADER info_header;
	RGBQUAD color_index[256];

	memset(&file_header, 0, sizeof file_header); 
	memset(&info_header, 0, sizeof info_header);

	int32_t width = cache.frame_bound.right - cache.frame_bound.left;
	int32_t height = cache.frame_bound.bottom - cache.frame_bound.top;
	int32_t swidth = shadow.frame_bound.right - shadow.frame_bound.left;
	int32_t sheight = shadow.frame_bound.bottom - shadow.frame_bound.top;

	int64_t stride = ((width * 8 + 31) & ~31) >> 3;
	int64_t sstride = ((swidth * 8 + 31) & ~31) >> 3;

	file_header.bfSize = sizeof file_header;
	file_header.bfType = 'MB';
	file_header.bfOffBits = sizeof file_header + sizeof info_header + sizeof color_index;

	info_header.biSize = sizeof info_header;
	info_header.biWidth = width;
	info_header.biHeight = -height;
	info_header.biPlanes = 1;
	info_header.biBitCount = 8;
	info_header.biCompression = BI_RGB;
	info_header.biSizeImage = static_cast<DWORD>(stride * height);

	for (uint32_t i = 0; i <= 0xffu; i++)
		color_index[i] = { palette[i].b,palette[i].g,palette[i].r,0 };

	file.write(reinterpret_cast<char*>(&file_header), sizeof file_header);
	file.write(reinterpret_cast<char*>(&info_header), sizeof info_header);
	file.write(reinterpret_cast<char*>(color_index), sizeof color_index);

	for (uint32_t l = 0; l < static_cast<uint32_t>(height); l++)
	{
		file.write(reinterpret_cast<char*>(&cache.cache[l * width]), width);
		file.write(reinterpret_cast<char*>(&cache.cache[l * width]), stride - width);
	}

	file.flush();
	file.close();

	info_header.biWidth = swidth;
	info_header.biHeight = -sheight;
	info_header.biSizeImage = static_cast<DWORD>(sstride * sheight);

	file.open(filename + "_shadow.bmp", std::ios::out | std::ios::binary);
	file.write(reinterpret_cast<char*>(&file_header), sizeof file_header);
	file.write(reinterpret_cast<char*>(&info_header), sizeof info_header);
	file.write(reinterpret_cast<char*>(color_index), sizeof color_index);

	for (uint32_t l = 0; l < static_cast<uint32_t>(sheight); l++)
	{
		file.write(reinterpret_cast<char*>(&shadow.cache[l * swidth]), swidth);
		file.write(reinterpret_cast<char*>(&shadow.cache[l * swidth]), sstride - swidth);
	}

	file.close();

	return true;
}

int main()
{
	const std::string indir = "E:\\";
	const std::string vplname = "voxels.vpl";
	std::string palname = "unittem.pal";
	std::string vxlname = "bfrt.vxl";
	std::string hvaname = "bfrt.hva";
	const char* outdir = "E:\\out_shot\\";

	thomas::vxlfile file;
	thomas::palette palette(indir + palname);
	
	if (thomas::vplfile::load_global(indir + vplname))
		std::cout << "Vpl is loaded.\n";

	file.load(indir + vxlname);
	file.load_hva(indir + hvaname);

	if (file.prepare_all_cache(thomas::vplfile::global, 0, 0, 0))
		std::cout << "All caches engaged.\n";

	if (!file.is_loaded())
		std::cout << "File is not loaded.\n";
	else
		std::cout << "File is loaded.\n";

	file.print_info();
	if (!CreateDirectory(outdir, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		std::cout << "Directory not created.\n";
		return 1;
	}

	for (size_t i = 0; i < thomas::vxlfile::direction_count; i++)
	{
		save_cache_to_bitmap(outdir + std::string("out_") + std::to_string(i), file, palette, i);
	}

	return 0;
}