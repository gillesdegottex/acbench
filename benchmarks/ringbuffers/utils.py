# Copyright (C) 2024 Gilles Degottex - All Rights Reserved
#
# You may use, distribute and modify this code under the
# terms of the Apache 2.0 license. You should have
# received a copy of this license with this file.
# If not, please visit:
#     https://github.com/gillesdegottex/acbench

import platform
import subprocess
import re

def get_processor_name():
    if platform.system() == "Windows":
        return platform.processor()
    elif platform.system() == "Darwin":
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + '/usr/sbin'
        command ="sysctl -n machdep.cpu.brand_string"
        return subprocess.check_output(command).strip()
    elif platform.system() == "Linux":
        command = "cat /proc/cpuinfo"
        all_info = subprocess.check_output(command, shell=True).strip().decode()
        for line in all_info.split("\n"):
            if "model name" in line:
                return re.sub( ".*model name.*:\s*", "", line,1)
    return ""
