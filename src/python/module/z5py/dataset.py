import numpy as np
import numbers
from ._z5py import DatasetImpl, open_dataset, create_dataset
from .attribute_manager import AttributeManager


class Dataset(object):

    # FIXME for now we hardcode all compressors
    # but we should instead check which ones are present
    # (similar to nifty WITH_CPLEX, etc.)
    compressors_zarr = ['raw', 'blosc', 'zlib', 'bzip2']
    compressors_n5 = ['raw', 'gzip', 'bzip2']
    zarr_default_compressor = 'blosc'
    n5_default_compressor = 'gzip'

    def __init__(self, path, dset_impl):
        assert isinstance(dset_impl, DatasetImpl)
        self._impl = dset_impl
        self._attrs = AttributeManager(path, self._impl.is_zarr)

    @classmethod
    def create_dataset(cls,
                       path,
                       dtype,
                       shape,
                       chunks,
                       is_zarr,
                       fill_value,
                       compressor,
                       codec,
                       level,
                       shuffle):
        if is_zarr and compressor not in cls.compressors_zarr:
            compressor = cls.zarr_default_compressor
        elif not is_zarr and compressor not in cls.compressors_n5:
            compressor = cls.n5_default_compressor

        return cls(path, create_dataset(path,
                                        dtype,
                                        list(shape),
                                        list(chunks),
                                        is_zarr,
                                        fill_value,
                                        compressor,
                                        codec,
                                        level,
                                        shuffle))

    @classmethod
    def open_dataset(cls, path):
        return cls(path, open_dataset(path))

    @property
    def is_zarr(self):
        return self._impl.is_zarr

    @property
    def attrs(self):
        return self._attrs

    @property
    def shape(self):
        return tuple(self._impl.shape) if self.is_zarr else \
            tuple(self._impl.shape[::-1])

    @property
    def ndim(self):
        return self._impl.ndim

    @property
    def size(self):
        return self._impl.size

    @property
    def chunks(self):
        return tuple(self._impl.chunks)

    @property
    def dtype(self):
        return np.dtype(self._impl.dtype)

    def __len__(self):
        return self._impl.len

    # compute the roi for zarr (straighforward)
    def _index_to_roi_zarr(self, index):
        # get the roi begin and shape from the slicing
        roi_begin = [
            (0 if index[d].start is None else index[d].start)
            if d < len(index) else 0 for d in range(self.ndim)
        ]
        roi_shape = tuple(
            ((self.shape[d] if index[d].stop is None else index[d].stop)
             if d < len(index) else self.shape[d]) - roi_begin[d] for d in range(self.ndim)
        )
        return roi_begin, roi_shape

    # compute the roi for n5
    # ranges must be reversed due to different axis convention
    def _index_to_roi_n5(self, index):
        roi_begin, roi_shape = self._index_to_roi_zarr(index)
        return roi_begin[::-1], roi_shape[::-1]

    # TODO support ellipsis
    def index_to_roi(self, index):

        # check index types of index and normalize the index
        assert isinstance(index, (slice, tuple)), \
            "z5py.Dataset: index must be slice or tuple of slices"
        index_ = (index,) if isinstance(index, slice) else index

        # check lengths of index
        assert len(index_) <= self.ndim, "z5py.Dataset: index is longer than dimension"

        # check the individual slices
        assert all(isinstance(ii, slice) for ii in index_), \
                "z5py.Dataset: index must be slice or tuple of slices"
        assert all(ii.step is None for ii in index_), \
                "z5py.Dataset: slice with non-trivial step is not supported"
        return self._index_to_roi_zarr(index_) if self.is_zarr else \
                self._index_to_roi_n5(index_)

    # most checks are done in c++
    def __getitem__(self, index):
        roi_begin, shape = self.index_to_roi(index)
        out = np.zeros(shape, dtype=self.dtype)
        self._impl.read_subarray(out, roi_begin)
        # n5 output must be transposed due to different axis convention
        return out if self.is_zarr else out.transpose()

    # most checks are done in c++
    def __setitem__(self, index, item):
        assert isinstance(item, (numbers.Number, np.ndarray))
        roi_begin, shape = self.index_to_roi(index)

        # n5 input must be transpsed due to different axis convention
        # write the complete array
        if isinstance(item, np.ndarray):
            assert item.ndim == self.ndim, \
                "z5py.Dataset: complicated broadcasting is not supported"
            self._impl.write_subarray(item if self.is_zarr else item.transpose(), roi_begin)

        # broadcast scalar
        else:
            self._impl.write_scalar(roi_begin, list(shape), item)
