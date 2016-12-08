#!/usr/bin/env python
#
# calculate efficiency from pt-match tree output

from __future__ import print_function, unicode_literals
import sys

import ROOT

f = ROOT.TFile(sys.argv[1])
t = f.Get(b'h35_05/tracks')

def cut(axis):
    expr, x0, x1, nbins = axis
    return '({x0}<({expr}))&&(({expr})<{x1})'.format(expr=expr, x0=x0, x1=x1)

def make_h1(name, tree, axis_x, selection=''):
    expr_x, x0, x1, nx = axis_x
    expr = '{}>>{}({:d}, {:d}, {:d})'.format(expr_x, name, nx, x0, x1)
    tree.Draw(expr.encode(), selection.encode(), b'goff')
    return ROOT.gDirectory.Get(name.encode())
def make_h2(name, tree, axis_x, axis_y, selection=''):
    expr_x, x0, x1, nx = axis_x
    expr_y, y0, y1, ny = axis_y
    expr = '{}:{}>>{}({:d}, {:d}, {:d}, {:d}, {:d}, {:d})'.format(
        expr_y, expr_x, name, nx, x0, x1, ny, y0, y1)
    tree.Draw(expr.encode(), selection.encode(), b'goff')
    return ROOT.gDirectory.Get(name.encode())
def make_h(name, tree, args, selection=''):
    if len(args) == 1:
        return make_h1(name, tree, *args, selection=selection)
    if len(args) == 2:
        return make_h2(name, tree, *args, selection=selection)
    raise Exception('Invalid number or arguments')

def make_eff(name, tree, args, selection_passed, selection_total=''):
    if selection_total:
        selection_passed = selection_total + '&&' + selection_passed
    total = make_h(name + '-total', tree, args, selection_total)
    passed = make_h(name + '-passed', tree, args, selection_passed)
    eff = ROOT.TEfficiency(passed, total)
    return eff

# axis definitions
trk_col = ('trk_col', 40, 65, 25)
trk_row = ('trk_row', 0, 301, 301)
cut_good = 'clu_size>0 && d2<10'

eff = make_eff('eff_cr', t, [trk_col, trk_row], cut_good)
# eff = make_eff('eff_c', t, [trk_col], cut_good, cut(trk_row))
# eff = make_eff('eff_r', t, [trk_row], cut_good, cut(trk_col))
tot = eff.GetTotalHistogram()
pas = eff.GetPassedHistogram()

labelx = b'Column pixel'
labely = b'Row pixel'
eff.SetNameTitle(b'', b'')
eff.SetTitle('Efficiency')
tot.SetTitle('All tracks')
tot.SetXTitle(labelx)
tot.SetYTitle(labely)
pas.SetTitle('Matched tracks')
pas.SetXTitle(labelx)
pas.SetYTitle(labely)

ROOT.gStyle.SetOptStat(0)
c = ROOT.TCanvas(b'Efficieny', '', 800, 400)
c.Divide(3, 1)
c.cd(1)
tot.Draw('colz')
c.cd(2)
pas.Draw('colz')
c.cd(3)
eff.Draw('colz')

# c.SaveAs('eff.pdf')
