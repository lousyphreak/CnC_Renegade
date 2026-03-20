#ifndef WWBITPACK_PLATFORM_H
#define WWBITPACK_PLATFORM_H

#include <SDL3/SDL_assert.h>

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#define WWBITPACK_ASSERT(expr) SDL_assert(expr)

namespace wwbitpack {

inline constexpr std::uint32_t kBitsPerByte = CHAR_BIT;
inline constexpr double kEpsilon = 0.0001;

template <typename T>
constexpr std::uint32_t Bit_Depth() noexcept
{
	return static_cast<std::uint32_t>(kBitsPerByte * sizeof(T));
}

inline double Round_To_Nearest(double arg)
{
	if (arg > kEpsilon) {
		return std::floor(arg + 0.5);
	}

	if (arg < -kEpsilon) {
		return std::ceil(arg - 0.5);
	}

	return 0.0;
}

inline std::uint32_t Round_To_UInt32(double arg)
{
	WWBITPACK_ASSERT(arg > -kEpsilon);
	WWBITPACK_ASSERT(arg <= static_cast<double>(std::numeric_limits<std::uint32_t>::max()) + kEpsilon);
	return static_cast<std::uint32_t>(Round_To_Nearest(arg));
}

template <typename T>
std::uint32_t Encode_Uncompressed(T value)
{
	static_assert(sizeof(T) <= sizeof(std::uint32_t), "wwbitpack only supports uncompressed atomics up to 32 bits");

	std::uint32_t encoded = 0;
	std::memcpy(&encoded, &value, sizeof(T));
	return encoded;
}

template <typename T>
T Decode_Uncompressed(std::uint32_t encoded)
{
	static_assert(sizeof(T) <= sizeof(std::uint32_t), "wwbitpack only supports uncompressed atomics up to 32 bits");

	T value{};
	std::memcpy(&value, &encoded, sizeof(T));
	return value;
}

} // namespace wwbitpack

#endif // WWBITPACK_PLATFORM_H
