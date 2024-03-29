#! /usr/bin/env python
# coding=utf-8

import os
import sys
import shutil

g_proto_path = ""
g_code_path = ""

class SheetClassInfo:
    def __init__(self, proto_name, class_name, file_name):
        self.proto_name = proto_name
        self.class_name = class_name
        self.file_name = file_name

def gen_header_file(proto_name, header_file):
    #首字母调整为大写
    new_proto_name = proto_name.capitalize()
    content = ''
    #include
    content += '#pragma once\n\n'
    content += '#include "sheet_reader.h"\n'
    content += '#include <' + proto_name + '.pb.h>\n\n' 
    #new item
    content += 'struct ' + new_proto_name + 'Item : public sheet::' + proto_name + ' {\n'
    content += '};\n\n'
    #reader
    content += 'struct ' + new_proto_name + 'Reader : public SheetReader<SheetKey<>, ' + new_proto_name + 'Item, sheet::' \
        + proto_name + ', sheet::' + proto_name + '_array> {\n'
    content += 'protected:\n'
    content += '    bool parser(SheetKey<>& key, ' + new_proto_name + 'Item& new_item, sheet::' + proto_name + '& item);\n'
    content += '};\n'

    #delete old file
    if os.path.exists(header_file):
        os.remove(header_file)
    #gen
    f = open(header_file, "w")
    f.write(content)
    f.close()
    

def gen_source_file(proto_name, source_file):
    new_proto_name = proto_name.capitalize()
    content = ''
    #include
    content += '#include "' + proto_name + '_sheet_reader.h"\n\n'
    #reader
    content += 'bool ' + new_proto_name + 'Reader::parser(SheetKey<>& key, ' + new_proto_name + 'Item& new_item, sheet::' + proto_name + '& item) {\n'
    content += '    return false;\n'
    content += '};\n'

    #delete old file
    if os.path.exists(source_file):
        os.remove(source_file)
    #gen
    f = open(source_file, "w")
    f.write(content)
    f.close()

def gen_mgr_f_file(class_infos, mgr_file):
    content = ''
    content += '#pragma once\n\n'
    #include
    content += '#include <assert.h>\n'
    content += '#include <memory>\n'
    for cls in class_infos:
        content += '#include "' + cls.file_name + '"\n'
    content += '\n'

    #mgr class
    content += 'class SheetMgr {\n'
    content += 'public:\n'
    content += '    bool Load(const std::string& dir_path);\n\n'
    content += '    bool Check();\n\n'

    content += 'public:\n'
    for cls in class_infos:
        content += '    std::shared_ptr<' + cls.class_name + '> m' + cls.class_name + ';\n'
    content += '};\n'
    
    #delete old file
    if os.path.exists(mgr_file):
        os.remove(mgr_file)
    #gen
    f = open(mgr_file, "w")
    f.write(content)
    f.close()

def gen_mgr_c_file(class_infos, mgr_file):
    content = ''
    #include 
    content += '#include "sheet_mgr.h"\n\n'

    #mgr class
    content += 'bool SheetMgr::Load(const std::string& dir_path) {\n'
    content += '    std::string file_path;\n'
    content += '    bool ret = false;\n\n'
    for cls in class_infos:
        content += '    file_path = dir_path + "/' +  cls.proto_name + '.conf";\n'
        content += '    m' + cls.class_name + ' = std::make_shared<' + cls.class_name + '>();\n'
        content += '    assert(ret = m' + cls.class_name + '>Load(file_path.c_str()));\n'
        content += '    if (ret == false) { return false; }\n' 
        content += '\n'
    
    content += '    return Check();\n'
    content += '}\n\n'

    content += 'bool SheetMgr::Check() {\n'
    content += '    bool ret = false;\n\n'
    for cls in class_infos:
        content += '    assert(ret = m' +  cls.class_name + '->check());\n'
        content += '    if (ret == false) { return false; }\n' 
        content += '\n'

    content += '    return true;\n'
    content += '}\n\n'

    #delete old file
    if os.path.exists(mgr_file):
        os.remove(mgr_file)
    #gen
    f = open(mgr_file, "w")
    f.write(content)
    f.close()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: param is too less for gen xls mgr code")
        print("python3 gen_xls_mgr.py proto_path code_path")
        sys.exit(-1)

    dirname = os.path.dirname(sys.argv[0])
    g_proto_path = sys.argv[1]
    g_code_path = sys.argv[2]

    g_proto_path = os.path.join(g_proto_path, '')
    g_code_path = os.path.join(g_code_path, '')
    #print("%s:%s" % (g_proto_path, g_code_path))
    
    if not os.path.exists(g_code_path):
        os.makedirs(g_code_path)

    #copy sheet_reader.h
    shutil.copy(dirname + '/sheet_reader.h', g_code_path)

    #迭代proto文件
    proto_names = []
    files = os.listdir(g_proto_path)
    for f in files:
        if f.endswith('.proto'):
            proto_names.append(f)

    class_infos = []
    for name in proto_names:
        new_name = os.path.splitext(name)[0]
        h_f = g_code_path + new_name + '_sheet_reader.h'
        c_f = g_code_path + new_name + '_sheet_reader.cpp'

        #fill
        cls_info = SheetClassInfo(new_name, new_name.capitalize() + 'Reader', new_name + '_sheet_reader.h')
        class_infos.append(cls_info)

        if os.path.exists(h_f) and os.path.exists(c_f):
            continue

        gen_header_file(new_name, h_f)
        gen_source_file(new_name, c_f)
        #print(h_f)
        #print(c_f)
        
    #gen mgr
    mgr_h_f = g_code_path + 'sheet_mgr.h'
    mgr_c_f = g_code_path + 'sheet_mgr.cpp'
    gen_mgr_f_file(class_infos, mgr_h_f)
    gen_mgr_c_file(class_infos, mgr_c_f)