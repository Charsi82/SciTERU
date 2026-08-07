#pragma once

class twBitMap
{
	size_t w{1};
	size_t h{1};
	size_t imageSize{};
	std::unique_ptr<uint8_t[]> data;
	size_t position(size_t x, size_t y) const;
	static constexpr size_t bytesPerPixel = 4;

public:
	twBitMap(size_t _w, size_t _h);
	static const char* classname() { return "twBitMap"; }
	UINT getBytesPerPixel() const { return bytesPerPixel; }
	size_t width() const { return w; }
	size_t height() const { return h; }
	void reset(size_t _w, size_t _h);
	void set_pixel(size_t _w, size_t _h, COLORREF color);
	COLORREF get_pixel(size_t x, size_t y) const;
	void fill_pixels(COLORREF color);
	void grayscale(double r, double g, double b);
	bool save_to_bmp(const wchar_t* path) const;
	const void* pixels() const { return data.get(); };
};