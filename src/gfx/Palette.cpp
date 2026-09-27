#include "Palette.h"

#include <algorithm>

sf::Color hex(unsigned v) { return sf::Color((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF); }

static const Palette kPalettes[] = {
    // skyTop   skyBot    far       mid       near      earth     earthDk   grass     grassLt   accent    water     fog
    {hex(0x7EC8E3), hex(0xF2E7B5), hex(0x8FBF9F), hex(0x4F9A5E), hex(0x2E6B3A), hex(0x7A5232), hex(0x4E3320),
     hex(0x6DBE45), hex(0xB4E86A), hex(0xF4A93B), hex(0x4FA5C9), hex(0xDFF1C8)}, // canopy
    {hex(0x9AD6C8), hex(0xF6F0C4), hex(0x7DBF9E), hex(0x3F8E63), hex(0x20553C), hex(0x6E4A2D), hex(0x46301C),
     hex(0x58B348), hex(0xA6E066), hex(0xE85D9A), hex(0x3F9BB5), hex(0xD6F0DA)}, // grove
    {hex(0xA9C4D8), hex(0xE9D9B0), hex(0x8AA0A8), hex(0x5B6F62), hex(0x334A3A), hex(0x6B5B45), hex(0x3E3527),
     hex(0x7FA84A), hex(0xB8D46A), hex(0xC9433C), hex(0x5B93A8), hex(0xD9DDD0)}, // gully
    {hex(0x66B8E8), hex(0xFFF1BF), hex(0x8CCFB7), hex(0x3E9E7D), hex(0x1F6A4E), hex(0x6C4B2C), hex(0x43301B),
     hex(0x63C24E), hex(0xB2EA6C), hex(0xFF8A3D), hex(0x2F8FC2), hex(0xCFEFEA)}, // river
    {hex(0x5F7A8C), hex(0xB9A6C7), hex(0x6E5F8A), hex(0x4A3F66), hex(0x2B2545), hex(0x5C4A40), hex(0x332822),
     hex(0x4E9E63), hex(0x8CD98A), hex(0xE3B04B), hex(0x3A6E7E), hex(0xB8A9C9)}, // hollow
    {hex(0x8FC3E6), hex(0xF3DDB0), hex(0xB3A38A), hex(0x8A7256), hex(0x5B4A36), hex(0x8A6A47), hex(0x553F2A),
     hex(0x93B14D), hex(0xCBDD7A), hex(0xD9553F), hex(0x4C97B8), hex(0xE6DCC4)}, // ridge
    {hex(0x6FAEE0), hex(0xD9E9F5), hex(0xA5B8C8), hex(0x7189A0), hex(0x3F5568), hex(0x6F6A63), hex(0x3F3B36),
     hex(0x87A85B), hex(0xB6D278), hex(0xF2D268), hex(0x4A8FB8), hex(0xE4ECF2)}, // cliffs
    {hex(0x7E9C7A), hex(0xD6CF95), hex(0x6F8A62), hex(0x4D6942), hex(0x2E4429), hex(0x5E5238), hex(0x383021),
     hex(0x7FA43C), hex(0xB7CF5C), hex(0xB9E356), hex(0x5E7F4A), hex(0xC7C79E)}, // swamp
    {hex(0x8EA5B8), hex(0xE2CFA5), hex(0x8E7F70), hex(0x66594D), hex(0x3E362E), hex(0x7A6046), hex(0x4A3826),
     hex(0x7D9E4B), hex(0xB3C96E), hex(0xC48A3F), hex(0x4B7F96), hex(0xD8CDBA)}, // mill
    {hex(0x3C4A63), hex(0x7F8FA6), hex(0x4E5B72), hex(0x35415A), hex(0x1F2738), hex(0x5A4A3A), hex(0x2E251C),
     hex(0x4F8A47), hex(0x86BF66), hex(0xFFD972), hex(0x365A73), hex(0x8892A6)}, // storm
};

const Palette& paletteFor(Biome b) { return kPalettes[static_cast<int>(b)]; }

sf::Color lerp(sf::Color a, sf::Color b, float t) {
	t = std::clamp(t, 0.f, 1.f);
	auto m = [t](sf::Uint8 x, sf::Uint8 y) { return static_cast<sf::Uint8>(x + (y - x) * t + 0.5f); };
	return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
}

sf::Color withAlpha(sf::Color c, int a) {
	c.a = static_cast<sf::Uint8>(std::clamp(a, 0, 255));
	return c;
}

sf::Color scale(sf::Color c, float f) {
	auto s = [f](sf::Uint8 x) { return static_cast<sf::Uint8>(std::clamp(x * f, 0.f, 255.f)); };
	return sf::Color(s(c.r), s(c.g), s(c.b), c.a);
}
