#! /usr/bin/env python
# coding=utf-8

import os
import platform
import xlrd
import sys
import shutil
import argparse

# 遍历sheet
def iteratorSheet(xls_file_path, proto_path, data_path):
    print(xls_file_path)
    book = xlrd.open_workbook(xls_file_path)
    for sheet in book.sheet_names():
        cmd = 'python3 xls_translator.py ' + sheet + ' ' + xls_file_path + ' ' + proto_path + ' ' + data_path
        os.system(cmd)

def protocFile(proto_path):
    files = os.listdir(proto_path)
    for f in files:
        if not f.endswith('.proto'):
            continue
        
        command = None
        if platform.system() == 'Windows':
            command = "..\\bin\\protoc.exe "
            command += "--proto_path=" + proto_path + " "
            command += "--cpp_out=" + proto_path + " "
            command += f
        else:
            command =  "cd " + proto_path + ";"
            command += "protoc --cpp_out=./ "
            command += f
        os.system(command)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--excel', required=True,  help='the excel directory path')
    parser.add_argument('--proto', required=True,  help='the proto directory path')
    parser.add_argument('--data',  required=True,  help='the data directory path')
    parser.add_argument('--code',  required=False, help='the code directory path')

    parser.print_help()
    args = parser.parse_args()
    
    # 生成proto_path目录
    if not os.path.exists(args.proto):
        os.makedirs(args.proto)

    # 迭代excel目录
    files = os.listdir(args.excel)
    for f in files:
        if f.startswith('~.') or f.startswith("~$"):
            continue
        if not f.endswith('.xlsx') and not f.endswith('.xls'):
            continue
        iteratorSheet(args.excel + f, args.proto, args.data)
    
    # 生成c++版本的proto源文件
    protocFile(args.proto)
    
    #删除无用文件
    pb_files = os.listdir(args.proto)
    for f in pb_files:
        if f.endswith('.py'):
            os.remove(args.proto + f)
        elif f.endswith('__pycache__'):
            shutil.rmtree(args.proto + f)

    data_files = os.listdir(args.data)
    for f in data_files:
        if f.endswith('.data'):
            os.remove(args.data + f)

    #生成管理器代码
    cmd = "python3 ./gen_xls_mgr.py " + args.proto + " " + args.code
    #print("%s" % (cmd))
    os.system(cmd)


