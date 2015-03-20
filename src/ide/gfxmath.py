import wx
import pdb
import math

PI = 3.141592636


def lineRectIntersect(lineIn,lineOut, rectUL, rectLR):
  """Given a line segment defined by 2 points: lineIn and lineOut where lineIn is INSIDE the rectangle
     Given a rectangle defined by 2 points rectUL and rectLR where rectUL < rectLR (ul is upper left) and where the rectangle is aligned with the axes.

     Return the point where the line intersects the rectangle.

     This function can be used to make the pointer arrows start and end at the edges of the entities
  """
  # TODO
  return lineIn 

def boxIntersect(pos1, size1, pos2, size2):
  """Calculate the largest box inside BOTH boxes.  NOTE: returns x,y,x1,y1, NOT x,y,size"""
  ret = (max(pos1[0],pos2[0]),max(pos1[1],pos2[1]),min(pos1[0]+size1[0],pos2[0]+size2[0]),min(pos1[1]+size1[1],pos2[1]+size2[1]))
  # ret is now the box x,y,x1,y1
  # TODO convert to pos, size?
  return ret


def calcArrow(start_x, start_y, end_x, end_y,length=5.0,degrees=PI/10.0):
  angle = math.atan2 (end_y - start_y, end_x - start_x) + PI;
  x1 = end_x + length * math.cos(angle - degrees);
  y1 = end_y + length * math.sin(angle - degrees);
  x2 = end_x + length * math.cos(angle + degrees);
  y2 = end_y + length * math.sin(angle + degrees);
  return((x1,y1),(x2,y2))

def lineVector(a,b):
  ret = (b[0]-a[0], b[1]-a[1])
  length = math.sqrt(ret[0]*ret[0] + ret[1]*ret[1])
  if length == 0:
    return (0,0)
  ret = (ret[0]/length, ret[1]/length)
  return ret

def convertToRealPos(pos, scale):
  if pos == None: return pos
  if isinstance(pos, wx.Point):
    return wx.Point(round(pos.x/scale), round(pos.y/scale))
  elif isinstance(pos, tuple):
    return map(lambda x: convertToRealPos(x, scale), pos)
  else:
    return round(pos/scale)

def rectOverlaps(rect_a, rect_b):
    """Assumes tuples of format x0,y0, x1,y1 where 0 is upper left (lowest), 1 is lower right
       This code finds if the rectangles are separate by checking if the right side of one rectangle is further left then the leftmost side of the other, etc...
    """
    print rect_a, rect_b
    separate = rect_a[2] < rect_b[0] or rect_a[0] > rect_b[2] or rect_a[1] > rect_b[3] or rect_a[3] < rect_b[1]
    return not separate

def partition(numitems, bound):
  size = (bound[2] - bound[0], bound[3] - bound[1])
  topMargin = size[1]/5
  margin = 5
  innermargin = 2

  if numitems==0:
    return
  if numitems==1:
    yield (bound[0]+margin, bound[1]+topMargin,bound[2]-margin, bound[3]-margin)
  elif numitems==2:
    yield (bound[0]+margin, bound[1]+topMargin,bound[0]+(size[0]/2)-innermargin, bound[3]-margin)
    yield (bound[0]+(size[0]/2)+innermargin, bound[1]+topMargin,bound[2]-margin, bound[3]-margin)
  elif numitems==3:
    yield (bound[0]+margin,bound[1]+topMargin,bound[0]+(size[0]/3)-innermargin,bound[3]-margin)
    yield (bound[0]+(size[0]/3)+innermargin,bound[1]+topMargin,bound[0]+(2*size[0]/3)-innermargin,bound[3]-margin)
    yield (bound[0]+(2*size[0]/3)+innermargin,bound[1]+topMargin,bound[2]-margin, bound[3]-margin)
  else:
    yield (bound[0]+margin, bound[1]+topMargin,bound[0]+(size[0]/numitems)-innermargin, bound[3]-margin)
    i = 1
    while i < (numitems-1):
      yield (bound[0]+(i*size[0]/numitems)+innermargin, bound[1]+topMargin,bound[0]+((i+1)*size[0]/numitems)-innermargin, bound[3]-margin)
      i+=1
    yield (bound[0]+((numitems-1)*size[0]/numitems)+innermargin,bound[1]+topMargin,  bound[2]-margin, bound[3]-margin)
    #pdb.set_trace()

def inBox(point, bound):
  """is a point inside a box?  The box bound must be specified (x,y,x1,y1) where x<=x1 y<=y1"""
  if point[0] < bound[0] or point[0] > bound[2] or point[1] < bound[1] or point[1] > bound[3]: return False
  return True
