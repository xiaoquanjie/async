#! /usr/bin/env python
# coding=utf-8

import os
import platform
import xlrd
import sys
import shutil

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
    if len(sys.argv) < 4:
        print("Usage: param is too less for translate sheet")
        print('python3 run_me.py excel_path proto_path data_path')
        sys.exit(-1)
    
    excel_path = sys.argv[1]
    proto_path = sys.argv[2]
    data_path = sys.argv[3]
    
    # 生成proto_path目录
    if not os.path.exists(proto_path):
        os.makedirs(proto_path)

    # 迭代excel目录
    files = os.listdir(excel_path)
    for f in files:
        if f.startswith('~.') or f.startswith("~$"):
            continue
        if not f.endswith('.xlsx') and not f.endswith('.xls'):
            continue
        iteratorSheet(excel_path + f, proto_path, data_path)
    
    # 生成c++版本的proto源文件
    protocFile(proto_path)
    
    #删除无用文件
    pb_files = os.listdir(proto_path)
    for f in pb_files:
        if f.endswith('.py'):
            os.remove(proto_path + f)
        elif f.endswith('__pycache__'):
            shutil.rmtree(proto_path + f)

    #生成管理器代码
    cmd = "python3 ./gen_xls_mgr.py " + proto_path + " " + proto_path
    #print("%s" % (cmd))
    os.system(cmd)


