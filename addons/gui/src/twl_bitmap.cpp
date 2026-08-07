#include <Windows.h>
#include <string>
#include <format>
#include <algorithm>
#include <memory>
#include <vector>
#include "lua.hpp"
#include "luabinder.hpp"
#include "utf.h"
#include "twl_bitmap.hpp"

COLORREF lua_optColor(lua_State* L, int idx = 1, COLORREF def_clr = 0);
void lua_pushcolor(lua_State* L, COLORREF);

twBitMap* lua_checkBitMap(lua_State* L, int idx = 1)
{
	twBitMap* ptwBitMap = check_arg<twBitMap>(L, idx);
	return ptwBitMap ? ptwBitMap : nullptr;
}

namespace
{
	auto GetRGB(COLORREF color)
	{
		return std::make_tuple<uint8_t, uint8_t, uint8_t>(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
	}

	int tbitmap_tostring(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			const std::string name = std::format("{} [{}x{}]", twBitMap::classname(), pBitMap->width(), pBitMap->height());
			lua_pushstring(L, name.c_str());
			return 1;
		}
		return 0;
	}

	int do_reset_bitmap(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			int w = static_cast<int>(luaL_checkinteger(L, 2));
			int h = static_cast<int>(luaL_checkinteger(L, 3));
			pBitMap->reset(w, h);
		}
		return 0;
	}

	int do_set_pixel(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			int x = static_cast<int>(luaL_checkinteger(L, 2));
			int y = static_cast<int>(luaL_checkinteger(L, 3));
			COLORREF color = lua_optColor(L, 4);
			pBitMap->set_pixel(x, y, color);
		}
		return 0;
	}

	int do_get_pixel(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			int x = static_cast<int>(luaL_checkinteger(L, 2));
			int y = static_cast<int>(luaL_checkinteger(L, 3));
			COLORREF color = pBitMap->get_pixel(x, y);
			lua_pushcolor(L, color);
			return 1;
		}
		return 0;
	}

	int do_fill_pixels(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			COLORREF color = lua_optColor(L, 2);
			pBitMap->fill_pixels(color);
		}
		return 0;
	}

	int do_grayscale(lua_State* L)
	{
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			auto r = luaL_optnumber(L, 2, 0.299f);
			auto g = luaL_optnumber(L, 3, 0.587f);
			auto b = luaL_optnumber(L, 4, 0.114f);
			pBitMap->grayscale(r, g, b);
		}
		return 0;
	}

	int do_save_bmp(lua_State* L)
	{
		bool status = false;
		if (twBitMap* pBitMap = lua_checkBitMap(L))
		{
			if (const char* txt = luaL_optstring(L, 2, nullptr))
			{
				status = pBitMap->save_to_bmp(StringFromUTF8(txt).c_str());
			}
		}
		lua_pushboolean(L, status);
		return 1;
	}
}

static int destroy_twbitmap(lua_State* L)
{
	return do_destroy<twBitMap>(L);
}

const luaL_Reg LuaBinder<twBitMap>::metamethods[] =
{
	{ "__gc",		destroy_twbitmap	},
	{ "__close",	destroy_twbitmap	},
	{ "__tostring",	tbitmap_tostring	},
	{ NULL, NULL }
};

const luaL_Reg LuaBinder<twBitMap>::methods[] =
{
	{ "reset",			do_reset_bitmap	},
	{ "set_pixel",		do_set_pixel	},
	{ "get_pixel",		do_get_pixel	},
	{ "fill",			do_fill_pixels	},
	{ "grayscale",		do_grayscale	},
	{ "save_as_bmp",	do_save_bmp		},
	{ NULL, NULL }
};

void lua_openclass_TBitMap(lua_State* L)
{
	LuaBinder<twBitMap>().createClass(L);
}

int new_bitmap(lua_State* L)
{
	int w = static_cast<int>(luaL_checkinteger(L, 1));
	int h = static_cast<int>(luaL_checkinteger(L, 2));
	lua_push_newobject<twBitMap>(L, w, h);
	return 1;
}

////////////////
twBitMap::twBitMap(size_t _w, size_t _h)
{
	reset(_w, _h);
}

void twBitMap::reset(size_t _w, size_t _h)
{
	h = _h;
	w = _w;
	const size_t rawRowSize = _w * bytesPerPixel;                 // без выравнивания
	const size_t rowSize = (rawRowSize + 3) & ~3;                 // выравнивание до 4 байт
	imageSize = rowSize * h;
	data.reset(new uint8_t[imageSize]);
	fill_pixels(RGB(0xFF, 0xFF, 0xFF));
}

size_t twBitMap::position(size_t x, size_t y) const
{
	return (x + y * w) * bytesPerPixel;
}

void twBitMap::set_pixel(size_t x, size_t y, COLORREF color)
{
	size_t pos = position(x, y);
	auto [r, g, b] = GetRGB(color);
	data[pos + 0] = b;
	data[pos + 1] = g;
	data[pos + 2] = r;
	if (bytesPerPixel == 4)
	{
		data[pos + 3] = 0xFF;
	}
}

COLORREF twBitMap::get_pixel(size_t x, size_t y) const
{
	size_t pos = position(x, y);
	return data[pos + 2] | (data[pos + 1] << 8) | (data[pos + 0] << 16);

}

void twBitMap::fill_pixels(COLORREF color)
{
	auto [r, g, b] = GetRGB(color);
	for (size_t pos = 0; pos < imageSize; pos += bytesPerPixel)
	{
		data[pos + 0] = b; // b
		data[pos + 1] = g; // g
		data[pos + 2] = r; // r
		if (bytesPerPixel == 4)
		{
			data[pos + 3] = 0xFF;
		}
	}
}
void twBitMap::grayscale(double r, double g, double b)
{
	int8_t maximum = 0;
	for (size_t pos = 0; pos < imageSize; pos += bytesPerPixel)
	{
		data[pos + 0] = data[pos + 1] = data[pos + 2] =
			static_cast<uint8_t>(data[pos + 0] * r + data[pos + 1] * r + data[pos + 2] * b);
		if (data[pos] > maximum) maximum = data[pos];
	}
	if (maximum < 255)
	{
		for (size_t pos = 0; pos < imageSize; pos += bytesPerPixel)
		{
			data[pos + 0] = data[pos + 1] = data[pos + 2] =
				static_cast<uint8_t>(data[pos] * 255 / maximum);
		}
	}
}

namespace
{
	bool SaveBmpFromByteArray(
		const uint8_t* pixelData,
		int width,
		int height,
		int bytesPerPixel,          // 3 или 4
		const wchar_t* path
	)
	{
		if (bytesPerPixel != 3 && bytesPerPixel != 4)
		{
			return false;
		}

		const int bitsPerPixel = bytesPerPixel * 8;
		const int rawRowSize = width * bytesPerPixel;
		const int rowSize = (rawRowSize + 3) & ~3;                 // выравнивание до 4 байт
		const size_t imageSize = static_cast<size_t>(rowSize) * static_cast<size_t>(height);

		BITMAPFILEHEADER bfh =
		{
			.bfType = 0x4D42, // 'BM'
			.bfSize = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize),
			.bfReserved1 = 0,
			.bfReserved2 = 0,
			.bfOffBits = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
		};

		BITMAPINFOHEADER bi =
		{
			.biSize = static_cast<DWORD>(sizeof(BITMAPINFOHEADER)),
			.biWidth = width,
			.biHeight = height,
			.biPlanes = 1,
			.biBitCount = static_cast<WORD>(bitsPerPixel),
			.biCompression = BI_RGB,
			.biSizeImage = static_cast<DWORD>(imageSize),
			.biXPelsPerMeter = 2835, // 72 dpi
			.biYPelsPerMeter = 2835, // 72 dpi
			.biClrUsed = 0,
			.biClrImportant = 0
		};

		HANDLE raw = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (raw == INVALID_HANDLE_VALUE) return false;

		auto deleter = [](HANDLE h) noexcept
			{
				if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
			};
		std::unique_ptr<void, decltype(deleter)> hFile(raw, deleter);

		DWORD bytesWritten = 0;
		if (!WriteFile(hFile.get(), &bfh, sizeof(bfh), &bytesWritten, nullptr) || bytesWritten != sizeof(bfh))
			return false;
		if (!WriteFile(hFile.get(), &bi, sizeof(bi), &bytesWritten, nullptr) || bytesWritten != sizeof(bi))
			return false;

		std::vector<uint8_t> rowBuffer(rowSize, 0);
		for (int y = height - 1; y >= 0; --y) {
			const uint8_t* src = pixelData + static_cast<size_t>(y) * static_cast<size_t>(rawRowSize);
			uint8_t* dst = rowBuffer.data();

			for (int x = 0; x < width; ++x)
			{
				std::memcpy(dst, src, bytesPerPixel);
				src += bytesPerPixel;
				dst += bytesPerPixel;
			}

			if (!WriteFile(hFile.get(), rowBuffer.data(), rowSize, &bytesWritten, nullptr) ||
				bytesWritten != static_cast<DWORD>(rowSize)) {
				return false;
			}
		}
		return true;
	}
}

bool twBitMap::save_to_bmp(const wchar_t* path) const
{
	return SaveBmpFromByteArray(data.get(), (int)w, (int)h, bytesPerPixel, path);
}
