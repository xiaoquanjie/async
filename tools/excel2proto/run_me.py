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

g_python_cmd = 'python3 '

# 遍历sheet
def iteratorSheet(xls_file_path, file_name, proto_path, data_path, r2):
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

        cmd = g_python_cmd
        cmd += dirname + '/xls_translator.py '
        cmd += '--sheet ' + sheet + ' '
        cmd += '--file ' + xls_file_path + ' '
        if proto_path:
            cmd += '--proto ' + proto_path + ' '
        if data_path:
            cmd += '--data ' + data_path + ' '
        if r2:
            cmd += '--r2 ' 

        if os.system(cmd) != 0:
            raise 'fail' 

def protocFile(proto_path, pb_path):
    files = os.listdir(proto_path)
    for f in files:
        if not f.endswith('.proto'):
            continue
        
        command = None
        if platform.system() == 'Windows':
            dirname = os.path.dirname(sys.argv[0])
            command = dirname
            command += "\\protoc.exe "
            command += "--proto_path=" + os.path.join(proto_path, " --cpp_out=")
            name = " " + f
            command += os.path.join(pb_path, name)
        else:
            command = 'protoc '
            command += '--proto_path=' + proto_path + ' '
            command += '--cpp_out=' + pb_path + ' ' 
            command += f
        #print('command:', command)
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
            os.remove(os.path.join(dir_path, f))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--excel', required=True,  help='the excel directory path [./excel/]')
    parser.add_argument('--proto', required=False,  help='the proto directory path [./proto/]')
    parser.add_argument('--data',  required=False,  help='the data directory path  [./data/]')
    parser.add_argument('--code',  required=False,  help='the code directory path   [./code/]')
    parser.add_argument('--pb',  required=False,  help='the code directory path   [./code/]')
    parser.add_argument('--python', required=False, help='python command')
    parser.add_argument('--r2', nargs='?', const=True, required=False, help='readable version2')
    parser.add_argument('--white', nargs='?', const='white_file', required=False, help='the white list')
    parser.add_argument('--black', nargs='?', const='black_file', required=False, help='the black list')

    #parser.print_help()
    args = parser.parse_args()
    if args.python:
        g_python_cmd = args.python + ' '
    #print(args)

    # 清除自动生成的文件
    clearDirectoryFile(args.proto, '.proto')
    clearDirectoryFile(args.pb, '.pb.h')
    clearDirectoryFile(args.pb, '.pb.cc')
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
        iteratorSheet(os.path.join(args.excel, f), f, args.proto, args.data, args.r2)
    
    # 生成c++版本的proto源文件
    if args.pb:
        protocFile(args.proto, args.pb)
    
    #删除无用文件
    pb_files = os.listdir(args.proto)
    for f in pb_files:
        if f.endswith('.py'):
            os.remove(os.path.join(args.proto, f))
        elif f.endswith('__pycache__'):
            shutil.rmtree(os.path.join(args.proto, f))

    #生成管理器代码
    if args.code:
        dirname = os.path.dirname(sys.argv[0])
        cmd = g_python_cmd
        cmd += dirname + '/gen_xls_mgr.py '
        cmd += args.proto + " " + args.code
        #print("%s" % (cmd))
        os.system(cmd)


