#pragma once

namespace zarr {
namespace compression {

    // abstract basis class for compression
    class CompressorBase {

    public:
        //
        // API -> must be implemented by child classes
        //

        template<typename T>
        virtual int64_t compress(const T *, T *, size_t, size_t) const = 0;

        template<typename T>
        virtual int64_t decompress(const T *, T *, size_t) const = 0;

    };


}
} // namespace::zarr
