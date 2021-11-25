#! /usr/bin/env python
# coding=utf-8

import sys
import os
import logging


class Color(object):
    BLACK = "\033[30m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    PURPLE = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"
    NONE = "\033[0m"


class LogHelper:
    """日志辅助类"""
    _logger = None
    _close_imme = True

    @staticmethod
    def set_close_flag(flag):
        LogHelper._close_imme = flag

    @staticmethod
    def _init():
        # file log
        formatter = logging.Formatter('%(asctime)s|%(levelname)s|%(lineno)d|%(funcName)s|%(message)s')
        handler = logging.FileHandler('xls_translator.log', mode='w')
        handler.setFormatter(formatter)
        LogHelper._logger = logging.getLogger()
        LogHelper._logger.addHandler(handler)
        LogHelper._logger.setLevel(logging.DEBUG)

        # console log
        console_handler = logging.StreamHandler()
        console_handler.setFormatter(formatter)
        LogHelper._logger.addHandler(console_handler)

    @staticmethod
    def get_logger():
        if LogHelper._logger is None:
            LogHelper._init()

        return LogHelper._logger

    @staticmethod
    def close():
        if LogHelper._close_imme:
            if LogHelper._logger is None:
                return
            logging.shutdown()


# log wrapper
LOG_DEBUG=LogHelper.get_logger().debug
LOG_INFO=LogHelper.get_logger().info
LOG_WARN=LogHelper.get_logger().warn
LOG_ERROR=LogHelper.get_logger().error

