"""
Global definitions used by Direct Gui Classes and handy constants
that can be used during widget construction
"""

__all__ = []

from panda3d.core import *

default_font = None
default_font_func = TextNode.getDefaultFont
default_click_sound = None
default_rollover_sound = None
default_dialog_geom = None
default_dialog_relief = PGFrameStyle.TBevelOut
draw_order = 100
panel = None

# USEFUL GUI CONSTANTS

#: Constant used to indicate that an option can only be set by a call
#: to the constructor.
INITOPT = ['initopt']

# Mouse buttons
LMB = 0
MMB = 1
RMB = 2

# Widget state
NORMAL = 'normal'
DISABLED = 'disabled'

# Frame style
FLAT = PGFrameStyle.TFlat
RAISED = PGFrameStyle.TBevelOut
SUNKEN = PGFrameStyle.TBevelIn
GROOVE = PGFrameStyle.TGroove
RIDGE = PGFrameStyle.TRidge
TEXTURE_BORDER = PGFrameStyle.TTextureBorder

FrameStyleDict = {'flat': FLAT, 'raised': RAISED, 'sunken': SUNKEN,
                  'groove': GROOVE, 'ridge': RIDGE,
                  'texture_border': TEXTURE_BORDER,
                  }

# Orientation of DirectSlider and DirectScrollBar
HORIZONTAL = 'horizontal'
VERTICAL = 'vertical'
VERTICAL_INVERTED = 'vertical_inverted'

# Dialog button values
DIALOG_NO = 0
DIALOG_OK = DIALOG_YES = DIALOG_RETRY = 1
DIALOG_CANCEL = -1

# User can bind commands to these gui events
DESTROY = 'destroy-'
PRINT = 'print-'
ENTER = PGButton.getEnterPrefix()
EXIT = PGButton.getExitPrefix()
WITHIN = PGButton.getWithinPrefix()
WITHOUT = PGButton.getWithoutPrefix()
B1CLICK = PGButton.getClickPrefix() + MouseButton.one().getName() + '-'
B2CLICK = PGButton.getClickPrefix() + MouseButton.two().getName() + '-'
B3CLICK = PGButton.getClickPrefix() + MouseButton.three().getName() + '-'
B1PRESS = PGButton.getPressPrefix() + MouseButton.one().getName() + '-'
B2PRESS = PGButton.getPressPrefix() + MouseButton.two().getName() + '-'
B3PRESS = PGButton.getPressPrefix() + MouseButton.three().getName() + '-'
B1RELEASE = PGButton.getReleasePrefix() + MouseButton.one().getName() + '-'
B2RELEASE = PGButton.getReleasePrefix() + MouseButton.two().getName() + '-'
B3RELEASE = PGButton.getReleasePrefix() + MouseButton.three().getName() + '-'
# For DirectEntry widgets
OVERFLOW = PGEntry.getOverflowPrefix()
ACCEPT = PGEntry.getAcceptPrefix() + KeyboardButton.enter().getName() + '-'
ACCEPTFAILED = PGEntry.getAcceptFailedPrefix(
) + KeyboardButton.enter().getName() + '-'
TYPE = PGEntry.getTypePrefix()
ERASE = PGEntry.getErasePrefix()
CURSORMOVE = PGEntry.getCursormovePrefix()
# For DirectSlider and DirectScrollBar widgets
ADJUST = PGSliderBar.getAdjustPrefix()


# For setting the sorting order of a widget's visible components
IMAGE_SORT_INDEX = 10
GEOM_SORT_INDEX = 20
TEXT_SORT_INDEX = 30

FADE_SORT_INDEX = 1000
NO_FADE_SORT_INDEX = 2000

# Handy conventions for organizing top-level gui objects in loose buckets.
BACKGROUND_SORT_INDEX = -100
MIDGROUND_SORT_INDEX = 0
FOREGROUND_SORT_INDEX = 100

# Symbolic constants for the indexes into an optionInfo list.
_OPT_DEFAULT = 0
_OPT_VALUE = 1
_OPT_FUNCTION = 2

# DirectButton States:
BUTTON_READY_STATE = PGButton.SReady       # 0
BUTTON_DEPRESSED_STATE = PGButton.SDepressed   # 1
BUTTON_ROLLOVER_STATE = PGButton.SRollover    # 2
BUTTON_INACTIVE_STATE = PGButton.SInactive    # 3


def get_default_rollover_sound():
    return default_rollover_sound


def set_default_rollover_sound(newSound):
    global default_rollover_sound
    default_rollover_sound = newSound


def getDefaultClickSound():
    return default_click_sound


def set_default_click_sound(newSound):
    global default_click_sound
    default_click_sound = newSound


def get_default_font():
    global default_font
    if default_font is None:
        default_font = default_font_func()
    return default_font


def set_default_font(new_font):
    """Changes the default font for DirectGUI items.  To change the default
    font across the board, see :meth:`.TextNode.set_default_font`. """
    global default_font
    default_font = new_font


def set_default_font_func(new_font_func):
    global default_font_func
    default_font_func = new_font_func


def get_default_dialog_geom():
    global default_dialog_geom
    return default_dialog_geom


def get_default_dialog_relief():
    global default_dialog_relief
    return default_dialog_relief


def set_default_dialog_geom(new_dialog_geom, relief=None):
    global default_dialog_geom, default_dialog_relief
    default_dialog_geom = new_dialog_geom
    default_dialog_relief = relief


def get_default_draw_order():
    return draw_order


def set_default_draw_order(new_draw_order):
    global draw_order
    draw_order = new_draw_order


def get_default_panel():
    return panel


def set_default_panel(new_panel):
    global panel
    panel = new_panel
