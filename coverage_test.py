# Copyright (C) 2024 Gilles Degottex - All Rights Reserved
#
# You may use, distribute and modify this code under the
# terms of the Apache 2.0 license. You should have
# received a copy of this license with this file.
# If not, please visit:
#     https://github.com/gillesdegottex/acbench

import logging

import json

with open('ringbuffer_test_coverage.summary.json') as f:
    data = json.load(f)

print('')

error = False
print(f"Total: Branches: {data['branch_covered']}/{data['branch_total']}, {data['branch_percent']:.1f}%")
if data['branch_percent'] < 95.0:
    error = True
    logging.error(f"Branch coverage is below 95%.")

print(f"Total: Lines: {data['line_covered']}/{data['line_total']}, {data['line_percent']:.1f}%")
if data['line_covered'] < data['line_total']:
    error = True
    logging.error(f"Not all lines have been executed.")

if error:
    exit(1)
