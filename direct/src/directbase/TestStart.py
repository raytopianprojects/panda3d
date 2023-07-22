from direct.showbase import ShowBase
from direct.showbase.PythonUtil import *
from panda3d.core import *
print('TestStart: Starting up test environment.')


base = ShowBase.ShowBase()

# Put an axis in the world:
loader.loadModel("models/misc/xyzAxis").reparentTo(render)

if 0:
    # Hack:
    # Enable drive mode but turn it off, and reset the camera
    # This is here because ShowBase sets up a drive interface, this
    # can be removed if ShowBase is changed to not set that up.
    base.use_drive()
    base.disable_mouse()
    if base.mouseInterface:
        base.mouseInterface.reparentTo(base.dataUnused)
    if base.mouse2cam:
        base.mouse2cam.reparentTo(base.dataUnused)
    # end of hack.

camera.setPosHpr(0, -10.0, 0, 0, 0, 0)
base.cam_lens.setFov(52.0)
base.cam_lens.setNearFar(1.0, 10000.0)

globalClock.setMaxDt(0.2)
base.enableParticles()

# Force the screen to update:
base.graphics_engine.renderFrame()
