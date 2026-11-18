"""Accepted POC4 continuous translucent flame field, independent of preview tools."""
import numpy as np


def flame_texture(u,v,frame,eye):
    """Return RGB and alpha; u=-1..1, v=0 bottom..1 top, eye=-1 or +1.

    The accepted animation loops over 180 frames; runtime assets sample every
    third frame. Maximum opacity is 65%. No color/shape/motion revisions belong
    in a packaging-only change to this module.
    """
    t=frame*2*np.pi/180
    warp=u+.075*np.sin(v*10-5*t+eye)+.045*np.sin(v*19-7*t-eye)
    shoulder=np.sqrt(np.clip(1-(u/.96)**2,0,1))
    crown=(.72+.13*np.sin(8*warp-4*t+eye)+.09*np.sin(16*warp+6*t))
    top=.20+shoulder*crown
    bottom=.08+.18*(1-shoulder)
    side=np.clip((.98-np.abs(u))/.15,0,1)
    alpha=side*np.clip((v-bottom)/.07,0,1)*np.clip((top-v)/.045,0,1)
    body=np.exp(-((u/.77)**4+((v-.34)/.26)**4))
    alpha=np.maximum(alpha,body)*.65
    turbulent=.5+.5*np.sin(9*warp+12*v-6*t+1.6*np.sin(6*v-3*t))
    heat=np.clip(np.exp(-((warp/.67)**2+((v-.34)/.46)**2))*(.9+.15*turbulent),0,1)
    rgb=np.stack([12+220*heat**2,100+155*heat,250+5*heat],1)
    return rgb,alpha[:,None]
