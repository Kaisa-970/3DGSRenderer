#!/usr/bin/env python3
"""
PLY文件球谐简化工具
将包含高阶球谐分量的PLY文件简化为只保留直流分量的版本

用法:
    python simplify_ply.py input.ply                    # 处理单个文件
    python simplify_ply.py input_dir/                   # 处理目录下所有.ply文件
    python simplify_ply.py input.ply -o output.ply      # 指定输出文件名
"""

import struct
import os
import sys
import argparse
from pathlib import Path


class PLYSimplifier:
    """PLY文件简化器，去除球谐高阶分量"""

    def __init__(self):
        self.properties = []
        self.vertex_count = 0
        self.is_binary = False
        self.endianness = '<'  # little endian

    def parse_header(self, lines):
        """解析PLY文件头"""
        properties = []
        vertex_count = 0
        is_binary = False
        in_vertex_element = False

        for line in lines:
            line = line.strip()

            if line.startswith('format'):
                if 'binary_little_endian' in line:
                    is_binary = True
                    self.endianness = '<'
                elif 'binary_big_endian' in line:
                    is_binary = True
                    self.endianness = '>'

            elif line.startswith('element vertex'):
                parts = line.split()
                vertex_count = int(parts[2])
                in_vertex_element = True

            elif line.startswith('element'):
                in_vertex_element = False

            elif line.startswith('property') and in_vertex_element:
                parts = line.split()
                prop_type = parts[1]
                prop_name = parts[2]
                properties.append((prop_name, prop_type))

            elif line == 'end_header':
                break

        return properties, vertex_count, is_binary

    def should_keep_property(self, prop_name):
        """判断是否保留该属性（去除f_rest_*）"""
        return not prop_name.startswith('f_rest_')

    def get_property_format(self, prop_type):
        """获取属性的struct格式字符"""
        type_map = {
            'float': 'f',
            'double': 'd',
            'uchar': 'B',
            'char': 'b',
            'ushort': 'H',
            'short': 'h',
            'uint': 'I',
            'int': 'i'
        }
        return type_map.get(prop_type, 'f')

    def process_file(self, input_path, output_path):
        """处理单个PLY文件"""
        print(f"处理文件: {input_path}")

        # 读取文件
        with open(input_path, 'rb') as f:
            content = f.read()

        # 找到header结束位置
        header_end = content.find(b'end_header\n') + len(b'end_header\n')
        if header_end == len(b'end_header\n') - 1:
            header_end = content.find(b'end_header\r\n') + len(b'end_header\r\n')

        header_bytes = content[:header_end]
        header_text = header_bytes.decode('utf-8', errors='ignore')
        header_lines = header_text.split('\n')

        # 解析header
        properties, vertex_count, is_binary = self.parse_header(header_lines)
        print(f"  顶点数: {vertex_count}")
        print(f"  格式: {'二进制' if is_binary else 'ASCII'}")
        print(f"  原始属性数: {len(properties)}")

        # 筛选要保留的属性
        kept_properties = [(name, typ) for name, typ in properties if self.should_keep_property(name)]
        removed_count = len(properties) - len(kept_properties)
        print(f"  保留属性数: {len(kept_properties)}")
        print(f"  移除属性数: {removed_count} (f_rest_*)")

        if removed_count == 0:
            print("  警告: 没有找到需要移除的f_rest_*属性")
            return False

        # 生成新的header
        new_header_lines = []
        in_vertex_element = False

        for line in header_lines:
            line = line.strip()

            if line.startswith('element vertex'):
                new_header_lines.append(line)
                in_vertex_element = True

            elif line.startswith('element'):
                in_vertex_element = False
                new_header_lines.append(line)

            elif line.startswith('property') and in_vertex_element:
                parts = line.split()
                prop_name = parts[2]
                if self.should_keep_property(prop_name):
                    new_header_lines.append(line)

            elif line == 'end_header':
                new_header_lines.append(line)
                break

            elif not line.startswith('property'):
                new_header_lines.append(line)

        new_header = '\n'.join(new_header_lines) + '\n'

        # 处理数据
        data_start = header_end

        if is_binary:
            # 二进制格式
            new_data = self.process_binary_data(
                content[data_start:],
                properties,
                kept_properties,
                vertex_count
            )
        else:
            # ASCII格式
            new_data = self.process_ascii_data(
                content[data_start:].decode('utf-8'),
                properties,
                kept_properties,
                vertex_count
            )

        # 写入新文件
        with open(output_path, 'wb') as f:
            f.write(new_header.encode('utf-8'))
            f.write(new_data)

        # 计算文件大小变化
        input_size = os.path.getsize(input_path)
        output_size = os.path.getsize(output_path)
        reduction = (1 - output_size / input_size) * 100

        print(f"  原始大小: {input_size / 1024 / 1024:.2f} MB")
        print(f"  处理后大小: {output_size / 1024 / 1024:.2f} MB")
        print(f"  减少: {reduction:.1f}%")
        print(f"  输出: {output_path}")

        return True

    def process_binary_data(self, data, old_props, new_props, vertex_count):
        """处理二进制数据"""
        # 构建struct格式字符串
        old_format = self.endianness + ''.join(self.get_property_format(t) for _, t in old_props)
        old_size = struct.calcsize(old_format)

        # 找出要保留的属性索引
        kept_indices = [i for i, (name, _) in enumerate(old_props) if self.should_keep_property(name)]

        new_data = bytearray()
        offset = 0

        for i in range(vertex_count):
            if i % 100000 == 0 and i > 0:
                print(f"  处理进度: {i}/{vertex_count} ({i*100//vertex_count}%)")

            # 解包一个顶点的所有数据
            vertex_data = struct.unpack(old_format, data[offset:offset + old_size])

            # 只保留需要的属性
            kept_data = [vertex_data[idx] for idx in kept_indices]

            # 打包并写入
            new_format = self.endianness + ''.join(self.get_property_format(t) for _, t in new_props)
            new_data.extend(struct.pack(new_format, *kept_data))

            offset += old_size

        print(f"  处理进度: {vertex_count}/{vertex_count} (100%)")
        return bytes(new_data)

    def process_ascii_data(self, data, old_props, new_props, vertex_count):
        """处理ASCII数据"""
        lines = data.strip().split('\n')
        kept_indices = [i for i, (name, _) in enumerate(old_props) if self.should_keep_property(name)]

        new_lines = []
        for i, line in enumerate(lines[:vertex_count]):
            if i % 100000 == 0 and i > 0:
                print(f"  处理进度: {i}/{vertex_count} ({i*100//vertex_count}%)")

            values = line.split()
            kept_values = [values[idx] for idx in kept_indices]
            new_lines.append(' '.join(kept_values))

        print(f"  处理进度: {vertex_count}/{vertex_count} (100%)")
        return '\n'.join(new_lines).encode('utf-8') + b'\n'


def main():
    parser = argparse.ArgumentParser(
        description='简化PLY文件：移除球谐高阶分量(f_rest_*)，只保留直流分量(f_dc_*)',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('input', help='输入PLY文件或目录')
    parser.add_argument('-o', '--output', help='输出文件名（仅处理单个文件时有效）')
    parser.add_argument('-s', '--suffix', default='_dc', help='输出文件后缀（默认: _dc）')

    args = parser.parse_args()

    input_path = Path(args.input)
    simplifier = PLYSimplifier()

    # 处理单个文件
    if input_path.is_file():
        if not input_path.suffix.lower() == '.ply':
            print(f"错误: {input_path} 不是PLY文件")
            return 1

        if args.output:
            output_path = Path(args.output)
        else:
            output_path = input_path.parent / f"{input_path.stem}{args.suffix}{input_path.suffix}"

        success = simplifier.process_file(str(input_path), str(output_path))
        return 0 if success else 1

    # 处理目录
    elif input_path.is_dir():
        ply_files = list(input_path.glob('*.ply'))

        if not ply_files:
            print(f"错误: 在 {input_path} 中没有找到PLY文件")
            return 1

        print(f"找到 {len(ply_files)} 个PLY文件\n")

        success_count = 0
        for i, ply_file in enumerate(ply_files, 1):
            print(f"\n[{i}/{len(ply_files)}]")
            output_path = ply_file.parent / f"{ply_file.stem}{args.suffix}{ply_file.suffix}"

            if simplifier.process_file(str(ply_file), str(output_path)):
                success_count += 1

        print(f"\n完成! 成功处理 {success_count}/{len(ply_files)} 个文件")
        return 0

    else:
        print(f"错误: {input_path} 不存在")
        return 1


if __name__ == '__main__':
    sys.exit(main())

