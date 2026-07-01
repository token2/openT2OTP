#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2.
# Renders bom-table.png from ../bom.csv. Suggested/reference BOM, not a locked one.
import csv, matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt

rows=list(csv.reader(open('../bom.csv')))
hdr=rows[0]; data=rows[1:]

# columns to show (trim Notes width): Ref, Qty, Component, Value/Part, Example MPN, Package
cols=[0,1,2,3,4,5]
colnames=['Ref','Qty','Component','Value / Part','Example MPN','Package']
widths=[0.055,0.04,0.20,0.24,0.19,0.10]  # fractional; Notes shown as small sub-row

fig,ax=plt.subplots(figsize=(13, 0.42*len(data)+2.2))
ax.axis('off')
TEAL='#0B7285'; LIGHT='#e6f2f4'; ALT='#f4fafb'

# title
ax.text(0.5,1.02,'openT2OTP — Suggested Bill of Materials',ha='center',va='bottom',
        fontsize=15,weight='bold',color=TEAL,transform=ax.transAxes)
ax.text(0.5,0.995,'Reference setup — example parts, not a locked BOM. Substitute equivalents freely (see sourcing note).',
        ha='center',va='top',fontsize=8.5,style='italic',color='#666',transform=ax.transAxes)

n=len(data); top=0.95; bot=0.02; rh=(top-bot)/(n+1)
# header row
x=0.01
xs=[]
for w in widths:
    xs.append(x); x+=w
xs.append(x)
def cell(xi,yc,w,txt,**kw):
    kw.setdefault('fontsize',7.6)
    ax.text(xi+0.006,yc,txt,ha='left',va='center',transform=ax.transAxes,wrap=True,**kw)
# header band
ax.add_patch(plt.Rectangle((0.01,top-rh),x-0.01,rh,transform=ax.transAxes,fc=TEAL,ec='none'))
for i,name in enumerate(colnames):
    cell(xs[i],top-rh/2,widths[i],name,color='white',weight='bold',fontsize=8)

# rows
for r,row in enumerate(data):
    yc=top-rh*(r+1)-rh/2
    fc=ALT if r%2 else 'white'
    ax.add_patch(plt.Rectangle((0.01,top-rh*(r+2)),x-0.01,rh,transform=ax.transAxes,fc=fc,ec='#dde',lw=0.4))
    vals=[row[c] for c in cols]
    for i,v in enumerate(vals):
        bold = (i==0)
        col = TEAL if i==0 else '#222'
        # truncate long fields
        maxlen=[6,4,30,34,26,12][i]
        vv=v if len(v)<=maxlen else v[:maxlen-1]+'…'
        cell(xs[i],yc,widths[i],vv,color=col,weight='bold' if bold else 'normal')

ax.set_xlim(0,1); ax.set_ylim(0,1)
plt.tight_layout()
plt.savefig('bom-table.png',dpi=130,facecolor='white',bbox_inches='tight')
print("rendered", n, "rows")
