#!/usr/bin/env python3
#
# provide tracking performance comparison plots

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import ROOT
import root_numpy

dataset_dir = Path('unigetel_dummy/ebeam120_pionp_nparticles01_inc')
title = '5GeV e⁺ (Sim)'
sensor_names = ['tel{}'.format(_) for _ in range(6)]
sensor_labels = ['Plane {}'.format(_) for _ in range(6)]
length_scale = 1000
length_unit = 'µm'

# dataset_dir = Path('MALTAscope/3GeV_electrons')
# title = '3GeV e$^-$ (Sim)'
# sensor_names = ['MALTAplane{}'.format(_) for _ in range(4)]
# sensor_labels = ['Plane {}'.format(_) for _ in range(4)]
# length_scale = 1000
# length_unit = 'µm'

plot_dir = 'plots' / dataset_dir
paths = [
    'test/output-straight3d' / dataset_dir / 'recon-hists.root',
    'test/output-gbl3d' / dataset_dir / 'recon-hists.root',
]
labels = [
    'Straight',
    'GBL',
]

def main():
    plot_dir.mkdir(parents=True, exist_ok=True)

    files = [ROOT.TFile.Open(str(_), 'READ') for _ in paths]
    plot_reduced_chi2(files)
    plot_probability(files)
    for s, l in zip(sensor_names, sensor_labels):
        plot_residuals(files, s, l)

def plot_h1(ax, h1, label=None, normalize=True, scale=1):
    n, edges = root_numpy.hist2array(h1, return_edges=True)
    edges = scale * edges[0]
    if normalize:
        normalization = np.sum(n * np.diff(edges))
        n /= normalization
    # to ensure n, edges have the same shape. last point not used in step
    n.resize(n.size + 1)
    ax.step(edges, n, where='post', label=label)

def plot_reduced_chi2(files):
    hs = [_.Get('tracks/reduced_chi2') for _ in files]
    fg, ax = plt.subplots()
    for h, l in zip(hs, labels):
        plot_h1(ax, h, l)
    ax.autoscale(axis='x', tight=True)
    ax.set_ylim(bottom=0)
    ax.set_xlabel('track $\\chi²$ / d.o.f.')
    ax.set_yticklabels([])
    ax.legend(loc='upper right')
    fg.savefig(plot_dir / 'track_reduced_chi2.pdf')

def plot_probability(files):
    hs = [_.Get('tracks/probability') for _ in files]
    fg, ax = plt.subplots()
    for h, l in zip(hs, labels):
        plot_h1(ax, h, l)
    ax.set_xlim(left=0, right=1)
    # ax.set_ylim(bottom=0)
    ax.set_yscale('log')
    ax.set_xlabel('track probability')
    ax.set_yticklabels([])
    ax.legend(loc='upper right', title=title)
    fg.savefig(plot_dir / 'track_probability.pdf')

def plot_residuals(files, sensor_name, sensor_label):
    hus = [_.Get('sensors/{}/residuals/res_u'.format(sensor_name)) for _ in files]
    hvs = [_.Get('sensors/{}/residuals/res_v'.format(sensor_name)) for _ in files]
    fg, (axu, axv) = plt.subplots(ncols=2)
    for hu, hv, l in zip(hus, hvs, labels):
        plot_h1(axu, hu, label=l, scale=length_scale)
        plot_h1(axv, hv, label=l, scale=length_scale)
    for ax in [axu, axv]:
        ax.autoscale(axis='both', tight=True)
        # scale w/ extra space for legend
        bottom, top = ax.get_ylim()
        ax.set_ylim(bottom=0, top=1.5*top)
        ax.set_yticklabels([])
    axu.set_xlabel('u residual / {}'.format(length_unit))
    axv.set_xlabel('v residual / {}'.format(length_unit))
    axu.legend(loc='upper left', title=title+'\n'+sensor_label)
    fg.savefig(plot_dir / 'track_residuals_{}.pdf'.format(sensor_name))

if __name__ == '__main__':
    main()
