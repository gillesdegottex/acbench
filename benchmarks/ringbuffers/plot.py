# Copyright (C) 2024 Gilles Degottex - All Rights Reserved
#
# You may use, distribute and modify this code under the
# terms of the Apache 2.0 license. You should have
# received a copy of this license with this file.
# If not, please visit:
#     https://github.com/gillesdegottex/acbench

import glob
import numpy as np
import math
import statistics

from utils import get_processor_name

import matplotlib.pyplot as plt
plt.ion()

def getlinestyle(method):
    color = None
    marker = None
    if method=='FastestBound':
        color = 'black'
        marker = None
    if method=='STD':
        color = 'grey'
    if method=='Boost':
        color = 'yellow'
    if method=='Portaudio':
        color = 'blue'
        marker = 'o'
    if method=='RubberBand':
        color = 'red'
        marker = 'x'
    if method=='Jack':
        color = 'magenta'
        marker = 'v'
    if method=='ACBench':
        color = 'green'
        marker = '^'

    return color, marker

plt.figure(figsize=(6,12))

for scenarion, scenario in enumerate(['push_back_array', 'push_pull_array']):
    plt.subplot(2,1,1+scenarion)

    for method in ['FastestBound', 'STL', 'Boost', 'Portaudio', 'RubberBand', 'Jack', 'ACBench']:
        chunk_sizes = np.sort([int(el[len(f"STL_{scenario}_"):-12]) for el in glob.glob(f'STL_{scenario}_*')])
        elapseds = {}
        centiles = [5, 50, 95]
        for centile in centiles:
            elapseds[f'cent{centile}'] = []
        for chunk_size in chunk_sizes:
            file_path = f'{method}_{scenario}_{chunk_size}_elapsed.bin'
            elapsed = np.fromfile(file_path, dtype=np.float32)
            elapsed *= 1e9  # [s] to [ns]
            elapsed /= chunk_size  # [ns] to [ns/sample]
            elapsed = np.sort(elapsed)
            # elapsed = np.array([int(el) for el in elapsed])
            for centile in centiles:
                elapseds[f'cent{centile}'].append(np.quantile(elapsed,centile/100.0))
            # hist, bin_edges = np.histogram(elapsed*1e9, bins=1000, range=[0.0, 1000.0], density=True)
            # plt.plot((bin_edges[:-1]+bin_edges[1:])/2, hist, label=method)

        color, marker = getlinestyle(method)

        if 0:
            plt.fill_between(chunk_sizes, elapseds[f'cent{centiles[0]}'], elapseds[f'cent{centiles[-1]}'], facecolor=color, alpha=0.5)
            plt.plot(chunk_sizes, elapseds['cent50'], label=method, color=color, marker=marker)
        else:
            plt.fill_between(chunk_sizes, np.log10(elapseds[f'cent{centiles[0]}']), np.log10(elapseds[f'cent{centiles[-1]}']), facecolor=color, alpha=0.5)
            plt.plot(chunk_sizes, np.log10(elapseds['cent50']), label=method, color=color, marker=marker)

    plt.legend(loc='upper right')
    plt.grid()
    if scenario=='push_back_array':
        plt.ylim([-2.0, 2.0])
    elif scenario=='push_pull_array':
        plt.ylim([-2.0, 2.0])
    plt.xlabel('Chunk size [samples]')
    plt.ylabel('Processing time [log10 ns/sample]')
    # plt.ylabel('Speed [GFLOPS]')
    plt.title(f'{scenario}')
    plt.gcf().suptitle(f'{get_processor_name()}')

plt.savefig('results.png')

from IPython.core.debugger import  Pdb; Pdb().set_trace()
