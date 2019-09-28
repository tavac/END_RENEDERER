DISCLOSURES
You can go into d3d11_renderer_impl.h at the top you can switch on/off features.
*Particles*
  The Particles rendered are the free_pool and sorted_pool tests(not them together).
  If you want to try to run them together enable "RENDER_PARTICLES" define in the header mentioned.
*Turn_To*
  It will randomly bug out and go into a loop of not knowing where to turn,
  but if you move around a bit and go back to the position where it bugged out,
  it's very possible it will work. This is an intermittent bug I haven't been able
  to figure out.
*Mouse_Cam Control*
  Only caveat is the limited screen space, so you'll have to click and drag repeatedly
  or increase mouse sensitivity if you want to rotate all the way.

-- Turn_To / Look_At --
*if it gets buggy you can try turning it off and on again.*
1 - Turns it on
2 - Turns it off

-- Camera Controls --
WASD - local translations
CX   - Up/Down translations
QE   - L/R rotations
RF   - Up/Down rotations
*Optional Hold RMB in render window and use mouse to rotate Camera*

-- Frustum Controls --
IJKL - local translations
NM   - Up/Down translations
UO   - L/R rotations
YH   - Up/Down rotations
