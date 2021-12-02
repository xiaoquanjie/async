#! /usr/bin/env python
# coding=utf-8

import os
import platform
import xlrd
import sys
import shutil
import argparse

# 白名单
g_white_files = []

# 黑名单
g_black_files = []

# 遍历sheet
def iteratorSheet(xls_file_path, file_name, proto_path, data_path):
    print(xls_file_path)
    book = xlrd.open_workbook(xls_file_path)
    dirname = os.path.dirname(sys.argv[0])
    for sheet in book.sheet_names():
        flag = True
        if g_white_files:
            flag = False
            # 存在白名单
            for w_f in g_white_files:
                if w_f[0] == file_name and w_f[1] == sheet:
                    flag = True
                    break

        if not flag:
            # 白名单未通过
            continue

        # 黑名单检查
        for b_f in g_black_files:
            if b_f[0] == file_name and b_f[1] == sheet:
                flag = False
                break
            
        if not flag:
            continue

        cmd = 'python3 '
        cmd += dirname + '/xls_translator.py '
        cmd += '--sheet ' + sheet + ' '
        cmd += '--file ' + xls_file_path + ' '
        if proto_path:
            cmd += '--proto ' + proto_path + ' '
        if data_path:
            cmd += '--data ' + data_path

        if os.system(cmd) != 0:
            raise 'fail' 

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

def parseWhiteBlackFile(files, file_path):
    if file_path == 'white_file':
        file_path = os.path.dirname(sys.argv[0]) + '/white_file'
    if file_path == 'black_file':
        file_path = os.path.dirname(sys.argv[0]) + '/black_file'

    f = open(file_path)
    while True:
        line = f.readline()
        line = line.strip()
        if not line:
            break
        d = line.split(':')
        files.append(d)

    #print(files)

# 清除文件
def clearDirectoryFile(dir_path, suffix):
    if not dir_path:
        return
    if not os.path.exists(dir_path):
        return

    data_files = os.listdir(dir_path)
    for f in data_files:
        if f.endswith(suffix):
            os.remove(dir_path + f)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--excel', required=True,  help='the excel directory path [./excel/]')
    parser.add_argument('--proto', required=False,  help='the proto directory path [./proto/]')
    parser.add_argument('--data',  required=False,  help='the data directory path  [./data/]')
    parser.add_argument('--code',  required=False,  help='the code directory path   [./code/]')
    parser.add_argument('--pb',  required=False,  help='the code directory path   [./code/]')
    parser.add_argument('--white', nargs='?', const='white_file', required=False, help='the white list')
    parser.add_argument('--black', nargs='?', const='black_file', required=False, help='the black list')

    #parser.print_help()
    args = parser.parse_args()
    #print(args)

    # 清除自动生成的文件
    clearDirectoryFile(args.proto, '.proto')
    clearDirectoryFile(args.proto, '.pb.h')
    clearDirectoryFile(args.proto, '.pb.cc')
    clearDirectoryFile(args.data, '.conf')
    
    # 白名单
    if args.white:
        parseWhiteBlackFile(g_white_files, args.white)

    # 黑名单
    if args.black:
        parseWhiteBlackFile(g_black_files, args.black)

    # 迭代excel目录
    files = os.listdir(args.excel)
    for f in files:
        if f.startswith('~.') or f.startswith("~$"):
            continue
        if not f.endswith('.xlsx') and not f.endswith('.xls'):
            continue
        iteratorSheet(args.excel + f, f, args.proto, args.data)
    
    # 生成c++版本的proto源文件
    if args.pb:
        protocFile(args.proto)
    
    #删除无用文件
    pb_files = os.listdir(args.proto)
    for f in pb_files:
        if f.endswith('.py'):
            os.remove(args.proto + f)
        elif f.endswith('__pycache__'):
            shutil.rmtree(args.proto + f)

    #生成管理器代码
    if args.code:
        dirname = os.path.dirname(sys.argv[0])
        cmd = 'python3 '
        cmd += dirname + '/gen_xls_mgr.py '
        cmd += args.proto + " " + args.code
        #print("%s" % (cmd))
        os.system(cmd)


