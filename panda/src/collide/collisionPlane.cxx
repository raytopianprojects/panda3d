// Filename: collisionPlane.cxx
// Created by:  drose (25Apr00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////


#include "collisionPlane.h"
#include "collisionHandler.h"
#include "collisionEntry.h"
#include "collisionSphere.h"
#include "collisionLine.h"
#include "collisionRay.h"
#include "collisionSegment.h"
#include "collisionParabola.h"
#include "config_collide.h"
#include "pointerToArray.h"
#include "geomNode.h"
#include "geom.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "boundingPlane.h"
#include "geom.h"
#include "geomTrifans.h"
#include "geomLinestrips.h"
#include "geomVertexWriter.h"

PStatCollector CollisionPlane::_volume_pcollector("Collision Volumes:CollisionPlane");
PStatCollector CollisionPlane::_test_pcollector("Collision Tests:CollisionPlane");
TypeHandle CollisionPlane::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CollisionSolid *CollisionPlane::
make_copy() {
  return new CollisionPlane(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::xform
//       Access: Public, Virtual
//  Description: Transforms the solid by the indicated matrix.
////////////////////////////////////////////////////////////////////
void CollisionPlane::
xform(const LMatrix4 &mat) {
  _plane = _plane * mat;
  CollisionSolid::xform(mat);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::get_collision_origin
//       Access: Public, Virtual
//  Description: Returns the point in space deemed to be the "origin"
//               of the solid for collision purposes.  The closest
//               intersection point to this origin point is considered
//               to be the most significant.
////////////////////////////////////////////////////////////////////
LPoint3 CollisionPlane::
get_collision_origin() const {
  // No real sensible origin exists for a plane.  We return 0, 0, 0,
  // without even bothering to ensure that that point exists on the
  // plane.
  return LPoint3::origin();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::get_volume_pcollector
//       Access: Public, Virtual
//  Description: Returns a PStatCollector that is used to count the
//               number of bounding volume tests made against a solid
//               of this type in a given frame.
////////////////////////////////////////////////////////////////////
PStatCollector &CollisionPlane::
get_volume_pcollector() {
  return _volume_pcollector;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::get_test_pcollector
//       Access: Public, Virtual
//  Description: Returns a PStatCollector that is used to count the
//               number of intersection tests made against a solid
//               of this type in a given frame.
////////////////////////////////////////////////////////////////////
PStatCollector &CollisionPlane::
get_test_pcollector() {
  return _test_pcollector;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::output
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
void CollisionPlane::
output(ostream &out) const {
  out << "cplane, (" << _plane << ")";
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::compute_internal_bounds
//       Access: Protected, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(BoundingVolume) CollisionPlane::
compute_internal_bounds() const {
  return new BoundingPlane(_plane);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_sphere
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_sphere(const CollisionEntry &entry) const {
  const CollisionSphere *sphere;
  DCAST_INTO_R(sphere, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  LPoint3 from_center = sphere->get_center() * wrt_mat;
  LVector3 from_radius_v =
    LVector3(sphere->get_radius(), 0.0f, 0.0f) * wrt_mat;
  PN_stdfloat from_radius = length(from_radius_v);

  PN_stdfloat dist = dist_to_plane(from_center);
  if (dist > from_radius) {
    // No intersection.
    return NULL;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LVector3 normal = (
    has_effective_normal() && sphere->get_respect_effective_normal())
    ? get_effective_normal() : get_normal();

  new_entry->set_surface_normal(normal);
  new_entry->set_surface_point(from_center - get_normal() * dist);
  new_entry->set_interior_point(from_center - get_normal() * from_radius);

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_line
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_line(const CollisionEntry &entry) const {
  const CollisionLine *line;
  DCAST_INTO_R(line, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  LPoint3 from_origin = line->get_origin() * wrt_mat;
  LVector3 from_direction = line->get_direction() * wrt_mat;

  PN_stdfloat t;
  if (!_plane.intersects_line(t, from_origin, from_direction)) {
    // No intersection.  The line is parallel to the plane.

    if (_plane.dist_to_plane(from_origin) > 0.0f) {
      // The line is entirely in front of the plane.
      return NULL;
    }

    // The line is entirely behind the plane.
    t = 0.0f;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LPoint3 into_intersection_point = from_origin + t * from_direction;

  LVector3 normal = 
    (has_effective_normal() && line->get_respect_effective_normal())
    ? get_effective_normal() : get_normal();

  new_entry->set_surface_normal(normal);
  new_entry->set_surface_point(into_intersection_point);

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_ray
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_ray(const CollisionEntry &entry) const {
  const CollisionRay *ray;
  DCAST_INTO_R(ray, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  LPoint3 from_origin = ray->get_origin() * wrt_mat;
  LVector3 from_direction = ray->get_direction() * wrt_mat;

  PN_stdfloat t;

  if (_plane.dist_to_plane(from_origin) < 0.0f) {
    // The origin of the ray is behind the plane, so we don't need to
    // test further.
    t = 0.0f;

  } else {
    if (!_plane.intersects_line(t, from_origin, from_direction)) {
      // No intersection.  The ray is parallel to the plane.
      return NULL;
    }

    if (t < 0.0f) {
      // The intersection point is before the start of the ray, and so
      // the ray is entirely in front of the plane.
      return NULL;
    }
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LPoint3 into_intersection_point = from_origin + t * from_direction;

  LVector3 normal =
    (has_effective_normal() && ray->get_respect_effective_normal()) 
    ? get_effective_normal() : get_normal();

  new_entry->set_surface_normal(normal);
  new_entry->set_surface_point(into_intersection_point);

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_segment
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_segment(const CollisionEntry &entry) const {
  const CollisionSegment *segment;
  DCAST_INTO_R(segment, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  LPoint3 from_a = segment->get_point_a() * wrt_mat;
  LPoint3 from_b = segment->get_point_b() * wrt_mat;

  PN_stdfloat dist_a = _plane.dist_to_plane(from_a);
  PN_stdfloat dist_b = _plane.dist_to_plane(from_b);

  if (dist_a >= 0.0f && dist_b >= 0.0f) {
    // Entirely in front of the plane means no intersection.
    return NULL;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LVector3 normal = (has_effective_normal() && segment->get_respect_effective_normal()) ? get_effective_normal() : get_normal();
  new_entry->set_surface_normal(normal);

  PN_stdfloat t;
  LVector3 from_direction = from_b - from_a;
  if (_plane.intersects_line(t, from_a, from_direction)) {
    // It intersects the plane.
    if (t >= 0.0f && t <= 1.0f) {
      // Within the segment!  Yay, that means we have a surface point.
      new_entry->set_surface_point(from_a + t * from_direction);
    }
  }

  if (dist_a < dist_b) {
    // Point A penetrates deeper.
    new_entry->set_interior_point(from_a);

  } else if (dist_b < dist_a) {
    // No, point B does.
    new_entry->set_interior_point(from_b);

  } else {
    // Let's be fair and choose the center of the segment.
    new_entry->set_interior_point((from_a + from_b) * 0.5);
  }

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_parabola
//       Access: Public, Virtual
//  Description: This is part of the double-dispatch implementation of
//               test_intersection().  It is called when the "from"
//               object is a parabola.
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_parabola(const CollisionEntry &entry) const {
  const CollisionParabola *parabola;
  DCAST_INTO_R(parabola, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  // Convert the parabola into local coordinate space.
  LParabola local_p(parabola->get_parabola());
  local_p.xform(wrt_mat);

  PN_stdfloat t;
  if (_plane.dist_to_plane(local_p.calc_point(parabola->get_t1())) < 0.0f) {
    // The first point in the parabola is behind the plane, so we
    // don't need to test further.
    t = parabola->get_t1();

  } else {
    PN_stdfloat t1, t2;
    if (!get_plane().intersects_parabola(t1, t2, local_p)) {
      // No intersection.  The infinite parabola is entirely in front
      // of the plane.
      return NULL;
    }

    if (t1 >= parabola->get_t1() && t1 <= parabola->get_t2()) {
      if (t2 >= parabola->get_t1() && t2 <= parabola->get_t2()) {
        // Both intersection points are within our segment of the
        // parabola.  Choose the first of the two.
        t = min(t1, t2);
      } else {
        // Only t1 is within our segment.
        t = t1;
      }
      
    } else if (t2 >= parabola->get_t1() && t2 <= parabola->get_t2()) {
      // Only t2 is within our segment.
      t = t2;
      
    } else {
      // Neither intersection point is within our segment.
      return NULL;
    }
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LPoint3 into_intersection_point = local_p.calc_point(t);

  LVector3 normal = (has_effective_normal() && parabola->get_respect_effective_normal()) ? get_effective_normal() : get_normal();

  new_entry->set_surface_normal(normal);
  new_entry->set_surface_point(into_intersection_point);

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::test_intersection_from_box
//       Access: Public, Virtual
//  Description: This is part of the double-dispatch implementation of
//               test_intersection().  It is called when the "from"
//               object is a box.
////////////////////////////////////////////////////////////////////
PT(CollisionEntry) CollisionPlane::
test_intersection_from_box(const CollisionEntry &entry) const {
  const CollisionBox *box;
  DCAST_INTO_R(box, entry.get_from(), 0);

  const LMatrix4 &wrt_mat = entry.get_wrt_mat();

  LPoint3 from_center = box->get_center() * wrt_mat;
  LVector3 from_extents = box->get_dimensions() * wrt_mat * 0.5;
  PN_stdfloat dist = _plane.dist_to_plane(from_center);

  LVecBase3 box_x = entry.get_wrt_mat().get_row3(0);
  LVecBase3 box_y = entry.get_wrt_mat().get_row3(1);
  LVecBase3 box_z = entry.get_wrt_mat().get_row3(2);

  if (abs(dist) >
      abs(box_x.dot(_plane.get_normal()) * from_extents[0]) +
      abs(box_y.dot(_plane.get_normal()) * from_extents[1]) +
      abs(box_z.dot(_plane.get_normal()) * from_extents[2])) {
    // No collision.
    return NULL;
  }

  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "intersection detected from " << entry.get_from_node_path()
      << " into " << entry.get_into_node_path() << "\n";
  }
  PT(CollisionEntry) new_entry = new CollisionEntry(entry);

  LVector3 normal = (has_effective_normal() && box->get_respect_effective_normal()) ? get_effective_normal() : get_normal();

  new_entry->set_surface_normal(normal);
  new_entry->set_surface_point(from_center - get_normal() * dist);

  return new_entry;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::fill_viz_geom
//       Access: Protected, Virtual
//  Description: Fills the _viz_geom GeomNode up with Geoms suitable
//               for rendering this solid.
////////////////////////////////////////////////////////////////////
void CollisionPlane::
fill_viz_geom() {
  if (collide_cat.is_debug()) {
    collide_cat.debug()
      << "Recomputing viz for " << *this << "\n";
  }

  // Since we can't represent an infinite plane, we'll have to be
  // satisfied with drawing a big polygon.  Choose four points on the
  // plane to be the corners of the polygon.

  // We must choose four points fairly reasonably spread apart on
  // the plane.  We'll start with a center point and one corner
  // point, and then use cross products to find the remaining three
  // corners of a square.

  // The center point will be on the axis with the largest
  // coefficent.  The first corner will be diagonal in the other two
  // dimensions.

  LPoint3 cp;
  LVector3 p1, p2, p3, p4;

  LVector3 normal = get_normal();
  PN_stdfloat D = _plane[3];

  if (fabs(normal[0]) > fabs(normal[1]) &&
      fabs(normal[0]) > fabs(normal[2])) {
    // X has the largest coefficient.
    cp.set(-D / normal[0], 0.0f, 0.0f);
    p1 = LPoint3(-(normal[1] + normal[2] + D)/normal[0], 1.0f, 1.0f) - cp;

  } else if (fabs(normal[1]) > fabs(normal[2])) {
    // Y has the largest coefficient.
    cp.set(0.0f, -D / normal[1], 0.0f);
    p1 = LPoint3(1.0f, -(normal[0] + normal[2] + D)/normal[1], 1.0f) - cp;

  } else {
    // Z has the largest coefficient.
    cp.set(0.0f, 0.0f, -D / normal[2]);
    p1 = LPoint3(1.0f, 1.0f, -(normal[0] + normal[1] + D)/normal[2]) - cp;
  }

  p1.normalize();
  p2 = cross(normal, p1);
  p3 = cross(normal, p2);
  p4 = cross(normal, p3);

  static const double plane_scale = 10.0;

  PT(GeomVertexData) vdata = new GeomVertexData
    ("collision", GeomVertexFormat::get_v3(),
     Geom::UH_static);
  GeomVertexWriter vertex(vdata, InternalName::get_vertex());
  
  vertex.add_data3(cp + p1 * plane_scale);
  vertex.add_data3(cp + p2 * plane_scale);
  vertex.add_data3(cp + p3 * plane_scale);
  vertex.add_data3(cp + p4 * plane_scale);
  
  PT(GeomTrifans) body = new GeomTrifans(Geom::UH_static);
  body->add_consecutive_vertices(0, 4);
  body->close_primitive();
  
  PT(GeomLinestrips) border = new GeomLinestrips(Geom::UH_static);
  border->add_consecutive_vertices(0, 4);
  border->add_vertex(0);
  border->close_primitive();
  
  PT(Geom) geom1 = new Geom(vdata);
  geom1->add_primitive(body);
  
  PT(Geom) geom2 = new Geom(vdata);
  geom2->add_primitive(border);
  
  _viz_geom->add_geom(geom1, get_solid_viz_state());
  _viz_geom->add_geom(geom2, get_wireframe_viz_state());
  
  _bounds_viz_geom->add_geom(geom1, get_solid_bounds_viz_state());
  _bounds_viz_geom->add_geom(geom2, get_wireframe_bounds_viz_state());
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void CollisionPlane::
write_datagram(BamWriter *manager, Datagram &me)
{
  CollisionSolid::write_datagram(manager, me);
  _plane.write_datagram(me);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void CollisionPlane::
fillin(DatagramIterator& scan, BamReader* manager)
{
  CollisionSolid::fillin(scan, manager);
  _plane.read_datagram(scan);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::make_CollisionPlane
//       Access: Protected
//  Description: Factory method to generate a CollisionPlane object
////////////////////////////////////////////////////////////////////
TypedWritable* CollisionPlane::
make_CollisionPlane(const FactoryParams &params)
{
  CollisionPlane *me = new CollisionPlane;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionPlane::register_with_factory
//       Access: Public, Static
//  Description: Factory method to generate a CollisionPlane object
////////////////////////////////////////////////////////////////////
void CollisionPlane::
register_with_read_factory()
{
  BamReader::get_factory()->register_factory(get_class_type(), make_CollisionPlane);
}
