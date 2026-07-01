#!/usr/bin/env python3
# SPDX-License-Identifier: CERN-OHL-P-2.0
# Copyright (c) 2020-2026 Token2 Sarl
#
# Part of openT2OTP hardware reference. Licensed under CERN-OHL-P v2.
# Renders the SUGGESTED component placement diagram (placement.png) from the
# .kicad_pcb. This is a placement illustration, NOT a manufactured PCB.
import re, matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyBboxPatch, Circle
import matplotlib.patheffects as pe

txt=open('openT2OTP.kicad_pcb').read()
comps=[]
for fm in re.finditer(r'\(footprint "([^"]+)".*?\(at ([\d.]+) ([\d.]+)(?: (\d+))?\)(.*?)\n  \)', txt, re.S):
    name,fx,fy,rot=fm.group(1),float(fm.group(2)),float(fm.group(3)),float(fm.group(4) or 0)
    block=fm.group(5); ref=re.search(r'"Reference" "([^"]+)"',block)
    if not ref or 'MountingHole' in name: continue
    side='B' if ('B.Cu' in name or '(back side)' in block) else 'F'
    kind=name.split(':')[-1]
    comps.append((ref.group(1), fx-150, fy-100, rot, kind, side))

KIND={"lcd":(26,15),"qfn48":(7,7),"qfn32":(6,6),"son8":(5,4),"cr2032":(20,20),
      "bat_clip":(18,16),"xtal":(3.2,1.5),"xtal_can":(8,3.5),"c0805":(2,1.25),
      "c0603":(1.6,0.8),"sw":(4,3),"sw_th":(6,6),"swd":(5,6)}
HW,HH,r=24,12,3
MOUNT=[(-19.2,7.3),(19.2,7.3),(19.2,-7.3),(-19.2,-7.3)]

def board_outline(ax):
    # rounded rect
    ax.add_patch(FancyBboxPatch((-HW+r,-HH+r),2*(HW-r),2*(HH-r),
        boxstyle=f"round,pad={r}",fc="#0a5c2a",ec="#083",lw=2,zorder=0))
    # notches
    for nx in (-8,8):
        for ey,d in ((HH,-1),(-HH,1)):
            ax.add_patch(Rectangle((nx-1,ey if d<0 else ey),2,abs(d),fc='white',ec='none',zorder=1))
    # mounting holes
    for mx,my in MOUNT:
        ax.add_patch(Circle((mx,my),1.6,fc='#ccc',ec='#888',lw=1,zorder=3))
        ax.add_patch(Circle((mx,my),1.1,fc='white',ec='#888',lw=0.5,zorder=4))

def draw_qfn(ax,x,y,w,h,label,col):
    ax.add_patch(FancyBboxPatch((x-w/2,y-h/2),w,h,boxstyle="round,pad=0.15",
        fc=col,ec='#111',lw=1,zorder=5))
    # pins on 4 sides
    n=max(3,int(w//1.2)); step=w/(n+1)
    for i in range(n):
        px=x-w/2+step*(i+1)
        for py in (y-h/2, y+h/2-0.5):
            ax.add_patch(Rectangle((px-0.15,py),0.3,0.5,fc='#ccc',ec='none',zorder=6))
    m=max(3,int(h//1.2)); step=h/(m+1)
    for i in range(m):
        py=y-h/2+step*(i+1)
        for px in (x-w/2, x+w/2-0.5):
            ax.add_patch(Rectangle((px,py-0.15),0.5,0.3,fc='#ccc',ec='none',zorder=6))
    # pin-1 dot
    ax.add_patch(Circle((x-w/2+1,y+h/2-1),0.4,fc='#fff',ec='#111',lw=0.5,zorder=7))
    ax.text(x,y,label,ha='center',va='center',fontsize=6.5,weight='bold',color='white',zorder=8)

def draw_soic(ax,x,y,w,h,label,col):
    ax.add_patch(FancyBboxPatch((x-w/2,y-h/2),w,h,boxstyle="round,pad=0.1",fc=col,ec='#111',lw=1,zorder=5))
    n=int(w//1.2)
    for i in range(n):
        px=x-w/2+w/(n+1)*(i+1)
        for py in (y-h/2-0.4,y+h/2-0.1):
            ax.add_patch(Rectangle((px-0.2,py),0.4,0.5,fc='#ccc',ec='none',zorder=6))
    ax.add_patch(Circle((x-w/2+0.8,y+h/2-0.8),0.3,fc='#fff',ec='#111',lw=0.4,zorder=7))
    ax.text(x,y,label,ha='center',va='center',fontsize=6,weight='bold',color='white',zorder=8)

def draw_passive(ax,x,y,w,h,rot,label,col):
    if rot==90: w,h=h,w
    ax.add_patch(Rectangle((x-w/2,y-h/2),w,h,fc=col,ec='#111',lw=0.6,zorder=5))
    # end caps
    ax.add_patch(Rectangle((x-w/2,y-h/2),0.4,h,fc='#aaa',ec='none',zorder=6))
    ax.add_patch(Rectangle((x+w/2-0.4,y-h/2),0.4,h,fc='#aaa',ec='none',zorder=6))
    ax.text(x,y+h/2+0.6,label,ha='center',va='bottom',fontsize=5.5,color='#003',zorder=8)

def draw_lcd(ax,x,y,w,h,label):
    ax.add_patch(FancyBboxPatch((x-w/2,y-h/2),w,h,boxstyle="round,pad=0.2",fc="#c9d4b0",ec='#556',lw=1.5,zorder=5))
    # display active area
    ax.add_patch(Rectangle((x-w/2+2,y-h/2+2),w-4,h-5,fc='#aeb89a',ec='#889',lw=0.8,zorder=6))
    # segment digits hint
    for i in range(6):
        dx=x-w/2+3.5+i*(w-7)/6
        ax.text(dx,y-0.5,'8',ha='center',va='center',fontsize=9,color='#7a8468',family='monospace',zorder=7)
    ax.text(x,y+h/2-1.5,label,ha='center',va='center',fontsize=6,color='#334',zorder=8)

def draw_button(ax,x,y,w,h,label):
    ax.add_patch(Circle((x,y),w/2,fc='#888',ec='#333',lw=1.2,zorder=5))
    ax.add_patch(Circle((x,y),w/2-1,fc='#aaa',ec='#555',lw=0.6,zorder=6))
    ax.text(x,y-w/2-1,label,ha='center',va='top',fontsize=6,weight='bold',color='#003',zorder=8)

def draw_battery(ax,x,y,w,h,label):
    ax.add_patch(Circle((x+2,y),9.5,fc='#f0d840',ec='#a90',lw=2,zorder=5))
    ax.add_patch(Circle((x+2,y),8,fc='#2a2a2a',ec='#000',lw=0.5,zorder=6))
    ax.text(x+2,y,'+',ha='center',va='center',fontsize=16,color='#ccc',zorder=7)
    ax.text(x+2,y-11,label,ha='center',va='top',fontsize=6,weight='bold',color='#003',zorder=8)

def draw_xtal(ax,x,y,w,h,label):
    ax.add_patch(FancyBboxPatch((x-w/2,y-h/2),w,h,boxstyle="round,pad=0.4",fc='#c0c4c8',ec='#555',lw=1,zorder=5))
    ax.text(x,y,label,ha='center',va='center',fontsize=5.5,color='#113',zorder=8)

def draw_pads(ax,x,y,w,h,label):
    ax.add_patch(Rectangle((x-w/2,y-h/2),w,h,fc='none',ec='#dd0',lw=1,ls='--',zorder=5))
    n=4
    for i in range(n):
        ax.add_patch(Circle((x,y-h/2+0.8+i*(h-1.6)/(n-1)),0.6,fc='#dd0',ec='#880',zorder=6))
    ax.text(x+w/2+0.5,y,label,ha='left',va='center',fontsize=5.5,color='#003',zorder=8)

COL={'qfn48':'#3a6ea5','qfn32':'#8a4a9a','son8':'#c05a2a'}

fig,axes=plt.subplots(1,2,figsize=(16,5.2),facecolor='white')
for ax,side,title in zip(axes,['F','B'],
        ['FRONT — display side (what the user sees)','BACK — component side']):
    board_outline(ax)
    for ref,x,y,rot,kind,cside in comps:
        if cside!=side:
            # show battery ghost on front (it overhangs / visible)
            if side=='F' and ref=='BT1':
                ax.add_patch(Circle((x+2,y),9.5,fc='none',ec='#a90',lw=1,ls=':',zorder=2))
                ax.text(x+2,y,'(battery\nbehind)',ha='center',va='center',fontsize=5,color='#684',zorder=2)
            continue
        w,h=KIND[kind]
        if kind=='lcd': draw_lcd(ax,x,y,w,h,ref)
        elif kind=='sw_th': draw_button(ax,x,y,w,h,ref)
        elif kind=='bat_clip': draw_battery(ax,x,y,w,h,ref)
        elif kind in ('qfn48','qfn32'): draw_qfn(ax,x,y,w,h,ref,COL[kind])
        elif kind=='son8': draw_soic(ax,x,y,w,h,ref,COL[kind])
        elif kind=='xtal_can': draw_xtal(ax,x,y,w,h,ref)
        elif kind in ('c0805','c0603'): draw_passive(ax,x,y,w,h,rot,ref,'#d8b078')
        elif kind=='swd': draw_pads(ax,x,y,w,h,ref)
    ax.set_xlim(-28,28); ax.set_ylim(-15,15); ax.set_aspect('equal'); ax.invert_yaxis()
    ax.axis('off'); ax.set_title(title,fontsize=11,weight='bold',pad=8)

fig.suptitle('openT2OTP — SUGGESTED component placement (not a manufactured PCB · no routing)',
             fontsize=12,weight='bold',y=0.99)
fig.text(0.5,0.02,'48 × 24 mm · fits OT1 enclosure · M2 mounting holes at corners · placement matched to the reference board · copper routing not included',
         ha='center',fontsize=8,color='#555')
plt.tight_layout(rect=[0,0.03,1,0.96])
plt.savefig('/tmp/placement_nice.png',dpi=120,facecolor='white')
print("rendered")
