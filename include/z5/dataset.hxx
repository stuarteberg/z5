#pragma once

#include <memory>

#include "z5/metadata.hxx"
#include "z5/handle/handle.hxx"
#include "z5/types/types.hxx"
#include "z5/util/util.hxx"

// different compression backends
#include "z5/compression/raw_compressor.hxx"
#include "z5/compression/blosc_compressor.hxx"
#include "z5/compression/zlib_compressor.hxx"
#include "z5/compression/bzip2_compressor.hxx"

// different io backends
#include "z5/io/io_zarr.hxx"
#include "z5/io/io_n5.hxx"

namespace z5 {

    // Abstract basis class for the zarr-arrays
    class Dataset {

    public:

        //
        // API
        //

        // we need to use void pointer here to have a generic API
        // write a chunk
        virtual void writeChunk(const types::ShapeType &, const void *) const = 0;
        // read a chunk
        virtual void readChunk(const types::ShapeType &, void *) const = 0;

        // helper functions for multiarray API
        virtual void checkRequestShape(const types::ShapeType &, const types::ShapeType &) const = 0;
        virtual void checkRequestType(const std::type_info &) const = 0;
        virtual void getChunkRequests(
            const types::ShapeType &,
            const types::ShapeType &,
            std::vector<types::ShapeType> &) const = 0;
        virtual bool getCoordinatesInRequest(
            const types::ShapeType &,
            const types::ShapeType &,
            const types::ShapeType &,
            types::ShapeType &,
            types::ShapeType &,
            types::ShapeType &) const = 0;

        // size and shape of an actual chunk
        virtual size_t getChunkSize(const types::ShapeType &) const = 0;
        virtual void getChunkShape(const types::ShapeType &, types::ShapeType &) const = 0;
        virtual size_t getChunkShape(const types::ShapeType &, const unsigned) const = 0;

        // maximal chunk size and shape
        virtual size_t maxChunkSize() const = 0;
        virtual const types::ShapeType & maxChunkShape() const = 0;
        virtual size_t maxChunkShape(const unsigned) const = 0;

        // shapes and dimension
        virtual unsigned dimension() const = 0;
        virtual const types::ShapeType & shape() const = 0;
        virtual size_t shape(const unsigned) const = 0;
        virtual size_t size() const = 0;

        virtual size_t numberOfChunks() const = 0;
        virtual const types::ShapeType & chunksPerDimension() const = 0;
        virtual size_t chunksPerDimension(const unsigned) const = 0;

        // dtype
        virtual types::Datatype getDtype() const = 0;
        virtual bool isZarr() const = 0;
        virtual types::Compressor getCompressor() const = 0;
        virtual void getCodec(std::string &) const = 0;
        virtual const handle::Dataset & handle() const = 0;
    };



    template<typename T>
    class DatasetTyped : public Dataset {

    public:

        // create a new array with metadata
        DatasetTyped(
            const handle::Dataset & handle,
            const DatasetMetadata & metadata) : handle_(handle) {

            // make sure that the file does not exist already
            if(handle.exists()) {
                throw std::runtime_error(
                    "Creating a new Dataset failed because file already exists."
                );
            }
            init(metadata);
            handle.createDir();
            writeMetadata(handle, metadata);
        }


        // open existing array
        DatasetTyped(const handle::Dataset & handle) : handle_(handle) {

            // make sure that the file exists
            if(!handle.exists()) {
                throw std::runtime_error(
                    "Opening an existing Dataset failed because file does not exists."
                );
            }
            DatasetMetadata metadata;
            readMetadata(handle, metadata);
            init(metadata);
        }


        virtual inline void writeChunk(const types::ShapeType & chunkIndices, const void * dataIn) const {
            handle::Chunk chunk(handle_, chunkIndices, isZarr_);
            writeChunk(chunk, dataIn);
        }


        // read a chunk
        // IMPORTANT we assume that the data pointer is already initialized up to chunkSize_
        virtual inline void readChunk(const types::ShapeType & chunkIndices, void * dataOut) const {
            handle::Chunk chunk(handle_, chunkIndices, isZarr_);
            readChunk(chunk, dataOut);
        }


        virtual void checkRequestShape(const types::ShapeType & offset, const types::ShapeType & shape) const {
            if(offset.size() != shape_.size() || shape.size() != shape_.size()) {
                throw std::runtime_error("Request has wrong dimension");
            }
            for(int d = 0; d < shape_.size(); ++d) {
                if(offset[d] + shape[d] > shape_[d]) {
                    throw std::runtime_error("Request is out of range");
                }
                if(shape[d] == 0) {
                    throw std::runtime_error("Request shape has a zero entry");
                }
            }
        }


        virtual void checkRequestType(const std::type_info & type) const {
            if(type != typeid(T)) {
                std::cout << "Mytype: " << typeid(T).name() << " your type: " << type.name() << std::endl;
                throw std::runtime_error("Request has wrong type");
            }
        }


        virtual void getChunkRequests(
            const types::ShapeType & offset,
            const types::ShapeType & shape,
            std::vector<types::ShapeType> & chunkRequests) const {

            size_t nDim = offset.size();
            // iterate over the dimension and find the min and max chunk ids
            types::ShapeType minChunkIds(nDim);
            types::ShapeType maxChunkIds(nDim);
            size_t endCoordinate, endId;
            for(int d = 0; d < nDim; ++d) {
                // integer division is ok for both min and max-id, because
                // the chunk is labeled by it's lowest coordinate
                minChunkIds[d] = offset[d] / chunkShape_[d];
                endCoordinate = (offset[d] + shape[d]);
                endId = endCoordinate / chunkShape_[d];
                maxChunkIds[d] = (endCoordinate % chunkShape_[d] == 0) ? endId - 1 : endId;
            }

            util::makeRegularGrid(minChunkIds, maxChunkIds, chunkRequests);
        }

        virtual bool getCoordinatesInRequest(
            const types::ShapeType & chunkId,
            const types::ShapeType & offset,
            const types::ShapeType & shape,
            types::ShapeType & localOffset,
            types::ShapeType & localShape,
            types::ShapeType & inChunkOffset) const {

            localOffset.resize(offset.size());
            localShape.resize(offset.size());
            inChunkOffset.resize(offset.size());

            types::ShapeType chunkShape;
            getChunkShape(chunkId, chunkShape);

            bool completeOvlp = true;
            size_t chunkBegin, chunkEnd, requestEnd;
            int offDiff, endDiff;
            for(int d = 0; d < offset.size(); ++d) {

                // first we calculate the chunk begin and end coordinates
                chunkBegin = chunkId[d] * chunkShape_[d];
                chunkEnd = chunkBegin + chunkShape[d];
                // next the request end
                requestEnd = offset[d] + shape[d];
                // then we calculate the difference between the chunk begin / end
                // and the request begin / end
                offDiff = chunkBegin - offset[d];
                endDiff = requestEnd - chunkEnd;

                // if the offset difference is negative, we are at a starting chunk
                // that is not completely overlapping
                // -> set all values accordingly
                if(offDiff < 0) {
                    localOffset[d] = 0; // start chunk -> no local offset
                    inChunkOffset[d] = -offDiff;
                    completeOvlp = false;
                    // if this chunk is the beginning chunk as well as the end chunk,
                    // we need to adjust the local shape accordingly
                    localShape[d] = (chunkEnd <= requestEnd) ? chunkEnd - offset[d] : requestEnd - offset[d];
                }

                // if the end difference is negative, we are at a last chunk
                // that is not completely overlapping
                // -> set all values accordingly
                else if(endDiff < 0) {
                    localOffset[d] = chunkBegin - offset[d];
                    inChunkOffset[d] = 0;
                    completeOvlp = false;
                    localShape[d] = requestEnd - chunkBegin;

                }

                // otherwise we are at a completely overlapping chunk
                else {
                    localOffset[d] = chunkBegin - offset[d];
                    inChunkOffset[d] = 0;
                    localShape[d] = chunkShape[d];
                }

            }
            return completeOvlp;
        }

        virtual void getChunkShape(const types::ShapeType & chunkId, types::ShapeType & chunkShape) const {
            handle::Chunk chunk(handle_, chunkId, isZarr_);
            getChunkShape(chunk, chunkShape);
        }
        
        virtual size_t getChunkShape(const types::ShapeType & chunkId, const unsigned dim) const {
            handle::Chunk chunk(handle_, chunkId, isZarr_);
            return getChunkShape(chunk, dim);
        }
        
        virtual size_t getChunkSize(const types::ShapeType & chunkId) const {
            handle::Chunk chunk(handle_, chunkId, isZarr_);
            return getChunkSize(chunk);
        }

        // shapes and dimension
        virtual unsigned dimension() const {return shape_.size();}
        virtual const types::ShapeType & shape() const {return shape_;}
        virtual size_t shape(const unsigned d) const {return shape_[d];}
        virtual const types::ShapeType & maxChunkShape() const {return chunkShape_;}
        virtual size_t maxChunkShape(const unsigned d) const {return chunkShape_[d];}

        virtual size_t numberOfChunks() const {return numberOfChunks_;}
        virtual const types::ShapeType & chunksPerDimension() const {return chunksPerDimension_;}
        virtual size_t chunksPerDimension(const unsigned d) const {return chunksPerDimension_[d];}
        virtual size_t maxChunkSize() const {return chunkSize_;}
        virtual size_t size() const {
            return std::accumulate(shape_.begin(), shape_.end(), 1, std::multiplies<size_t>());
        }

        // stuff
        virtual types::Datatype getDtype() const {return dtype_;}
        virtual bool isZarr() const {return isZarr_;}
        virtual types::Compressor getCompressor() const {return compressor_->type();}
        virtual void getCodec(std::string & codec) const {
            compressor_->getCodec(codec);
        };
        virtual const handle::Dataset & handle() const {return handle_;}

        // delete copy constructor and assignment operator
        // because the compressor cannot be copied by default
        // and we don't really need this to be copyable afaik
        // if this changes at some point, we need to provide a proper
        // implementation here
        DatasetTyped(const DatasetTyped & that) = delete;
        DatasetTyped & operator=(const DatasetTyped & that) = delete;

    private:

        //
        // member functions
        //
        void init(const DatasetMetadata & metadata) {

            // zarr or n5 array?
            isZarr_ = metadata.isZarr;

            // dtype
            dtype_ = metadata.dtype;
            // get shapes and fillvalue
            shape_ = metadata.shape;
            chunkShape_ = metadata.chunkShape;
            chunkSize_ = std::accumulate(
                chunkShape_.begin(), chunkShape_.end(), 1, std::multiplies<size_t>()
            );
            fillValue_ = static_cast<T>(metadata.fillValue);

            // TODO add more compressors
            switch(metadata.compressor) {
                case types::raw:
            	    compressor_.reset(new compression::RawCompressor<T>()); break;
                #ifdef WITH_BLOSC
                case types::blosc:
            	    compressor_.reset(new compression::BloscCompressor<T>(metadata)); break;
                #endif
                #ifdef WITH_ZLIB
                case types::zlib:
            	    compressor_.reset(new compression::ZlibCompressor<T>(metadata)); break;
                #endif
                #ifdef WITH_BZIP2
                case types::bzip2:
            	    compressor_.reset(new compression::Bzip2Compressor<T>(metadata)); break;
                #endif
            }

            // chunk writer
            if(isZarr_) {
                io_.reset(new io::ChunkIoZarr<T>());
            } else {
                io_.reset(new io::ChunkIoN5<T>(shape_, chunkShape_));
            }

            // get chunk specifications
            for(size_t d = 0; d < shape_.size(); ++d) {
                chunksPerDimension_.push_back(
                    shape_[d] / chunkShape_[d] + (shape_[d] % chunkShape_[d] == 0 ? 0 : 1)
                );
            }
            numberOfChunks_ = std::accumulate(
                chunksPerDimension_.begin(), chunksPerDimension_.end(), 1, std::multiplies<size_t>()
            );
        }


        // write a chunk
        inline void writeChunk(const handle::Chunk & chunk, const void * dataIn) const {

            // make sure that we have a valid chunk
            checkChunk(chunk);

            // get the correct chunk size and declare the out data
            size_t chunkSize = isZarr_ ? chunkSize_ : io_->getChunkSize(chunk);
            std::vector<T> dataOut;

            // reverse the endianness if necessary
            if(sizeof(T) > 1 && !isZarr_) {

                // copy the data and reverse endianness
                std::vector<T> dataTmp(static_cast<const T*>(dataIn), static_cast<const T*>(dataIn) + chunkSize);
                util::reverseEndiannessInplace<T>(dataTmp.begin(), dataTmp.end());

                // compress the data
                compressor_->compress(&dataTmp[0], dataOut, chunkSize);

            } else {

                // compress the data
                compressor_->compress(static_cast<const T*>(dataIn), dataOut, chunkSize);

            }

            // write the data
            io_->write(chunk, dataOut);
        }


        inline void readChunk(const handle::Chunk & chunk, void * dataOut) const {

            // make sure that we have a valid chunk
            checkChunk(chunk);

            // read the data
            std::vector<T> dataTmp;
            auto chunkExists = io_->read(chunk, dataTmp);

            // if the chunk exists, decompress it
            // otherwise we return the chunk with fill value
            if(chunkExists) {

                size_t chunkSize = isZarr_ ? chunkSize_ : io_->getChunkSize(chunk);
                compressor_->decompress(dataTmp, static_cast<T*>(dataOut), chunkSize);

                // reverse the endianness for N5 data
                // TODO actually check that the file endianness is different than the system endianness
                if(sizeof(T) > 1 && !isZarr_) { // we don't need to convert single bit numbers
                    util::reverseEndiannessInplace<T>(
                        static_cast<T*>(dataOut), static_cast<T*>(dataOut) + chunkSize);
                }

            }

            else {
                size_t chunkSize = isZarr_ ? chunkSize_ : io_->getChunkSize(chunk);
                std::fill(static_cast<T*>(dataOut), static_cast<T*>(dataOut) + chunkSize, fillValue_);
            }
        }


        // check that the chunk handle is valid
        inline void checkChunk(const handle::Chunk & chunk) const {
            // check dimension
            const auto & chunkIndices = chunk.chunkIndices();
            if(chunkIndices.size() != shape_.size()) {
                throw std::runtime_error("Invalid chunk dimension");
            }

            // check chunk dimensions
            for(int d = 0; d < shape_.size(); ++d) {
                if(chunkIndices[d] >= chunksPerDimension_[d]) {
                    throw std::runtime_error("Invalid chunk index");
                }
            }
        }


        inline void getChunkShape(const handle::Chunk & chunk, types::ShapeType & chunkShape) const {
            chunkShape.resize(shape_.size());
            // zarr has a fixed chunkShpae, whereas n5 has variable chunk shape
            if(isZarr_) {
                std::copy(chunkShape_.begin(), chunkShape_.end(), chunkShape.begin());
            } else {
                io_->getChunkShape(chunk, chunkShape);
            }
        }

        inline size_t getChunkShape(const handle::Chunk & chunk, const unsigned dim) const {
            // zarr has a fixed chunkShpae, whereas n5 has variable chunk shape
            if(isZarr_) {
                return maxChunkShape(dim);
            } else {
                std::vector<size_t> tmpShape;
                getChunkShape(chunk, tmpShape);
                return tmpShape[dim];
            }
        }

        inline size_t getChunkSize(const handle::Chunk & chunk) const {
            // zarr has a fixed chunkShpae, whereas n5 has variable chunk shape
            if(isZarr_) {
                return maxChunkSize();
            } else {
                std::vector<size_t> tmpShape;
                getChunkShape(chunk, tmpShape);
                return std::accumulate(tmpShape.begin(), tmpShape.end(), 1, std::multiplies<size_t>());
            }
        }


        //
        // member variables
        //

        // our handle
        handle::Dataset handle_;

        // unique ptr to hold child classes of compressor
        std::unique_ptr<compression::CompressorBase<T>> compressor_;

        // unique prtr chunk writer
        std::unique_ptr<io::ChunkIoBase<T>> io_;

        // flag to store whether the chunks are in zarr or n5 encoding
        bool isZarr_;

        // the datatype encoding
        types::Datatype dtype_;
        // the shape of the array
        types::ShapeType shape_;
        // the chunk-shape of the arrays
        types::ShapeType chunkShape_;
        // the chunk size and the chunk size in bytes
        size_t chunkSize_;
        // the fill value
        T fillValue_;
        // the number of chunks and chunks per dimension
        size_t numberOfChunks_;
        types::ShapeType chunksPerDimension_;
    };

} // namespace::z5
