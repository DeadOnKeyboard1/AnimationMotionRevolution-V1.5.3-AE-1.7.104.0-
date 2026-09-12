"""Read-only PE/CodeView and MSF 7 PDB identity check; Python standard library."""
import argparse
import json
import struct
from pathlib import Path


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def dll_identity(path):
    data = Path(path).read_bytes()
    if data[:2] != b'MZ':
        raise ValueError('Not a PE file')
    pe = u32(data, 0x3c)
    if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', data, pe+4)[0] != 0x8664:
        raise ValueError('Expected AMD64 PE')
    count = struct.unpack_from('<H', data, pe+6)[0]
    opt_size = struct.unpack_from('<H', data, pe+20)[0]
    opt = pe + 24
    if struct.unpack_from('<H', data, opt)[0] != 0x20b:
        raise ValueError('Expected PE32+')
    sections = opt + opt_size

    def raw(rva):
        for index in range(count):
            entry = sections + index*40
            virtual_size, start, size, offset = struct.unpack_from('<IIII', data, entry+8)
            if start <= rva < start + max(virtual_size, size):
                return offset + rva - start
        raise ValueError('RVA not mapped to a section')

    rva, size = struct.unpack_from('<II', data, opt+112+6*8)
    if not rva or size % 28:
        raise ValueError('Missing/invalid debug directory')
    begin = raw(rva)
    identities = set()
    for pos in range(begin, begin+size, 28):
        if u32(data, pos+12) != 2:
            continue
        length, _, offset = struct.unpack_from('<III', data, pos+16)
        if length >= 24 and data[offset:offset+4] == b'RSDS':
            identities.add((data[offset+4:offset+20].hex(), u32(data, offset+20)))
    if len(identities) != 1:
        raise ValueError('Expected one unambiguous RSDS identity')
    return identities.pop()


def pdb_identity(path):
    data = Path(path).read_bytes()
    if data[:32] != b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0':
        raise ValueError('Expected MSF 7 PDB')
    block, _, blocks, directory_size, _, block_map = struct.unpack_from('<6I', data, 32)
    if block not in (512, 1024, 2048, 4096, 8192, 16384, 32768) or blocks*block > len(data):
        raise ValueError('Invalid PDB superblock')

    def read_blocks(indices, length):
        if any(index >= blocks for index in indices):
            raise ValueError('PDB block out of bounds')
        return b''.join(data[i*block:(i+1)*block] for i in indices)[:length]

    n = (directory_size+block-1)//block
    directory = read_blocks(struct.unpack_from(f'<{n}I', data, block_map*block), directory_size)
    streams = u32(directory, 0)
    if streams < 2 or streams > len(directory)//4:
        raise ValueError('Invalid PDB stream directory')
    sizes = struct.unpack_from(f'<{streams}I', directory, 4)
    cursor = 4+streams*4
    for index, size in enumerate(sizes):
        n = 0 if size == 0xffffffff else (size+block-1)//block
        indices = struct.unpack_from(f'<{n}I', directory, cursor)
        cursor += n*4
        if index == 1:
            if size < 28 or size == 0xffffffff:
                raise ValueError('Missing PDB info stream')
            info = read_blocks(indices, size)
            return info[12:28].hex(), u32(info, 8)
    raise ValueError('PDB identity missing')


def verify(dll, pdb):
    expected, actual = dll_identity(dll), pdb_identity(pdb)
    if expected != actual:
        raise ValueError(f'DLL/PDB mismatch: {expected} != {actual}')
    return {'status': 'matched', 'guidBytes': expected[0], 'age': expected[1]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('dll')
    parser.add_argument('pdb')
    args = parser.parse_args()
    print(json.dumps(verify(args.dll, args.pdb)))
