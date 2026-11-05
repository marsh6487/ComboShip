"""Compile the exact EnRl_Update body against instrumented native-call stubs.

This is a focused C behavior test, not a complete ComboShip build/runtime test.
"""
import json,re,subprocess,tempfile
from pathlib import Path

def body(source):
 text=Path(source).read_text();start=text.index('void EnRl_Update(Actor* thisx, PlayState* play) {')
 level=0
 for i in range(text.index('{',start),len(text)):
  if text[i]=='{':level+=1
  elif text[i]=='}':
   level-=1
   if not level:return text[start:i+1]
 raise AssertionError('Missing function body')

def run(source):
 pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct { float x,y,z; } Vec3f;
typedef struct { Vec3f pos; } Home;
typedef struct { int params; Home home; } Actor;
typedef struct { Actor actor; int action,drawConfig,skelAnime; } EnRl;
typedef struct { int sceneNum,age,cutsceneActive,yaw,shapeYaw; } PlayState;
#define SCENE_TEMPLE_OF_TIME 67
#define SCENE_CHAMBER_OF_THE_SAGES 68
#define VT_FGCOL(x) ""
#define VT_RST ""
#define RED 1
#define osSyncPrintf(...) ((void)0)
static int nativeCalls,bgCalls,animCalls,blinkCalls;
void func_80AE744C(EnRl* self,PlayState* play){ bgCalls++; }
void EnRl_UpdateSkelAnime(EnRl* self){ animCalls++; }
void func_80AE72D0(EnRl* self){ blinkCalls++; }
static void nativeAction(EnRl* self,PlayState* play){
 nativeCalls++;
 if(self->action==0){self->action=1;self->drawConfig=1;play->yaw=5461;play->shapeYaw=5461;}
}
static void (*sActionFuncs[8])(EnRl*,PlayState*)={nativeAction,nativeAction,nativeAction,nativeAction,nativeAction,nativeAction,nativeAction,nativeAction};
'''
 post=r'''
int main(void){
 int errors=0,total=0;
 for(int age=0;age<2;age++) for(int active=0;active<2;active++){
  EnRl sage={0};sage.actor.home.pos.z=-1776.0f;
  PlayState p={SCENE_TEMPLE_OF_TIME,age,active,-32768,-32768};
  nativeCalls=bgCalls=animCalls=blinkCalls=0;
  EnRl_Update(&sage.actor,&p);EnRl_Update(&sage.actor,&p);total++;
  if(p.yaw!=-32768 || p.shapeYaw!=-32768 || nativeCalls || bgCalls!=2 || animCalls!=2 || blinkCalls!=2 || sage.drawConfig!=1){
   fprintf(stderr,"FAIL annex preserves pedestal yaw / idle age=%d cutscene=%d\n",age,active);errors++;
  }
 }
 // Native story Rauru, non-annex Temple placements and nonzero params retain
 // the existing dispatch and its original first-update player-facing behavior.
 for(int which=0;which<4;which++){
  EnRl sage={0};sage.actor.home.pos.z=which==1?0.0f:-1776.0f;
  sage.actor.params=which==2?2:0;
  PlayState p={which==0?SCENE_CHAMBER_OF_THE_SAGES:(which==3?1:SCENE_TEMPLE_OF_TIME),0,0,-32768,-32768};
  nativeCalls=bgCalls=animCalls=blinkCalls=0;
  EnRl_Update(&sage.actor,&p);total++;
  if(nativeCalls!=1 || bgCalls || animCalls || blinkCalls || p.yaw!=5461){errors++;fprintf(stderr,"FAIL native scope %d\n",which);}
 }
 printf("%d cases, %d failures\n",total,errors);return errors?1:0;
}
'''
 with tempfile.TemporaryDirectory() as d:
  src=Path(d)/'test.c';exe=Path(d)/'test';src.write_text(pre+body(source)+post)
  c=subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wno-unused-parameter','-Werror',str(src),'-o',str(exe)],capture_output=True,text=True)
  assert c.returncode==0,c.stderr
  p=subprocess.run([str(exe)],capture_output=True,text=True)
  return {'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr}

if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]/'soh/src/overlays/actors/ovl_En_Rl/z_en_rl.c')
 a=p.parse_args()
 result=run(a.source)
 print(result['stdout'],end='')
 print(result['stderr'],end='')
 raise SystemExit(result['returncode'])
