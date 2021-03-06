import os
from .base import Base
from .group import Group
from .dataset import Dataset
import json


class File(Base):

    def __init__(self, path, use_zarr_format=None):

        # check if the file already exists
        # and load it if it does
        if os.path.exists(path):
            zarr_group = os.path.join(path, '.zgroup')
            zarr_array = os.path.join(path, '.zarray')
            is_zarr = os.path.exists(zarr_group) or os.path.exists(zarr_array)
            is_n5 = os.path.exists(os.path.join(path, 'attributes.json'))
            assert is_zarr != is_n5, "z5py.File: existing file does not have a valid format"

            # automatically infering the format
            if use_zarr_format is None:
                use_zarr_format = is_zarr
            # file was opened as zarr file
            elif use_zarr_format:
                assert is_zarr, "z5py.File: can't open n5 file in zarr format"
            else:
                assert is_n5, "z5py.File: can't open zarr file in n5 format"

        # otherwise create a new file
        else:
            assert use_zarr_format is not None, \
                "z5py.File: Cannot infer the file format for non existing file"
            os.mkdir(path)
            meta_file = os.path.join(path, '.zgroup' if use_zarr_format else 'attributes.json')
            with open(meta_file, 'w') as f:
                if use_zarr_format:
                    json.dump({'zarr_format': 2}, f)

        super(File, self).__init__(path, use_zarr_format)

    def create_group(self, key):
        assert key not in self.keys(), \
            "z5py.File.create_group: Group is already existing"
        path = os.path.join(self.path, key)
        return Group.make_group(path, self.is_zarr)

    def __getitem__(self, key):
        path = os.path.join(self.path, key)
        assert os.path.exists(path), \
            "z5py.File.__getitem__: key is already existing"
        if self.is_group(path):
            return Group.open_group(path, self.is_zarr)
        else:
            return Dataset.open_dataset(path)

    # TODO setitem, delete datasets ?
